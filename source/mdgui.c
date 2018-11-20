#include "mdgui.h"

#include <melodeer/mdflac.h>
#include <melodeer/mdwav.h>
#include <melodeer/mdmpg123.h>
#include <melodeer/mdutils.h>

void        MDGUI__wait_for_keypress        (MDGUI__manager_t *mdgui,
                                             bool (key_pressed) (MDGUI__manager_t *mdgui, char [3]),
                                             void (on_completion) (MDGUI__manager_t *mdgui));
bool        key_pressed                     (MDGUI__manager_t *mdgui, char key[3]);
void        *terminal_change                (void *data);
void        MDGUI__draw_logo                (MDGUI__manager_t *mdgui);
void        MDGUI__start_playing            (MDGUI__manager_t *mdgui);
bool        MDGUI__stop_all_playing         (MDGUI__manager_t *mdgui);
void        MDGUI__complete                 (MDGUI__manager_t *mdgui);

void        MDGUIMB__transform              (volatile MD__buffer_chunk_t *curr_chunk,
                                             unsigned int sample_rate,
                                             unsigned int channels,
                                             unsigned int bps, void *user_data);

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
    mdgui->meta_bottom = 1;

    pthread_mutex_init (&mdgui->mutex, NULL);

    mdgui->tinfo = MDGUI__get_terminal_information ();

    mdgui->selected_component = MDGUI__NONE;
    mdgui->potential_component = MDGUI__NONE;
    mdgui->previous_potential_component = MDGUI__NONE;

    mdgui->stop_all_signal = false;

    int box_width = MDGUI__get_box_width (mdgui);
    int box_height = MDGUI__get_box_height (mdgui);

    mdgui->filebox = MDGUIFB__create ("files", MDGUI__get_box_x (mdgui, 0), mdgui->top, box_height, box_width);

    mdgui->metabox = MDGUIMB__create ("metadata", MDGUI__get_box_x (mdgui, 1) + 1, mdgui->top + mdgui->meta_top, box_height - mdgui->meta_bottom - mdgui->meta_top, box_width - 2);

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

    #ifdef MDGUI_DEBUG

        FILE *f = fopen ("mdgui.log", "a");

        if (f == NULL) return;

        time_t t = time (NULL);
        struct tm tm = *localtime (&t);

        fprintf (f, "[%02d.%02d.%04d. %02d:%02d:%02d] : %s\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
                                                               tm.tm_hour, tm.tm_min, tm.tm_sec, log);
        fclose (f);

    #endif
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

        int readn = select (1, &input_set, NULL, NULL, NULL);

        if (FD_ISSET (STDIN_FILENO, &input_set)) {

            int buffread = read(STDIN_FILENO, buff, 3);

            if (!key_pressed (mdgui, buff)) break;
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

    MDGUI__log ("Done playing!", mdgui->tinfo);

    MDGUIMB__unload (&mdgui->metabox);

    MDGUI__log ("Unloaded metabox.", mdgui->tinfo);

    MD__file_t *to_free = mdgui->curr_playing;
    mdgui->curr_playing = NULL;
    free (to_free);

    MDGUI__log ("Freed current MD__file.", mdgui->tinfo);

    if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

        mdgui->current_play_state = MDGUI__READY_TO_EXIT;

        MDGUI__mutex_unlock (mdgui);

        return;
    }

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

        MDGUI__start_playing (user_data);

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

    MDGUIMB__start_countdown (&mdgui->metabox);

    char buff[PATH_MAX + 10];

    snprintf (buff, PATH_MAX + 10, "Playing: %s", MDGUIPB__get_curr_filename (&mdgui->playlistbox));

    MDGUI__log (buff, mdgui->tinfo);

    MDGUI__mutex_unlock (mdgui);

    return;
}

void MDGUI__buff_underrun (void *user_data) {

    static unsigned int num_underrun = 0;

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    if (num_underrun > 5) {

        num_underrun = 0;

        MDAL__buff_resize (mdgui->curr_playing, NULL);
    }
    else num_underrun++;
}

void *MDGUI__play (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__mutex_lock (mdgui);

    MD__file_t *current_file = malloc (sizeof(*current_file));

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

    if (MD__initialize_with_user_data (current_file, filename, data)) {

        mdgui->curr_playing = current_file;

        mdgui->curr_playing->MD__buffer_transform = MDGUIMB__transform;

        MDGUI__log ("File initalized.", mdgui->tinfo);

        MDGUI__mutex_unlock (mdgui);

        if (!MD__play_raw_with_decoder (current_file, MD__handle_metadata, MDGUI__started_playing,
                                        MDGUI__handle_error, MDGUI__buff_underrun, MDGUI__play_complete)) {

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

bool filebox_or_playlist_return (MDGUI__manager_t *mdgui) {

    MDGUI__mutex_lock (mdgui);

    if (!(mdgui->current_play_state == MDGUI__PAUSE
       || mdgui->current_play_state == MDGUI__PLAYING
       || mdgui->current_play_state == MDGUI__NOT_PLAYING)) {

        MDGUI__mutex_unlock (mdgui);

        return false;
    }

    if (mdgui->current_play_state != MDGUI__NOT_PLAYING) {

        mdgui->current_play_state = MDGUI__WAITING_TO_STOP;

        mdgui->stop_all_signal = true;
        MDGUI__log ("Waiting to stop.", mdgui->tinfo);

        MD__stop (mdgui->curr_playing);

        MDGUI__log ("Stop signal sent.", mdgui->tinfo);
    }
    else MDGUI__log ("Not necessary to stop.", mdgui->tinfo);

    return true;
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

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__deselect_box (&mdgui->filebox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__PLAYLIST:
             
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__deselect_box (&mdgui->playlistbox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__METABOX:
                
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__deselect_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__LOGO:

                mdgui->selected_component = MDGUI__NONE;
                mdgui->potential_component = MDGUI__LOGO;
           
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

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

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__select_box (&mdgui->filebox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);
                
                break;

            case MDGUI__PLAYLIST:

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__select_box (&mdgui->playlistbox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__METABOX:
            
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__select_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__LOGO:
           
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            default:
                break;
            }

            break;

        case MDGUI__FILEBOX:

            if (MDGUIFB__return (&mdgui->filebox)) {

                if (!filebox_or_playlist_return(mdgui)) {

                    MDGUI__mutex_unlock (mdgui);
                    break;
                }

                struct MDGUI__prepend_info pr_info;

                pr_info.curr_dir = NULL;

                MDGUIFB__serialize_curr_dir (&mdgui->filebox, &pr_info.curr_dir);
                pr_info.curr_dir_str_size = MDGUI__get_string_size (pr_info.curr_dir);

                MDGUI__log (pr_info.curr_dir, mdgui->tinfo);

                MDGUI__str_array_copy_raw (&mdgui->filebox.listbox.str_array,
                                           &mdgui->playlistbox.filenames,
                                            mdgui->filebox.listbox.num_selected,
                                            mdgui->filebox.listbox.str_array.cnum - 1,
                                           &pr_info,
                                            MDGUI__str_transform_prepend_dir);

                free (pr_info.curr_dir);

                mdgui->playlistbox.num_playing = 0;
                mdgui->playlistbox.listbox.num_selected = -1;

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUIPB__redraw (&mdgui->playlistbox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                if (mdgui->current_play_state == MDGUI__NOT_PLAYING) {

                    MDGUI__mutex_unlock (mdgui);
                    MDGUI__start_playing (mdgui);
                    break;
                }

                MDGUI__mutex_unlock (mdgui);

                break;
            }
            else MDGUI__mutex_unlock (mdgui);

            break;

        case MDGUI__PLAYLIST:

            if (!filebox_or_playlist_return (mdgui)) {

                MDGUI__mutex_unlock (mdgui);
                break;
            }

            mdgui->playlistbox.num_playing = mdgui->playlistbox.listbox.num_selected;

            pthread_mutex_lock (&mdgui->metabox.mutex);
            MDGUIPB__redraw (&mdgui->playlistbox);
            pthread_mutex_unlock (&mdgui->metabox.mutex);

            MDGUI__log ("Set new play item.", mdgui->tinfo);

            if (mdgui->current_play_state == MDGUI__NOT_PLAYING) {

                MDGUI__mutex_unlock (mdgui);
                MDGUI__start_playing (mdgui);
                break;
            }
            MDGUI__mutex_unlock (mdgui);

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

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                MDGUI__highlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);
            }
            else if (mdgui->potential_component == MDGUI__NONE) {

                mdgui->potential_component = MDGUI__METABOX;
    
                pthread_mutex_lock (&mdgui->metabox.mutex);            
                MDGUI__highlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);
            }

            break;

        case MDGUI__FILEBOX:

            pthread_mutex_lock (&mdgui->metabox.mutex);
            MDGUILB__down_arrow (&mdgui->filebox.listbox);
            MDGUIFB__redraw (&mdgui->filebox);
            pthread_mutex_unlock (&mdgui->metabox.mutex);

            break;

        case MDGUI__PLAYLIST:

            pthread_mutex_lock (&mdgui->metabox.mutex);
            MDGUILB__down_arrow (&mdgui->playlistbox.listbox);
            MDGUIPB__redraw (&mdgui->playlistbox);
            pthread_mutex_unlock (&mdgui->metabox.mutex);

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
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                MDGUI__unhighlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);
            }

            else if (mdgui->potential_component == MDGUI__NONE) {

                mdgui->potential_component = MDGUI__LOGO;

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                pthread_mutex_unlock (&mdgui->metabox.mutex);
            }

            break;

        case MDGUI__FILEBOX:

            pthread_mutex_lock (&mdgui->metabox.mutex);
            MDGUILB__up_arrow (&mdgui->filebox.listbox);
            MDGUIFB__redraw (&mdgui->filebox);
            pthread_mutex_unlock (&mdgui->metabox.mutex);
            
            break;

        case MDGUI__PLAYLIST:

            pthread_mutex_lock (&mdgui->metabox.mutex);
            MDGUILB__up_arrow (&mdgui->playlistbox.listbox);
            MDGUIPB__redraw (&mdgui->playlistbox);
            pthread_mutex_unlock (&mdgui->metabox.mutex);

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

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__FILEBOX:

                mdgui->potential_component = MDGUI__METABOX;
         
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__unhighlight_box (&mdgui->filebox.listbox.box);
                MDGUI__highlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__LOGO:

                mdgui->potential_component = MDGUI__PLAYLIST;

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__METABOX:

                mdgui->potential_component = MDGUI__PLAYLIST;
                
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__unhighlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

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

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__highlight_box (&mdgui->filebox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__LOGO:

                mdgui->potential_component = MDGUI__FILEBOX;

                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__draw_logo (mdgui);
                MDGUI__highlight_box (&mdgui->filebox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__METABOX:

                mdgui->potential_component = MDGUI__FILEBOX;
           
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__unhighlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                MDGUI__highlight_box (&mdgui->filebox.listbox.box);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

                break;

            case MDGUI__PLAYLIST:

                mdgui->potential_component = MDGUI__METABOX;
            
                pthread_mutex_lock (&mdgui->metabox.mutex);
                MDGUI__unhighlight_box (&mdgui->playlistbox.listbox.box);
                MDGUI__highlight_box (&mdgui->metabox.box);
                MDGUIMB__draw (&mdgui->metabox);
                pthread_mutex_unlock (&mdgui->metabox.mutex);

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

        if (mdgui->current_play_state == MDGUI__PLAYING
         || mdgui->current_play_state == MDGUI__PAUSE) {

            MD__toggle_pause (mdgui->curr_playing);

            mdgui->current_play_state = MD__is_paused (mdgui->curr_playing)
                                      ? MDGUI__PAUSE
                                      : MDGUI__PLAYING;

            if (mdgui->current_play_state == MDGUI__PAUSE) MDGUIMB__set_pause (&mdgui->metabox);
            else MDGUIMB__unset_pause (&mdgui->metabox);

            MDGUI__log (mdgui->current_play_state == MDGUI__PAUSE
                        ? "Playing paused."
                        : "Playing resumed.", mdgui->tinfo);

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

    mdgui->metabox.box.x = MDGUI__get_box_x (mdgui, 1) + 1;
    mdgui->metabox.box.height = box_height - mdgui->meta_bottom - mdgui->meta_top;
    mdgui->metabox.box.width = box_width - 2;

    mdgui->playlistbox.listbox.box.x = MDGUI__get_box_x (mdgui, 2);
    mdgui->playlistbox.listbox.box.height = box_height;
    mdgui->playlistbox.listbox.box.width = box_width;
}

void *terminal_change (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__terminal previous_tinfo = mdgui->tinfo;

    struct timespec tsp = {0, 10000};

    while (true) {

        nanosleep(&tsp, NULL);

        MDGUI__mutex_lock (mdgui);
        pthread_mutex_lock (&mdgui->metabox.mutex);

        if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

            MDGUI__mutex_unlock (mdgui);

            pthread_mutex_unlock (&mdgui->metabox.mutex);

            break;
        }

        mdgui->tinfo = MDGUI__get_terminal_information ();

        if (mdgui->tinfo.cols != previous_tinfo.cols
         || mdgui->tinfo.lines != previous_tinfo.lines) {

            MDGUI__update_size (mdgui);

            clear();

            MDGUI__draw (mdgui);

            unsigned int tinfo_size = snprintf (NULL, 0, "Changed size to %d x %d.",
                                                mdgui->tinfo.cols, mdgui->tinfo.lines) + 1;
            char *tinfo_string = malloc (sizeof (*tinfo_string) * tinfo_size);
            snprintf (tinfo_string, tinfo_size, "Changed size to %d x %d.", mdgui->tinfo.cols,
                      mdgui->tinfo.lines);

            MDGUI__log (tinfo_string, mdgui->tinfo);
            if (tinfo_string) free (tinfo_string);

            previous_tinfo = mdgui->tinfo;
        }

        MDGUI__mutex_unlock (mdgui);

        pthread_mutex_unlock (&mdgui->metabox.mutex);
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

    if (mdgui->current_play_state == MDGUI__PLAYING
     || mdgui->current_play_state == MDGUI__PAUSE) {

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

void MDGUIMB__transform (volatile MD__buffer_chunk_t *curr_chunk,
                         unsigned int sample_rate,
                         unsigned int channels,
                         unsigned int bps,
                         void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    static float complex buffer[4096];
    static float complex output[4096];

    int count = curr_chunk->size/((bps/8)*channels);

    for (int i=0; i<count; i++) {

        float mono_mix = 0;

        for (int c=0; c<channels; c++) {

            // this will depend on bps...
            short data = 0;

            for (int b=0; b<bps/8; b++)

                data = data + ((short)(curr_chunk->chunk[i * channels * (bps/8) + (c*channels) + b]) << (8*b));

            mono_mix += (((float)data) / SHRT_MAX)/channels;
        }

        buffer[i] = 0*I + mono_mix;
    }

    int new_count = count / 2;

    float coeff = 100000 / new_count;

    for (int i=0; i<2; i++) {

        MDFFT__apply_hanning (&(buffer[new_count*i]), new_count);

        MDFFT__iterative (false, &(buffer[new_count*i]), output, new_count);

        float *new_sample = malloc (sizeof (*new_sample) * 8);

        MDFFT__to_amp_surj (output, new_count / 2, new_sample, 8);

        for (int j=0; j<8; j++) {

            float new_j = log10f(coeff * new_sample[j]);
            // try 0.5 + (cos(2 * 3.14 * (new_j / 4) + 3.14)/4)
            new_sample[j] = new_j < 0 ? 0 : new_j / 4;
        };

        float secs = ((((float)curr_chunk->order) + i * 1/2) * curr_chunk->size / (bps * channels / 8)) / sample_rate;

        MDGUIMB__fft_queue (&mdgui->metabox, new_sample, secs);
    }
}
