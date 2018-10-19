#include "mdgui.h"

#include <melodeer/mdflac.h>
#include <melodeer/mdwav.h>
#include <melodeer/mdmpg123.h>
#include <melodeer/mdutils.h>

void MDGUI__wait_for_keypress (MDGUI__manager_t *mdgui, bool (key_pressed)(MDGUI__manager_t *mdgui, char [3]), void (on_completion)(MDGUI__manager_t *mdgui));
void MDGUI__complete (MDGUI__manager_t *mdgui);
bool key_pressed (MDGUI__manager_t *mdgui, char key[3]);
void *terminal_change (void *data);
void MDGUI__draw_logo (MDGUI__manager_t *mdgui);
void MDGUI__start_playing (MDGUI__manager_t *mdgui);
bool MDGUI__stop_all_playing (MDGUI__manager_t *mdgui);

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

    return true;
}

bool MDGUI__start (MDGUI__manager_t *mdgui) {

    MDAL__initialize (4096, 4, 4);

    initscr ();

    clear ();

    curs_set (0);

    MDGUI__draw (mdgui);

    MDGUI__log ("Use arrow keys to move, ENTER to select and ESC to deselect/exit.", mdgui->tinfo);

    if (pthread_create (&mdgui->terminal_thread, NULL, terminal_change, mdgui)) {

        return false;
    }

    MDGUI__wait_for_keypress (mdgui, key_pressed, MDGUI__complete);

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

    FILE *f = fopen ("mdgui.log", "a");

    if (f == NULL) return;

    time_t t = time (NULL);
    struct tm tm = *localtime (&t);

    fprintf (f, "[%02d.%02d.%04d. %02d:%02d:%02d] : %s\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
                                                           tm.tm_hour, tm.tm_min, tm.tm_sec, log);

    fclose (f);

    // move (tinfo.lines - 1, 0);
    // clrtoeol ();

    // int i = 0;

    // while (true) {

    //     if (i >= tinfo.cols) break;

    //     if (log[i] == 0) break;

    //     mvprintw (tinfo.lines - 1, i, "%c", log[i]);

    //     i++;
    // }

    // refresh ();
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

    if (dir_info->curr_dir[dir_info->curr_dir_str_size - 1] == 0) dir_info->curr_dir_str_size--;

    bool needs_slash = dir_info->curr_dir[dir_info->curr_dir_str_size - 1] != '/';

    int src_str_size = MDGUI__get_string_size (src);

    int new_size = dir_info->curr_dir_str_size + src_str_size + (needs_slash ? 1 : 0);

    *dest = malloc (sizeof (**dest) * new_size);

    for (int i=0; i<dir_info->curr_dir_str_size; i++) (*dest)[i] = dir_info->curr_dir[i];

    if (needs_slash) (*dest)[dir_info->curr_dir_str_size] = '/';

    int start = needs_slash ? 1 : 0;

    int end = src_str_size + (needs_slash ? 1 : 0);

    for (int i=start; i<end; i++) (*dest)[dir_info->curr_dir_str_size+i] = src[i-start + 1];

    return;
}

void MDGUI__wait_for_keypress (MDGUI__manager_t *mdgui, bool (key_pressed)(MDGUI__manager_t *mdgui, char [3]), void (on_completion)(MDGUI__manager_t *mdgui)) {

    struct termios orig_term_attr;
    struct termios new_term_attr;

    char chain [3];

    tcgetattr (fileno (stdin), &orig_term_attr);
    memcpy (&new_term_attr, &orig_term_attr, sizeof (struct termios));
    new_term_attr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr (fileno (stdin), TCSANOW, &new_term_attr);

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

    return;
}

void MD__handle_metadata (MD__metadata_t metadata, void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUIMB__load (&mdgui->metabox, metadata);

    return;
}

void MDGUI__play_complete (void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__mutex_lock (mdgui);

    MDGUI__log ("Play complete called.", mdgui->tinfo);

    if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

        mdgui->current_play_state = MDGUI__READY_TO_EXIT;

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    MDGUI__log ("Done playing!", mdgui->tinfo);

    MDGUIMB__unload (&mdgui->metabox);

    if ((mdgui->playlistbox.num_playing < mdgui->playlistbox.filenames.cnum) && !mdgui->stop_all_signal) {

        mdgui->playlistbox.num_playing++;

        MDGUI__log ("Playing next.", mdgui->tinfo);
    }
    else if (mdgui->playlistbox.num_playing >= mdgui->playlistbox.filenames.cnum
          || mdgui->playlistbox.num_playing < 0) {

        if (mdgui->stop_all_signal) mdgui->stop_all_signal = false;

        MDGUI__log ("Stopping playlist completely.", mdgui->tinfo);

        mdgui->playlistbox.num_playing = -1;

        MDGUIPB__redraw (&mdgui->playlistbox);

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    MDGUIPB__redraw (&mdgui->playlistbox);

    if (mdgui->stop_all_signal) {

        mdgui->stop_all_signal = false;

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    MDGUI__log ("Starting to play again.", mdgui->tinfo);

    MDGUI__mutex_unlock (mdgui);

    MDGUI__start_playing (user_data);

    return;
}

void MDGUI__handle_error (char *error, void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__mutex_lock (mdgui);

    MDGUI__log (error, mdgui->tinfo);

    MDGUI__mutex_unlock (mdgui);

    return;
}

void MDGUI__started_playing (void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__mutex_lock (mdgui);

    mdgui->current_play_state = MDGUI__PLAYING;

    char buff[PATH_MAX + 10];

    snprintf (buff, PATH_MAX + 10, "Playing: %s", MDGUIPB__get_curr_filename (&mdgui->playlistbox));

    MDGUI__log (buff, mdgui->tinfo);

    MDGUI__mutex_unlock (mdgui);

    return;
}

void *MDGUI__play (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__mutex_lock (mdgui);

    MD__file_t current_file;

    MDGUI__log ("Query filename.", mdgui->tinfo);

    char *filename_temp = MDGUIPB__get_curr_filename (&mdgui->playlistbox);

    if (!filename_temp) {

        MDGUI__log ("No filename.", mdgui->tinfo);

        MDGUI__mutex_unlock (mdgui);

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        return NULL;
    }

    MDGUI__log ("Copying filename.", mdgui->tinfo);

    int filename_size = MDGUI__get_string_size (filename_temp) + 1;

    char *filename = malloc (filename_size * sizeof (*filename));

    for (int i=0; i<filename_size; i++) filename[i] = filename_temp[i];

    MDGUI__log ("Got filename.", mdgui->tinfo);

    if (MD__initialize_with_user_data (&current_file, filename, data)) {

        mdgui->curr_playing = &current_file;

        MDGUI__log ("File initalized.", mdgui->tinfo);

        MDGUI__mutex_unlock (mdgui);

        if (!MD__play_raw_with_decoder (&current_file, MD__handle_metadata, MDGUI__started_playing,
                                        MDGUI__handle_error, NULL, MDGUI__play_complete)) {
            
            MDGUI__mutex_lock (mdgui);

            MDGUI__log ("(!) Unknown file type!",mdgui->tinfo);

            mdgui->current_play_state = MDGUI__NOT_PLAYING;
            mdgui->curr_playing = NULL;

            free (filename);

            MDGUI__mutex_unlock (mdgui);

            return NULL;
        }

    } else {

        MDGUI__log ("(!) Could not open file!", mdgui->tinfo);

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        free (filename);

        MDGUI__mutex_unlock (mdgui);

        return NULL;
    }

    mdgui->curr_playing = NULL;

    free (filename);

    MDGUI__mutex_unlock (mdgui);

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

            MDGUI__mutex_lock (mdgui);

            if (MDGUIFB__return (&mdgui->filebox)) {

                if (mdgui->current_play_state == MDGUI__WAITING_TO_STOP
                 || mdgui->current_play_state == MDGUI__INITIALIZING
                 || mdgui->current_play_state == MDGUI__EVALUATING) {

                    MDGUI__mutex_unlock (mdgui);

                    break;
                }

                mdgui->current_play_state = MDGUI__EVALUATING;

                struct MDGUI__prepend_info pr_info;

                pr_info.curr_dir = mdgui->filebox.curr_dir;
                pr_info.curr_dir_str_size = MDGUI__get_string_size (mdgui->filebox.curr_dir);

                MDGUI__str_array_copy_raw (&mdgui->filebox.listbox.str_array, &mdgui->playlistbox.filenames, mdgui->filebox.listbox.num_selected, mdgui->filebox.listbox.str_array.cnum - 1, &pr_info, MDGUI__str_transform_prepend_dir);

                mdgui->playlistbox.num_playing = 0;
                mdgui->playlistbox.listbox.num_selected = -1;

                MDGUIPB__redraw (&mdgui->playlistbox);

                MDGUI__mutex_unlock (mdgui);

                if (MDGUI__stop_all_playing (mdgui))

                    MDGUI__start_playing (mdgui);

                break;
            }
            else MDGUI__mutex_unlock (mdgui);

            break;

        case MDGUI__PLAYLIST:

            MDGUI__mutex_lock (mdgui);
            if (mdgui->current_play_state == MDGUI__WAITING_TO_STOP
             || mdgui->current_play_state == MDGUI__INITIALIZING
             || mdgui->current_play_state == MDGUI__EVALUATING) {

                MDGUI__log ("Can't play right now.", mdgui->tinfo);

                MDGUI__mutex_unlock (mdgui);

                break;
            }

            mdgui->current_play_state = MDGUI__EVALUATING;

            mdgui->playlistbox.num_playing = mdgui->playlistbox.listbox.num_selected;

            MDGUIPB__redraw (&mdgui->playlistbox);

            MDGUI__log ("Set new play item.", mdgui->tinfo);

            MDGUI__mutex_unlock (mdgui);

            if (MDGUI__stop_all_playing (mdgui)) {

                MDGUI__start_playing (mdgui);
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

        MDGUI__mutex_lock (mdgui);

        if (mdgui->current_play_state == MDGUI__PLAYING || mdgui->current_play_state == MDGUI__PAUSE) {

            MD__toggle_pause (mdgui->curr_playing);

            mdgui->current_play_state = MD__is_paused (mdgui->curr_playing) ? MDGUI__PAUSE : MDGUI__PLAYING;

            MDGUI__log (mdgui->current_play_state == MDGUI__PAUSE ? "Playing paused." : "Playing resumed.", mdgui->tinfo);
        }

        MDGUI__mutex_unlock (mdgui);
    }
    else if ((key[0] == 's' || key[0] == 'S') && key[1] == 0 && key[2] == 0) {

        // STOP

        MDGUI__mutex_lock (mdgui);

        mdgui->playlistbox.num_playing = -1;

        MDGUI__mutex_unlock (mdgui);

        bool stop = MDGUI__stop_all_playing (mdgui);

    }
    return true;
}

bool MDGUI__stop_all_playing (MDGUI__manager_t *mdgui) {

    MDGUI__mutex_lock (mdgui);

    if (mdgui->current_play_state == MDGUI__PLAYING
     || mdgui->current_play_state == MDGUI__PAUSE) {

        mdgui->current_play_state = MDGUI__WAITING_TO_STOP;

        mdgui->stop_all_signal = true;

        MDGUI__log ("Waiting to stop.", mdgui->tinfo);

        MD__stop (mdgui->curr_playing);

        MDGUI__log ("Stop signal sent.", mdgui->tinfo);

        MDGUI__mutex_unlock (mdgui);

        return false;
    }

    MDGUI__log ("Not necessary to stop.", mdgui->tinfo);

    MDGUI__mutex_unlock (mdgui);

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

void MDGUI__start_playing (MDGUI__manager_t *mdgui) {

    MDGUI__mutex_lock (mdgui);

    if (mdgui->playlistbox.num_playing < 0
       || mdgui->playlistbox.num_playing >= mdgui->playlistbox.filenames.cnum) {

        mdgui->playlistbox.num_playing = -1;

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        MDGUI__log ("Can't play - out of bounds.", mdgui->tinfo);

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    mdgui->current_play_state = MDGUI__INITIALIZING;

    MDGUI__mutex_unlock (mdgui);

    pthread_t melodeer_thread;

    MDGUI__log ("Will play.", mdgui->tinfo);

    if (pthread_create (&mdgui->melodeer_thread, NULL, MDGUI__play, (void *)mdgui))

        MDGUI__log ("(!) Could not create thread!", mdgui->tinfo);

    return;
}

void MDGUI__complete (MDGUI__manager_t *mdgui) {

    MDGUI__mutex_lock (mdgui);

    if (mdgui->current_play_state == MDGUI__PLAYING || mdgui->current_play_state == MDGUI__PAUSE) {

        mdgui->current_play_state = MDGUI__PROGRAM_EXIT;

        MD__stop (mdgui->curr_playing);

        MDGUI__mutex_unlock (mdgui);

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

void MDGUI__deinit (MDGUI__manager_t *mdgui) {

    pthread_mutex_destroy (&mdgui->mutex);

    MDGUIMB__deinit (&mdgui->metabox);
    MDGUIPB__deinit (&mdgui->playlistbox);
    MDGUIFB__deinit (&mdgui->filebox);
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
