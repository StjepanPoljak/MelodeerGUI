#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <melodeer/mdcore.h>
#include <melodeer/mdflac.h>
#include <melodeer/mdwav.h>
#include <melodeer/mdlame.h>

#include "mdgui.h"
#include "mdguifilebox.h"
#include "mdguimeta.h"

enum MD__filetype { MD__FLAC, MD__WAV, MD__MP3, MD__UNKNOWN };

typedef enum MD__filetype MD__filetype;

enum MDGUI__component { MDGUI__NONE, MDGUI__FILEBOX, MDGUI__METABOX, MDGUI__PLAYLIST };

typedef enum MDGUI__component MDGUI__component;

enum MDGUI__play_state { MDGUI__NOT_PLAYING, MDGUI__PAUSE, MDGUI__PLAYING,
                         MDGUI__WAITING_TO_STOP, MDGUI__INITIALIZING,
                         MDGUI__PROGRAM_EXIT, MDGUI__READY_TO_EXIT };

typedef enum MDGUI__play_state MDGUI__play_state;

MD__filetype    MD__get_extension       (const char *filename);

void            MD__handle_metadata     (MD__metadata_t metadata);
unsigned int    MD__get_seconds         (volatile MD__buffer_chunk_t *curr_chunk,
                                         unsigned int sample_rate,
                                         unsigned int channels,
                                         unsigned int bps);

void            transform               (volatile MD__buffer_chunk_t *curr_chunk,
                                         unsigned int sample_rate,
                                         unsigned int channels,
                                         unsigned int bps);

void            MD__cleanup             ();

void MDGUI__draw_meta_box_wrap ();
void MDGUI__play_complete ();
void draw_all ();

MDGUI__terminal tinfo;

MDGUI__terminal previous_tinfo;
pthread_mutex_t MDGUI__mutex;

// -1 - none selected
//  0 - file box
MDGUI__component selected_component = MDGUI__NONE;
MDGUI__component potential_component = MDGUI__NONE;
MDGUI__component previous_potential_component = MDGUI__NONE;

MDGUI__play_state volatile current_play_state = MDGUI__NOT_PLAYING;

char **MDGUI__playlist = NULL;
int MDGUI__playlist_current = -1;
int MDGUI__playlist_size = 0;
int MDGUI__playlist_highlighted = -1;
int MDGUI__playlist_first = 0;

MD__metadata_t curr_metadata;
bool curr_metadata_loaded = false;

int MDGUI__file_box_x = 2;
int MDGUI__file_box_y = 2;
int MDGUI__file_box_w = 20;
int MDGUI__file_box_h = 20;

int MDGUI__meta_box_x = 23;
int MDGUI__meta_box_y = 2;
int MDGUI__meta_box_w = 20;
int MDGUI__meta_box_h = 7;

pthread_t melodeer_thread;

pthread_t terminal_thread;

int selected_file = -1;
int first_line = 0;

char **ccont = NULL;
int cnum = 0;

char *curr_dir = NULL;
char *start_dir = NULL;

void redraw_file_box ()
{
    MDGUIFB__draw_file_box (ccont, cnum,
                           potential_component == MDGUI__FILEBOX, true,
                           first_line, selected_file, -1,
                           MDGUI__file_box_x, MDGUI__file_box_y,
                           MDGUI__file_box_w, MDGUI__file_box_h);
}

void redraw_playlist_box ()
{
    MDGUIFB__draw_file_box (MDGUI__playlist, MDGUI__playlist_size,
                           potential_component == MDGUI__PLAYLIST, false,
                           MDGUI__playlist_first, MDGUI__playlist_highlighted,
                           MDGUI__playlist_current - 1,
                           MDGUI__meta_box_x + MDGUI__meta_box_w,
                           MDGUI__file_box_y,
                           MDGUI__file_box_w, MDGUI__file_box_h);
}

void MDGUI__started_playing () {

    current_play_state = MDGUI__PLAYING;

    redraw_playlist_box ();

    char buff[PATH_MAX + 10];

    snprintf (buff, PATH_MAX + 10, "Playing: %s", MDGUI__playlist[MDGUI__playlist_current - 1]);

    MDGUI__log(buff, tinfo);

    return;
}

void MDGUI__handle_error (char *error) {

    MDGUI__log (error, tinfo);
    // current_play_state = MDGUI__NOT_PLAYING;

    return;
}

MD__file_t *curr_playing;

void *MDGUI__play (void *data) {

    MD__file_t current_file;

    void *(* decoder)(void *) = NULL;

    current_file.MD__buffer_transform = transform;

    MD__filetype type = MD__get_extension ((char *)data);

    switch (type) {

    case MD__FLAC:
        decoder = MDFLAC__start_decoding;
        break;

    case MD__WAV:
        decoder = MDWAV__parse;
        break;

    case MD__MP3:
        decoder = MDLAME__decoder;
        break;

    default:
        MDGUI__log ("(!) Unknown file type!",tinfo);
        current_play_state = MDGUI__NOT_PLAYING;
        return NULL;
    }

    if (MD__initialize (&current_file, (char *)data)) {

        curr_playing = &current_file;

        MD__play (&current_file, decoder, MD__handle_metadata, MDGUI__started_playing,
                  MDGUI__handle_error, MDGUI__play_complete);

    } else {

        MDGUI__log ("(!) Could not open file!", tinfo);
        current_play_state = MDGUI__NOT_PLAYING;
        return NULL;
    }

    return NULL;
}


char *will_play = NULL;

void MDGUI__start_playing () {

    pthread_mutex_unlock (&MDGUI__mutex);

    if (MDGUI__playlist_current < MDGUI__playlist_size){

        int curr_wp_size = MDGUI__get_string_size (MDGUI__playlist [MDGUI__playlist_current]) + 1;

        if (will_play) will_play = realloc(will_play, sizeof(*will_play)*curr_wp_size);
        else will_play = malloc(sizeof(*will_play)*curr_wp_size);

        for (int i=0; i<curr_wp_size; i++)

            will_play[i] = MDGUI__playlist[MDGUI__playlist_current][i];

        MDGUI__playlist_current++;
    }
    else return;

    if (will_play) {

        current_play_state = MDGUI__INITIALIZING;

        pthread_t melodeer_thread;

        if (pthread_create (&melodeer_thread, NULL, MDGUI__play, (void *)will_play))

            MDGUI__log ("(!) Could not create thread!", tinfo);
    }

    return;
}

void MDGUI__play_complete () {

    pthread_mutex_lock (&MDGUI__mutex);
    if (current_play_state == MDGUI__PROGRAM_EXIT) {

        current_play_state = MDGUI__READY_TO_EXIT;

        pthread_mutex_unlock (&MDGUI__mutex);

        return;
    }

    MDGUI__log ("Done playing!", tinfo);

    current_play_state = MDGUI__NOT_PLAYING;
    curr_metadata_loaded = false;

    pthread_mutex_unlock (&MDGUI__mutex);

    MDGUI__draw_meta_box_wrap ();

    MDGUI__start_playing ();

    return;
}

void layout () {

    MDGUI__file_box_w = (tinfo.cols - 3) / 3;
    MDGUI__file_box_h = tinfo.lines - 6;
    MDGUI__meta_box_w = MDGUI__file_box_w;
}

void *terminal_change (void *data) {

    previous_tinfo = tinfo;

    struct timespec tsp = {0,10000};

    while (true) {

        nanosleep(&tsp, NULL);

        pthread_mutex_lock (&MDGUI__mutex);

        if (current_play_state == MDGUI__READY_TO_EXIT) {

            pthread_mutex_unlock (&MDGUI__mutex);

            break;
        }

        tinfo = MDGUI__get_terminal_information ();

        if (tinfo.cols != previous_tinfo.cols || tinfo.lines != previous_tinfo.lines) {

            previous_tinfo = tinfo;

            layout ();

            if (MDGUI__file_box_h >= cnum) first_line = 0;

            if (MDGUI__file_box_x >= MDGUI__playlist_size) MDGUI__playlist_first = 0;

            clear();

            draw_all ();

            unsigned int tinfo_size = snprintf(NULL, 0, "Changed size to %d x %d.", tinfo.cols, tinfo.lines) + 1;
            char *tinfo_string = malloc (sizeof(*tinfo_string) * tinfo_size);
            snprintf(tinfo_string, tinfo_size, "Changed size to %d x %d.", tinfo.cols, tinfo.lines);


            MDGUI__log (tinfo_string, tinfo);
        }

        pthread_mutex_unlock (&MDGUI__mutex);
    }

    return NULL;
}


void draw_all () {

    redraw_file_box ();

    MDGUI__draw_meta_box_wrap ();

    redraw_playlist_box ();
}

void MD__cleanup() {

    if (MDGUI__playlist) {

        for (int i = 0; i < MDGUI__playlist_size; i++) if (MDGUI__playlist[i]) free(MDGUI__playlist[i]);
        free (MDGUI__playlist);
    }

    if (ccont) {

        for (int i=0;i<cnum;i++) if (ccont[i]) free(ccont[i]);
        free (ccont);
    }

    if (curr_dir) free (curr_dir);
    if (start_dir) free (start_dir);
    if (will_play) free (will_play);
}

bool key_pressed (char key[3]) {

    if (key[0] == 27 && key[1] == 0 && key[2] == 0) {

        // ESCAPE

        switch (selected_component) {

        case MDGUI__NONE:

            // pthread_join (terminal_thread, NULL);

            return false;

        default:

            selected_component = MDGUI__NONE;

            potential_component = previous_potential_component;

            draw_all ();
        }

    } else if ((key [0] == 13 || key [0] == 10) && key [1] == 0 && key[2] == 0) {

        // RETURN

        switch (selected_component) {

        case MDGUI__NONE:

            selected_component = potential_component;

            previous_potential_component = potential_component;

            potential_component = MDGUI__NONE;

            draw_all();

            break;

        case MDGUI__FILEBOX:

            if (current_play_state == MDGUI__WAITING_TO_STOP || current_play_state == MDGUI__INITIALIZING) break;

            char *olddir = curr_dir == NULL ? start_dir : curr_dir;

            switch (ccont[selected_file][0]) {

            case 'f':

                ;

                int old_playlist_size = MDGUI__playlist_size;

                MDGUI__playlist_size = cnum - selected_file;

                if (MDGUI__playlist_size > 0) {

                    MDGUI__playlist_current = 0;
                    MDGUI__playlist_first = 0;
                    MDGUI__playlist_highlighted = -1;

                    if (MDGUI__playlist) {

                        for (int i = 0; i < old_playlist_size; i++) {

                            char *oldchars = MDGUI__playlist[i];

                            if (oldchars) free (oldchars);
                        }

                        MDGUI__playlist = realloc (MDGUI__playlist, sizeof (*MDGUI__playlist) * MDGUI__playlist_size);

                    } else MDGUI__playlist = malloc (sizeof (*MDGUI__playlist) * MDGUI__playlist_size);

                    for (int i = 0; i < MDGUI__playlist_size; i++) {

                        char *tempfn = NULL;

                        MDGUIFB__append_to_dirname (&tempfn, olddir, &ccont [i + selected_file][1]);

                        int curr_string_size = MDGUI__get_string_size (tempfn);

                        MDGUI__playlist[i] = tempfn;
                    }
                }

                if (current_play_state == MDGUI__PLAYING || current_play_state == MDGUI__PAUSE) {

                    current_play_state = MDGUI__WAITING_TO_STOP;

                    MD__stop (curr_playing);

                    MDGUI__log ("Waiting to stop.", tinfo);

                } else MDGUI__start_playing ();

                break;

            case 'd':

                if (MDGUI__get_string_size (ccont[selected_file]) == 2) {

                    if (ccont[selected_file][1] == '.') break;
                }

                if (MDGUI__get_string_size (ccont[selected_file]) == 3) {

                    if (ccont[selected_file][1] == '.' && ccont[selected_file][2] == '.') {

                        MDGUIFB__get_parent_dir (&curr_dir, olddir);

                        MDGUIFB__get_dir_contents (&ccont, &cnum, curr_dir == NULL ? olddir : curr_dir);

                        MDGUI__sort (&ccont, cnum, MDGUIFB__compare);

                        selected_file = 0;
                        first_line = 0;

                        redraw_file_box ();

                        break;
                    }
                }

                MDGUIFB__append_to_dirname (&curr_dir, olddir, &ccont [selected_file][1]);

                MDGUIFB__get_dir_contents (&ccont, &cnum, curr_dir == NULL ? olddir : curr_dir);

                MDGUI__sort (&ccont, cnum, MDGUIFB__compare);

                selected_file = 0;
                first_line = 0;

                // draw_all ();
                redraw_file_box ();

                break;

            default:

                break;
            }

            break;

        default:

            break;
        }

    } else if (key[0] == 27 && key[1] == 91 && key[2] == 66) {

        // DOWN ARROW

        switch (selected_component) {

        case MDGUI__NONE:

            potential_component = MDGUI__FILEBOX;

            draw_all();

            break;

        case MDGUI__FILEBOX:

            if (selected_file < first_line && selected_file >= 0) first_line = selected_file;
            else if (selected_file > first_line + MDGUI__file_box_h - 2) {

                selected_file = first_line + MDGUI__file_box_h - 3;
                redraw_file_box ();
                break;
            }

            if (selected_file < cnum - 1) selected_file++;
            if (selected_file == first_line + MDGUI__file_box_h - 2) first_line++;

            redraw_file_box ();

            break;

        case MDGUI__PLAYLIST:

            if (MDGUI__playlist_highlighted < MDGUI__playlist_first && MDGUI__playlist_highlighted >= 0) MDGUI__playlist_first = MDGUI__playlist_highlighted;
            else if (MDGUI__playlist_highlighted > MDGUI__playlist_first + MDGUI__file_box_h - 2) {

                MDGUI__playlist_highlighted = MDGUI__playlist_first + MDGUI__file_box_h - 3;
                redraw_playlist_box ();
                break;
            }

            if (MDGUI__playlist_highlighted < MDGUI__playlist_size - 1) MDGUI__playlist_highlighted++;
            if (MDGUI__playlist_highlighted == MDGUI__playlist_first + MDGUI__file_box_h - 2) MDGUI__playlist_first++;

            redraw_playlist_box ();

            break;

        default:

            break;
        }

    } else if (key[0] == 27 && key[1] == 91 && key[2] == 65) {

        // UP ARROW

        switch (selected_component) {

        case MDGUI__NONE:

            potential_component = MDGUI__FILEBOX;

            draw_all();

            break;

        case MDGUI__FILEBOX:

            if (selected_file < first_line && selected_file >= 0) first_line = selected_file;
            else if (selected_file > first_line + MDGUI__file_box_h - 2) selected_file = first_line + MDGUI__file_box_h - 2;

            if (selected_file < 0) selected_file = 0;
            if (selected_file > 0) selected_file--;
            if (selected_file == first_line - 1 && first_line != 0) first_line--;

            redraw_file_box ();

            break;

        case MDGUI__PLAYLIST:

            if (MDGUI__playlist_highlighted < MDGUI__playlist_first && MDGUI__playlist_highlighted >= 0) MDGUI__playlist_first = MDGUI__playlist_highlighted;
            else if (MDGUI__playlist_highlighted > MDGUI__playlist_first + MDGUI__file_box_h - 2) MDGUI__playlist_highlighted = MDGUI__playlist_first + MDGUI__file_box_h - 2;

            if (MDGUI__playlist_highlighted < 0) MDGUI__playlist_highlighted = 0;
            if (MDGUI__playlist_highlighted > 0) MDGUI__playlist_highlighted--;
            if (MDGUI__playlist_highlighted == MDGUI__playlist_first - 1 && MDGUI__playlist_first != 0) MDGUI__playlist_first--;

            redraw_playlist_box ();

            break;

        default:

            break;
        }
    } else if (key[0] == 27 && key[1] == 91 && key[2] == 67) {

        // RIGHT ARROW

        switch (selected_component) {

        case MDGUI__NONE:

            switch (potential_component) {

            case MDGUI__NONE:

                potential_component = MDGUI__FILEBOX;
                break;

            case MDGUI__FILEBOX:

                potential_component = MDGUI__METABOX;
                break;

            case MDGUI__METABOX:

                potential_component = MDGUI__PLAYLIST;
                break;

            case MDGUI__PLAYLIST:

                potential_component = MDGUI__PLAYLIST;
                break;

            default:

                break;
            }

            draw_all ();
            break;

        default:

            break;
        }
    } else if (key[0] == 27 && key[1] == 91 && key[2] == 68) {

        // LEFT ARROW

        switch (selected_component) {

        case MDGUI__NONE:

            switch (potential_component) {

            case MDGUI__NONE:

                potential_component = MDGUI__FILEBOX;
                break;

            case MDGUI__METABOX:

                potential_component = MDGUI__FILEBOX;
                break;

            case MDGUI__PLAYLIST:

                potential_component = MDGUI__METABOX;
                break;

            default:

                break;
            }

            draw_all ();
            break;

        default:

            break;
        }
    }

    return true;
}

void mdgui_completion () {

    pthread_mutex_lock (&MDGUI__mutex);

    if (current_play_state == MDGUI__PLAYING || current_play_state == MDGUI__PAUSE) {

        current_play_state = MDGUI__PROGRAM_EXIT;

        pthread_mutex_unlock (&MDGUI__mutex);

        MD__stop (curr_playing);

        for (;;) {
            pthread_mutex_lock (&MDGUI__mutex);
            if (current_play_state == MDGUI__READY_TO_EXIT) {

                pthread_mutex_unlock (&MDGUI__mutex);
                break;
            }
            pthread_mutex_unlock (&MDGUI__mutex);
        }
    }
    else {

        current_play_state = MDGUI__PROGRAM_EXIT;
        pthread_mutex_unlock (&MDGUI__mutex);
    }

    MD__cleanup ();

    MDAL__close();
}

int main (int argc, char *argv[]) {

    MDAL__initialize (4096, 4, 4);

    pthread_mutex_init (&MDGUI__mutex, NULL);

    initscr();

    tinfo = MDGUI__get_terminal_information ();

    layout ();

    MDGUIFB__get_current_dir (&start_dir);

    clear();

    curs_set (0);

    MDGUIFB__get_dir_contents (&ccont, &cnum, start_dir);

    draw_all ();

    MDGUI__log ("Use arrow keys to move, ENTER to select and ESC to deselect/exit.", tinfo);

    if (pthread_create(&terminal_thread, NULL, terminal_change, NULL)) {
        return 0;
    }

    MDGUI__wait_for_keypress(key_pressed, mdgui_completion);

    return 0;
}

MD__filetype MD__get_extension (const char *filename) {

    char curr = filename [0];
    unsigned int last_dot_position = -1;
    unsigned int i = 0;

    while (curr != 0) {

        if (curr == '.') {

            last_dot_position = i;
        }

        curr = filename [i++];
    }

    if (last_dot_position == -1) return MD__UNKNOWN;

    unsigned int diff = i - last_dot_position;

    if (diff == 5) {

        if (filename [last_dot_position]     == 'f'
         && filename [last_dot_position + 1] == 'l'
         && filename [last_dot_position + 2] == 'a'
         && filename [last_dot_position + 3] == 'c') {

            return MD__FLAC;
        }
    }

    if (diff == 4) {

        if (filename [last_dot_position]     == 'w'
         && filename [last_dot_position + 1] == 'a'
         && filename [last_dot_position + 2] == 'v') {

            return MD__WAV;
        }

        if (filename [last_dot_position]     == 'm'
         && filename [last_dot_position + 1] == 'p'
         && filename [last_dot_position + 2] == '3') {

            return MD__MP3;
        }
    }

    return MD__UNKNOWN;
}

void MDGUI__draw_meta_box_wrap () {

    MDGUI__meta_box_x = MDGUI__file_box_x + MDGUI__file_box_w;

    if (curr_metadata_loaded)

        MDGUIMD__draw_meta_box (curr_metadata.total_samples,
                                curr_metadata.sample_rate,
                                curr_metadata.channels,
                                curr_metadata.bps,
                                potential_component == MDGUI__METABOX,
                                MDGUI__meta_box_x, MDGUI__meta_box_y,
                                MDGUI__meta_box_w, MDGUI__meta_box_h);

    else {

        MDGUI__draw_box (potential_component == MDGUI__METABOX,
                         MDGUI__meta_box_x, MDGUI__meta_box_y,
                         MDGUI__meta_box_w, MDGUI__meta_box_h);

        char *message = "no file loaded";
        int message_length = MDGUI__get_string_size (message);

        mvprintw(MDGUI__meta_box_y + MDGUI__meta_box_h / 2,
                                MDGUI__meta_box_x + (MDGUI__meta_box_w - message_length) / 2, "%s",
                                message);

        refresh();
    }
}

void MD__handle_metadata (MD__metadata_t metadata) {
    curr_metadata = metadata;
    curr_metadata_loaded = true;
    MDGUI__draw_meta_box_wrap ();

}

void transform (volatile MD__buffer_chunk_t *curr_chunk,
                unsigned int sample_rate,
                unsigned int channels,
                unsigned int bps) {

    for (int i=0; i<curr_chunk->size/((bps/8)*channels); i++) {

        for (int c=0; c<channels; c++) {

            // this will depend on bps...
            short data = 0;

            for (int b=0; b<bps/8; b++) {

                data = data + ((short)(curr_chunk->chunk[i*channels*(bps/8)+(c*channels)+b])<<(8*b));
            }

            // transform

            for (int b=0; b<bps/8; b++) {

                curr_chunk->chunk[i*channels*(bps/8)+(c*channels)+b] = data >> 8*b;
            }
        }
    }
}

unsigned int MD__get_seconds (volatile MD__buffer_chunk_t *curr_chunk,
                              unsigned int sample_rate,
                              unsigned int channels,
                              unsigned int bps) {

    return (curr_chunk->order * curr_chunk->size / (bps * channels / 8)) / sample_rate;

}
