#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <melodeer/mdcore.h>
#include <melodeer/mdflac.h>
#include <melodeer/mdwav.h>
#include <melodeer/mdmpg123.h>

#include "mdgui.h"
#include "mdguifilebox.h"
#include "mdguimeta.h"

enum MD__filetype { MD__FLAC, MD__WAV, MD__MP3, MD__UNKNOWN };

typedef enum MD__filetype MD__filetype;

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
// void draw_all ();

MD__file_t *curr_playing = NULL;
MDGUI__terminal previous_tinfo;


MDGUI__terminal tinfo;

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

// int MDGUI__file_box_x = 2;
// int MDGUI__file_box_y = 2;
// int MDGUI__file_box_w = 20;
// int MDGUI__file_box_h = 20;

// int MDGUI__meta_box_x = 23;
// int MDGUI__meta_box_y = 9;
// int MDGUI__meta_box_w = 20;
// int MDGUI__meta_box_h = 7;

pthread_t melodeer_thread;

pthread_t terminal_thread;

int selected_file = -1;
int first_line = 0;

char **ccont = NULL;
int cnum = 0;

char *curr_dir = NULL;
char *start_dir = NULL;

//
// void redraw_file_box () {
//
//
//     refresh ();
// }
//
// void redraw_playlist_box () {
//
//     int MDGUI__playlist_box_x = MDGUI__meta_box_x + MDGUI__meta_box_w;
//
//     MDGUIFB__draw_file_box (MDGUI__playlist, MDGUI__playlist_size,
//                            potential_component == MDGUI__PLAYLIST, false,
//                            MDGUI__playlist_first, MDGUI__playlist_highlighted,
//                            MDGUI__playlist_current - 1,
//                            MDGUI__playlist_box_x,
//                            MDGUI__file_box_y,
//                            MDGUI__file_box_w, MDGUI__file_box_h);
//
//     if (selected_component == MDGUI__PLAYLIST) attron (A_REVERSE);
//
//     mvprintw (MDGUI__file_box_y, MDGUI__playlist_box_x + (MDGUI__file_box_w - 10) / 2, " playlist ");
//
//     if (selected_component == MDGUI__PLAYLIST) attroff (A_REVERSE);
//
//     refresh ();
//
// }

void redraw_logo () {

    char logo [76];
    int line = 0;
    int col = 0;

    MD__get_logo (logo);

    if (potential_component == MDGUI__LOGO) attron (A_BOLD);

    bool reversing = false;
    int lastrev = -1;

    for (int i=0; i<75; i++) {

        if (logo[i] == '\n') {

            if (line == 4 && potential_component == MDGUI__LOGO) attroff (A_BOLD);
            if (line == 5 && potential_component == MDGUI__LOGO) attron (A_BOLD);
            line++;

            col = 0;
            continue;
        }

        if (logo[i] == '~' && selected_component == MDGUI__LOGO && !reversing) {
            reversing = true;
            lastrev = i;
            attron (A_REVERSE);
        }

        // mvprintw (1 + line, MDGUI__meta_box_x + (MDGUI__meta_box_w - 12) / 2 + col++, "%c", logo[i]);


        if (logo[i] == '~' && i != lastrev && selected_component == MDGUI__LOGO && reversing) {
            reversing = false;
            attroff (A_REVERSE);
        }

    }

    if (potential_component == MDGUI__LOGO) attroff (A_BOLD);

    refresh ();
}

void MDGUI__started_playing () {

    current_play_state = MDGUI__PLAYING;

    // redraw_playlist_box ();

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
        decoder = MDMPG123__decoder;
        break;

    default:
        MDGUI__log ("(!) Unknown file type!",tinfo);
        current_play_state = MDGUI__NOT_PLAYING;
        return NULL;
    }

    if (MD__initialize (&current_file, (char *)data)) {

        curr_playing = &current_file;

        MD__play (&current_file, decoder, MD__handle_metadata, MDGUI__started_playing,
                  MDGUI__handle_error, NULL, MDGUI__play_complete);

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

    // if (MDGUI__stop_all_signal) {

    //     MDGUI__stop_all_signal = false;

    //     current_play_state = MDGUI__NOT_PLAYING;

    //     curr_metadata_loaded = false;

    //     pthread_mutex_unlock (&MDGUI__mutex);

    //     MDGUI__draw_meta_box_wrap ();

    //     return;
    // }

    if (current_play_state == MDGUI__PROGRAM_EXIT) {

        current_play_state = MDGUI__READY_TO_EXIT;

        pthread_mutex_unlock (&MDGUI__mutex);

        return;
    }

    MDGUI__log ("Done playing!", tinfo);

    current_play_state = MDGUI__NOT_PLAYING;
    curr_metadata_loaded = false;

    pthread_mutex_unlock (&MDGUI__mutex);

    // MDGUI__draw_meta_box_wrap ();

    MDGUI__start_playing ();

    return;
}

// void layout () {
//
//     MDGUI__file_box_w = (tinfo.cols - 3) / 3;
//     MDGUI__file_box_h = tinfo.lines - 6;
//     MDGUI__meta_box_w = MDGUI__file_box_w;
// }




// void draw_all () {

    // redraw_file_box ();

    // MDGUI__draw_meta_box_wrap ();

    // redraw_playlist_box ();

    // redraw_logo ();
// }

void MD__cleanup() {

    // if (MDGUI__playlist) {
    //
    //     for (int i = 0; i < MDGUI__playlist_size; i++)
    //
    //         if (MDGUI__playlist[i]) free (MDGUI__playlist[i]);
    //
    //     free (MDGUI__playlist);
    // }
    //
    // if (ccont) {
    //
    //     for (int i=0; i<cnum; i++) if (ccont[i]) free (ccont[i]);
    //     free (ccont);
    // }
    //
    // if (curr_dir) free (curr_dir);
    // if (start_dir) free (start_dir);
    // if (will_play) free (will_play);
}

// void MDGUI__realloc_playlist (int old_playlist_size) {
//
//     if (MDGUI__playlist) {
//
//         for (int i = 0; i < old_playlist_size; i++) {
//
//             char *oldchars = MDGUI__playlist[i];
//
//             if (oldchars) free (oldchars);
//         }
//
//         MDGUI__playlist = realloc (MDGUI__playlist, sizeof (*MDGUI__playlist) * MDGUI__playlist_size);
//
//     } else MDGUI__playlist = malloc (sizeof (*MDGUI__playlist) * MDGUI__playlist_size);
//
// }
//
// void MDGUI__insert_current_to_playlist () {
//
//     int old_playlist_size = MDGUI__playlist_size;
//
//     char **temp = malloc (MDGUI__playlist_size * sizeof(*temp));
//
//     for (int i = 0; i < MDGUI__playlist_size; i++) {
//
//         int string_size = MDGUI__get_string_size(MDGUI__playlist[i]);
//
//         temp[i] = malloc (string_size + 1);
//
//         for (int j = 0; j <= string_size; j++) temp[i][j] = MDGUI__playlist[i][j];
//     }
//
//     MDGUI__playlist_size++;
//
//     MDGUI__realloc_playlist (old_playlist_size);
//
//     for (int i = 0; i < old_playlist_size; i++) {
//
//         if (temp[i]) free(temp[i]);
//     }
//
//     free (temp);
// }
//
// void MDGUI__generate_playlist (char *olddir) {
//
//     int old_playlist_size = MDGUI__playlist_size;
//
//     MDGUI__playlist_size = cnum - selected_file;
//
//     if (MDGUI__playlist_size > 0) {
//
//         MDGUI__playlist_current = 0;
//         MDGUI__playlist_first = 0;
//         MDGUI__playlist_highlighted = -1;
//
//         MDGUI__realloc_playlist (old_playlist_size);
//
//         for (int i = 0; i < MDGUI__playlist_size; i++) {
//
//             char *tempfn = NULL;
//
//             MDGUIFB__append_to_dirname (&tempfn, olddir, &ccont [i + selected_file][1]);
//
//             int curr_string_size = MDGUI__get_string_size (tempfn);
//
//             MDGUI__playlist[i] = tempfn;
//         }
//     }
// }



int main (int argc, char *argv[]) {

    return 0;

    

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

    // MDGUI__meta_box_x = MDGUI__file_box_x + MDGUI__file_box_w;
    //
    // if (curr_metadata_loaded)
    //
    //     MDGUIMD__draw_meta_box (curr_metadata.total_samples,
    //                             curr_metadata.sample_rate,
    //                             curr_metadata.channels,
    //                             curr_metadata.bps,
    //                             potential_component == MDGUI__METABOX,
    //                             MDGUI__meta_box_x, MDGUI__meta_box_y,
    //                             MDGUI__meta_box_w, MDGUI__meta_box_h);
    // else {
    //
    //     // MDGUI__draw_box (potential_component == MDGUI__METABOX,
    //     //                  MDGUI__meta_box_x, MDGUI__meta_box_y,
    //     //                  MDGUI__meta_box_w, MDGUI__meta_box_h);
    //
    //     char *message = "no file loaded";
    //     int message_length = MDGUI__get_string_size (message);
    //
    //     mvprintw(MDGUI__meta_box_y + MDGUI__meta_box_h / 2,
    //                             MDGUI__meta_box_x + (MDGUI__meta_box_w - message_length) / 2, "%s",
    //                             message);
    //
    //     refresh();
    // }
    //
    // if (selected_component == MDGUI__METABOX) attron (A_REVERSE);
    //
    // mvprintw (MDGUI__meta_box_y, MDGUI__meta_box_x + (MDGUI__meta_box_w - 10) / 2, " metadata ");
    //
    // if (selected_component == MDGUI__METABOX) attroff (A_REVERSE);
    //
    // refresh ();
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
