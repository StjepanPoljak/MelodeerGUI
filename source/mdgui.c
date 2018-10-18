#include "mdgui.h"

#include <melodeer/mdflac.h>
#include <melodeer/mdwav.h>
#include <melodeer/mdmpg123.h>


enum MD__filetype { MD__FLAC, MD__WAV, MD__MP3, MD__UNKNOWN };

typedef enum MD__filetype MD__filetype;

MD__filetype MD__get_filetype (const char *filename) {

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

void MDGUI__wait_for_keypress (MDGUI__manager_t *mdgui, bool (key_pressed)(MDGUI__manager_t *mdgui, char [3]), void (on_completion)(MDGUI__manager_t *mdgui));
void MDGUI__deinit (MDGUI__manager_t *mdgui);
bool key_pressed (MDGUI__manager_t *mdgui, char key[3]);
void *terminal_change (void *data);
void MDGUI__draw_logo (MDGUI__manager_t *mdgui);

int MDGUI__get_box_width (MDGUI__manager_t *mdgui) {

    return (mdgui->tinfo.cols - mdgui->left - mdgui->right) / 3;
}

int MDGUI__get_box_height (MDGUI__manager_t *mdgui) {

    return (mdgui->tinfo.lines - mdgui->top - mdgui->bottom);
}

int MDGUI__get_box_x (MDGUI__manager_t *mdgui, int order) {

    return (mdgui->left + (order * MDGUI__get_box_width(mdgui)));
}

bool MDGUI__init (MDGUI__manager_t *mdgui) {

    mdgui->top = 2;
    mdgui->bottom = 3;
    mdgui->left = 2;
    mdgui->right = 2;
    mdgui->meta_top = 7;
    mdgui->meta_bottom = 2;

    MDAL__initialize (4096, 4, 4);

    pthread_mutex_init (&mdgui->mutex, NULL);

    mdgui->tinfo = MDGUI__get_terminal_information ();

    mdgui->selected_component = MDGUI__NONE;
    mdgui->potential_component = MDGUI__NONE;
    mdgui->previous_potential_component = MDGUI__NONE;

    mdgui->stop_all_signal = false;

    int box_width = MDGUI__get_box_width (mdgui);
    int box_height = MDGUI__get_box_height (mdgui);

    mdgui->filebox = MDGUIFB__create ("files", MDGUI__get_box_x (mdgui, 0), mdgui->top, box_height, box_width);

    mdgui->metabox = MDGUIMB__create ("metadata", MDGUI__get_box_x (mdgui, 1), mdgui->top + mdgui->meta_top, box_height - mdgui->meta_bottom - mdgui->meta_top, box_width);

    mdgui->playlistbox = MDGUIPB__create ("playlist", MDGUI__get_box_x (mdgui, 2), mdgui->top, box_height, box_width);

    MDGUI__play_state volatile current_play_state = MDGUI__NOT_PLAYING;

    initscr ();

    clear ();

    curs_set (0);

    MDGUI__draw (mdgui);

    MDGUI__log ("Use arrow keys to move, ENTER to select and ESC to deselect/exit.", mdgui->tinfo);

    if (pthread_create (&mdgui->terminal_thread, NULL, terminal_change, mdgui)) {

        return false;
    }

    MDGUI__wait_for_keypress (mdgui, key_pressed, MDGUI__deinit);

    pthread_join (mdgui->terminal_thread, NULL);

    return true;
}

void MDGUI__mutex_lock (MDGUI__manager_t *mdgui) {

    pthread_mutex_lock (&mdgui->mutex);
}

void MDGUI__mutex_unlock (MDGUI__manager_t *mdgui) {

    pthread_mutex_unlock (&mdgui->mutex);
}

void MDGUI__clear_screen () {

    clear ();
}

void MDGUI__log (const char *log, MDGUI__terminal tinfo) {

    move (tinfo.lines - 1, 0);
    clrtoeol ();

    int i = 0;

    while (true) {

        if (i >= tinfo.cols) break;

        if (log[i] == 0) break;

        mvprintw (tinfo.lines - 1, i, "%c", log[i]);

        i++;
    }

    refresh ();
}

MDGUI__terminal MDGUI__get_terminal_information () {

    MDGUI__terminal tinfo;

    struct winsize w;
    ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);

    tinfo.cols = w.ws_col;
    tinfo.lines = w.ws_row;

    return tinfo;
}

struct MDGUI__prepend_info {

    char *curr_dir;
    int curr_dir_str_size;
};

void MDGUI__str_transform_prepend_dir (void *data, char *src, char **dest) {

    struct MDGUI__prepend_info *dir_info = ((struct MDGUI__prepend_info *)data);

    *dest = malloc (sizeof (**dest) * dir_info->curr_dir_str_size);

}

void MDGUI__wait_for_keypress (MDGUI__manager_t *mdgui, bool (key_pressed)(MDGUI__manager_t *mdgui, char [3]), void (on_completion)(MDGUI__manager_t *mdgui)) {

    struct termios orig_term_attr;
    struct termios new_term_attr;

    char chain[3];

    tcgetattr (fileno(stdin), &orig_term_attr);
    memcpy (&new_term_attr, &orig_term_attr, sizeof (struct termios));
    new_term_attr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr (fileno(stdin), TCSANOW, &new_term_attr);

    fd_set input_set, output_set;

    for(;;) {

        char buff[3];
        memset (buff, 0, 3);

        FD_ZERO (&input_set);
        FD_SET (STDIN_FILENO, &input_set);

        int readn = select(1, &input_set, NULL, NULL, NULL);

        if (FD_ISSET (STDIN_FILENO, &input_set))
        {
            int buffread = read(STDIN_FILENO, buff, 3);

            if (!key_pressed(mdgui, buff)) break;
        }
    }

    on_completion (mdgui);

    tcsetattr (fileno (stdin), TCSANOW, &orig_term_attr);

    curs_set (1);

    clear ();

    endwin();

    return;
}

void MDGUI__draw (MDGUI__manager_t *mdgui) {

    MDGUI__draw_logo (mdgui);
    MDGUIFB__draw (&mdgui->filebox);
    MDGUIPB__draw (&mdgui->playlistbox);
    MDGUIMB__draw (&mdgui->metabox);
}

void *MDGUI__play (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MD__file_t current_file;

    void *(* decoder)(void *) = NULL;

    current_file.MD__buffer_transform = NULL;

    MD__filetype type = MD__get_filetype ((char *)data);

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
        MDGUI__log ("(!) Unknown file type!",mdgui->tinfo);
        mdgui->current_play_state = MDGUI__NOT_PLAYING;
        return NULL;
    }

    if (MD__initialize (&current_file, (char *)data)) {

        mdgui->curr_playing = &current_file;

        // MD__play (&current_file, decoder, MD__handle_metadata, MDGUI__started_playing,
        //           MDGUI__handle_error, NULL, MDGUI__play_complete);

    } else {

        MDGUI__log ("(!) Could not open file!", mdgui->tinfo);
        mdgui->current_play_state = MDGUI__NOT_PLAYING;
        return NULL;
    }

    mdgui->curr_playing = NULL;

    return NULL;
}

bool key_pressed (MDGUI__manager_t *mdgui, char key[3]) {

    if (key[0] == 27 && key[1] == 0 && key[2] == 0) {

        // ESCAPE

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            return false;

        default:

            switch (mdgui->selected_component) {

            case MDGUI__FILEBOX:

                MDGUI__deselect_box (&mdgui->filebox.listbox.box);
                break;

            case MDGUI__PLAYLIST:

                MDGUI__deselect_box (&mdgui->playlistbox.listbox.box);
                break;

            case MDGUI__METABOX:

                MDGUI__deselect_box (&mdgui->metabox.box);
                break;

            case MDGUI__LOGO:

                mdgui->selected_component = MDGUI__NONE;
                mdgui->potential_component = MDGUI__LOGO;

                MDGUI__draw_logo (mdgui);
                return true;

            default:
                break;
            }

            mdgui->selected_component = MDGUI__NONE;

            mdgui->potential_component = mdgui->previous_potential_component;
        }

    }
    else if ((key [0] == 13 || key [0] == 10) && key [1] == 0 && key[2] == 0) {

        // RETURN

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            mdgui->selected_component = mdgui->potential_component;

            mdgui->previous_potential_component = mdgui->potential_component;

            mdgui->potential_component = MDGUI__NONE;

            switch (mdgui->selected_component) {

            case MDGUI__FILEBOX:

                MDGUI__select_box (&mdgui->filebox.listbox.box);
                break;

            case MDGUI__PLAYLIST:

                MDGUI__select_box (&mdgui->playlistbox.listbox.box);
                break;

            case MDGUI__METABOX:

                MDGUI__select_box (&mdgui->metabox.box);
                break;

            case MDGUI__LOGO:

                MDGUI__draw_logo (mdgui);
                break;

            default:
                break;
            }

            break;

        case MDGUI__FILEBOX:

            if (mdgui->current_play_state == MDGUI__WAITING_TO_STOP || mdgui->current_play_state == MDGUI__INITIALIZING) break;

            if (MDGUIFB__return (&mdgui->filebox)) {

                // MDGUI__str_array_copy_all_from (&mdgui->filebox.listbox.str_array,
                //                                 &mdgui->playlistbox.str_array,
                //                                 mdgui->filebox.listbox.num_selected, 1);

                MDGUIPB__redraw (&mdgui->playlistbox);
            }

            break;

        case MDGUI__PLAYLIST:

            if (mdgui->current_play_state == MDGUI__PLAYING || mdgui->current_play_state == MDGUI__PAUSE) {

                mdgui->current_play_state = MDGUI__WAITING_TO_STOP;

                MD__stop (mdgui->curr_playing);

                //MDGUI__playlist_current = MDGUI__playlist_highlighted;

                MDGUI__log ("Waiting to stop.", mdgui->tinfo);

            } else {

                //MDGUI__playlist_current = MDGUI__playlist_highlighted;

                // MDGUI__start_playing ();
            }

            break;

        default:

            break;
        }
    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 66) {

        // DOWN ARROW
        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            if (mdgui->potential_component == MDGUI__LOGO) {

                mdgui->potential_component = MDGUI__METABOX;
                MDGUI__draw_logo (mdgui);
                MDGUI__highlight_box (&mdgui->metabox.box);
            }
            else if (mdgui->potential_component == MDGUI__NONE) {

                mdgui->potential_component = MDGUI__METABOX;
                MDGUI__highlight_box (&mdgui->metabox.box);
            }

            break;

        case MDGUI__FILEBOX:

            MDGUILB__down_arrow (&mdgui->filebox.listbox);
            MDGUIFB__redraw (&mdgui->filebox);

            break;

        case MDGUI__PLAYLIST:

            MDGUILB__down_arrow (&mdgui->playlistbox.listbox);
            MDGUIPB__redraw (&mdgui->playlistbox);
            
            break;

        default:

            break;
        }

    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 65) {

        // UP ARROW

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            if (mdgui->potential_component == MDGUI__METABOX) {

                mdgui->potential_component = MDGUI__LOGO;
                MDGUI__draw_logo (mdgui);
                MDGUI__unhighlight_box (&mdgui->metabox.box);
            }

            else if (mdgui->potential_component == MDGUI__NONE) {

                mdgui->potential_component = MDGUI__LOGO;
                MDGUI__draw_logo (mdgui);
            }

            break;

        case MDGUI__FILEBOX:

            MDGUILB__up_arrow (&mdgui->filebox.listbox);
            MDGUIFB__redraw (&mdgui->filebox);

            break;

        case MDGUI__PLAYLIST:

            MDGUILB__up_arrow (&mdgui->playlistbox.listbox);
            MDGUIPB__redraw (&mdgui->playlistbox);

            break;

        default:

            break;
        }
    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 67) {

        // RIGHT ARROW

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            switch (mdgui->potential_component) {

            case MDGUI__NONE:

                mdgui->potential_component = MDGUI__PLAYLIST;
                MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);
                break;

            case MDGUI__FILEBOX:

                mdgui->potential_component = MDGUI__METABOX;

                MDGUI__unhighlight_box (&mdgui->filebox.listbox.box);
                MDGUI__highlight_box (&mdgui->metabox.box);

                break;

            case MDGUI__LOGO:

                mdgui->potential_component = MDGUI__PLAYLIST;
                MDGUI__draw_logo (mdgui);
                MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);

                break;

            case MDGUI__METABOX:

                mdgui->potential_component = MDGUI__PLAYLIST;
                MDGUI__unhighlight_box (&mdgui->metabox.box);
                MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);

                break;

            default:

                break;
            }

            break;

        default:

            break;
        }
    } else if (key[0] == 27 && key[1] == 91 && key[2] == 68) {

        // LEFT ARROW

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            switch (mdgui->potential_component) {

            case MDGUI__NONE:

                mdgui->potential_component = MDGUI__FILEBOX;
                MDGUI__highlight_box (&mdgui->filebox.listbox.box);
                break;

            case MDGUI__LOGO:

                mdgui->potential_component = MDGUI__FILEBOX;
                MDGUI__draw_logo (mdgui);
                MDGUI__highlight_box (&mdgui->filebox.listbox.box);
                break;

            case MDGUI__METABOX:

                mdgui->potential_component = MDGUI__FILEBOX;

                MDGUI__unhighlight_box (&mdgui->metabox.box);
                MDGUI__highlight_box (&mdgui->filebox.listbox.box);

                break;

            case MDGUI__PLAYLIST:

                mdgui->potential_component = MDGUI__METABOX;

                MDGUI__unhighlight_box (&mdgui->playlistbox.listbox.box);
                MDGUI__highlight_box (&mdgui->metabox.box);
                break;

            default:

                break;
            }

            break;

        default:

            break;
        }
    }
    else if ((key[0] == 'p' || key[0] == 'P') && key[1] == 0 && key[2] == 0) {

        // PAUSE

        if (mdgui->current_play_state == MDGUI__PLAYING || mdgui->current_play_state == MDGUI__PAUSE) {

            MD__toggle_pause (mdgui->curr_playing);
            mdgui->current_play_state = MD__is_paused (mdgui->curr_playing) ? MDGUI__PAUSE : MDGUI__PLAYING;

            MDGUI__log (mdgui->current_play_state == MDGUI__PAUSE ? "Playing paused." : "Playing resumed.", mdgui->tinfo);
        }
    }
    else if ((key[0] == 's' || key[0] == 'S') && key[1] == 0 && key[2] == 0) {

        // STOP

        if (mdgui->current_play_state == MDGUI__PLAYING || mdgui->current_play_state == MDGUI__PAUSE) {

            mdgui->current_play_state = MDGUI__WAITING_TO_STOP;

            mdgui->stop_all_signal = true;

            MD__stop (mdgui->curr_playing);

            MDGUI__log ("Waiting to stop.", mdgui->tinfo);
        }
    }
    return true;
}

void MDGUI__update_size (MDGUI__manager_t *mdgui) {

    int box_width = MDGUI__get_box_width (mdgui);
    int box_height = MDGUI__get_box_height (mdgui);

    mdgui->filebox.listbox.box.x = MDGUI__get_box_x (mdgui, 0);
    mdgui->filebox.listbox.box.height = box_height;
    mdgui->filebox.listbox.box.width = box_width;

    mdgui->metabox.box.x = MDGUI__get_box_x (mdgui, 1);
    mdgui->metabox.box.height = box_height - mdgui->meta_top;
    mdgui->metabox.box.width = box_width;

    mdgui->playlistbox.listbox.box.x = MDGUI__get_box_x (mdgui, 2);
    mdgui->playlistbox.listbox.box.height = box_height;
    mdgui->playlistbox.listbox.box.width = box_width;
}

void *terminal_change (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__terminal previous_tinfo = mdgui->tinfo;

    struct timespec tsp = {0,10000};

    while (true) {

        nanosleep(&tsp, NULL);

        MDGUI__mutex_lock (mdgui);

        if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

            MDGUI__mutex_unlock (mdgui);

            break;
        }

        mdgui->tinfo = MDGUI__get_terminal_information ();

        if (mdgui->tinfo.cols != previous_tinfo.cols || mdgui->tinfo.lines != previous_tinfo.lines) {

            MDGUI__update_size (mdgui);

            if (mdgui->filebox.listbox.box.height >= mdgui->filebox.listbox.str_array.cnum) mdgui->filebox.listbox.num_first = 0;

            if (mdgui->playlistbox.listbox.box.height >= mdgui->playlistbox.listbox.str_array.cnum) mdgui->playlistbox.listbox.num_first = 0;

            clear();

            MDGUI__draw (mdgui);

            unsigned int tinfo_size = snprintf (NULL, 0, "Changed size to %d x %d.", mdgui->tinfo.cols, mdgui->tinfo.lines) + 1;
            char *tinfo_string = malloc (sizeof (*tinfo_string) * tinfo_size);
            snprintf (tinfo_string, tinfo_size, "Changed size to %d x %d.", mdgui->tinfo.cols, mdgui->tinfo.lines);

            MDGUI__log (tinfo_string, mdgui->tinfo);

            previous_tinfo = mdgui->tinfo;
        }

        MDGUI__mutex_unlock (mdgui);
    }

    return NULL;
}

void MDGUI__deinit (MDGUI__manager_t *mdgui) {

    MDGUI__mutex_lock (mdgui);

    if (mdgui->current_play_state == MDGUI__PLAYING || mdgui->current_play_state == MDGUI__PAUSE) {

        mdgui->current_play_state = MDGUI__PROGRAM_EXIT;

        MDGUI__mutex_unlock (mdgui);

        MD__stop (mdgui->curr_playing);

        for (;;) {
            MDGUI__mutex_lock (mdgui);
            if (mdgui->current_play_state == MDGUI__READY_TO_EXIT) {

                MDGUI__mutex_unlock (mdgui);
                break;
            }
            MDGUI__mutex_unlock (mdgui);
        }
    }
    else {

        mdgui->current_play_state = MDGUI__PROGRAM_EXIT;
        MDGUI__mutex_unlock (mdgui);
    }


    MDAL__close();
}

void MDGUI__draw_logo (MDGUI__manager_t *mdgui) {

    char logo [76];
    int line = 0;
    int col = 0;

    MD__get_logo (logo);

    if (mdgui->potential_component == MDGUI__LOGO) attron (A_BOLD);

    bool reversing = false;
    int lastrev = -1;

    for (int i=0; i<75; i++) {

        if (logo[i] == '\n') {

            if (line == 4 && mdgui->potential_component == MDGUI__LOGO) attroff (A_BOLD);
            if (line == 5 && mdgui->potential_component == MDGUI__LOGO) attron (A_BOLD);
            line++;

            col = 0;
            continue;
        }

        if (logo[i] == '~' && mdgui->selected_component == MDGUI__LOGO && !reversing) {
            reversing = true;
            lastrev = i;
            attron (A_REVERSE);
        }

        mvprintw (1 + line, mdgui->metabox.box.x + (mdgui->metabox.box.width - 12) / 2 + col++, "%c", logo[i]);

        if (logo[i] == '~' && i != lastrev && mdgui->selected_component == MDGUI__LOGO && reversing) {
            reversing = false;
            attroff (A_REVERSE);
        }

    }

    if (mdgui->potential_component == MDGUI__LOGO) attroff (A_BOLD);

    refresh ();
}
