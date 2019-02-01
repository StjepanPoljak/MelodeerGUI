#include "mdgui.h"

#include <sys/time.h>

#include <melodeer/mdflac.h>
#include <melodeer/mdwav.h>
#include <melodeer/mdmpg123.h>
#include <melodeer/mdutils.h>

void        *MDGUI__wait_for_keypress       (void *data);
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

void        MDGUI__add_event                (MDGUI__manager_t *mdgui, bool (*new_f)(void *), void *data);
bool        MDGUI__exec_last_event          (MDGUI__manager_t *mdgui);
void        MDGUI__init_event_queue         (MDGUI__manager_t *mdgui);

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

    mdgui->refresh_rate = 50000;
    mdgui->max_events = 1;

    mdgui->top = 2;
    mdgui->bottom = 3;
    mdgui->left = 2;
    mdgui->right = 2;
    mdgui->meta_top = 7;
    mdgui->meta_bottom = 1;

    pthread_mutexattr_init (&mdgui->mutex_attributes);

    pthread_mutexattr_settype (&mdgui->mutex_attributes, PTHREAD_MUTEX_RECURSIVE);

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

    MDGUI__init_event_queue (mdgui);

    return true;
}

struct MDGUI__keypress_data {
    MDGUI__manager_t *mdgui;
    bool (*key_pressed) (MDGUI__manager_t *mdgui, char [3]);
    void (*on_completion) (MDGUI__manager_t *mdgui);
};

bool MDGUI__start (MDGUI__manager_t *mdgui) {

    MDAL__initialize (4096, 4, 4);

    clear ();

    curs_set (0);

    struct termios orig_term_attr;
    struct termios new_term_attr;

    char chain [3];

    tcgetattr (fileno (stdin), &orig_term_attr);
    memcpy (&new_term_attr, &orig_term_attr, sizeof (struct termios));
    new_term_attr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr (fileno (stdin), TCSANOW, &new_term_attr);

    MDGUI__draw (mdgui);

    MDGUI__log ("Use arrow keys to move, ENTER to select and ESC to deselect/exit.", mdgui);

    if (pthread_create (&mdgui->terminal_thread, NULL, terminal_change, mdgui)) {

        return false;
    }

    struct MDGUI__keypress_data *keypress_data = malloc(sizeof(*keypress_data));
    keypress_data->mdgui = mdgui;
    keypress_data->key_pressed = key_pressed;
    keypress_data->on_completion = MDGUI__complete;

    if (pthread_create (&mdgui->keyboard_input_thread, NULL, MDGUI__wait_for_keypress, keypress_data)) {

        return false;
    }

    double start, end;
    struct timeval timecheck;

    while (true) {

        gettimeofday (&timecheck, NULL);
        start = (double)timecheck.tv_sec + (double)timecheck.tv_usec / 1e6;

        usleep (mdgui->refresh_rate);

        MDGUI__mutex_lock (mdgui);

        if (!MDGUI__exec_last_event (mdgui)) {

            MDGUI__mutex_unlock (mdgui);

            break;
        }

        if (mdgui->metabox.metadata_present && mdgui->current_play_state != MDGUI__PAUSE && mdgui->current_play_state != MDGUI__NOT_PLAYING && mdgui->current_play_state != MDGUI__INITIALIZING) {

            MDGUIMB__draw_time (&mdgui->metabox);
            MDGUIMB__draw_fft (&mdgui->metabox);
            MDGUIMB__draw_progress_bar (&mdgui->metabox);

            refresh ();

            gettimeofday (&timecheck, NULL);
            end = (double)timecheck.tv_sec + (double)timecheck.tv_usec / 1e6;

            mdgui->metabox.curr_sec += end - start;
        }
        else refresh ();

        MDGUI__mutex_unlock (mdgui);
    }

    if (mdgui->curr_playing) MD__stop (mdgui->curr_playing);

    else {

        MDGUI__mutex_lock (mdgui);

        mdgui->current_play_state = MDGUI__PROGRAM_EXIT;

        MDGUI__mutex_unlock (mdgui);
    }

    pthread_join (mdgui->keyboard_input_thread, NULL);

    pthread_join (mdgui->terminal_thread, NULL);

    tcsetattr (fileno (stdin), TCSANOW, &orig_term_attr);

    curs_set (1);

    clear ();

    return true;
}

void MDGUI__set_refresh_rate (MDGUI__manager_t *mdgui, int refresh_rate) {

    mdgui->refresh_rate = refresh_rate;
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
}

void MDGUI__init_event_queue (MDGUI__manager_t *mdgui) {

    mdgui->event_queue = NULL;
    mdgui->event_queue_last = -1;
    mdgui->event_queue_size = 0;
}

void MDGUI__add_event_raw (MDGUI__manager_t *mdgui, bool (*new_f)(void *), void *data) {

    if (mdgui->max_events > 0 && mdgui->max_events <= mdgui->event_queue_last + 1) {
        return;
    }

    MDGUI__event_t **old_queue = NULL;

    if (mdgui->event_queue_last >= 0) {

        old_queue = malloc(sizeof(*old_queue) * (mdgui->event_queue_last + 1));

        for (int i=0; i<=mdgui->event_queue_last; i++) {
            old_queue[i] = mdgui->event_queue[i];
        }
    }

    mdgui->event_queue_last++;

    if (mdgui->event_queue_last >= mdgui->event_queue_size) {
        mdgui->event_queue_size = mdgui->event_queue_size == 0 ? 1 : mdgui->event_queue_size * 2;

        if (mdgui->event_queue) mdgui->event_queue = realloc (mdgui->event_queue, sizeof (*mdgui->event_queue) * mdgui->event_queue_size);

        else mdgui->event_queue = malloc (sizeof (*mdgui->event_queue) * mdgui->event_queue_size);
    }

    if (old_queue != NULL) {

        for (int i=0; i<mdgui->event_queue_last; i++) {
            mdgui->event_queue[i] = old_queue[i];
        }

        free (old_queue);
    }

    MDGUI__event_t *new_event = malloc (sizeof (*new_event));
    new_event->event = new_f;
    new_event->data = data;

    mdgui->event_queue[mdgui->event_queue_last] = new_event;
}

void MDGUI__add_event (MDGUI__manager_t *mdgui, bool (*new_f)(void *), void *data) {

    MDGUI__mutex_lock (mdgui);
    MDGUI__add_event_raw (mdgui, new_f, data);
    MDGUI__mutex_unlock (mdgui);
}

bool MDGUI__exec_last_event (MDGUI__manager_t *mdgui) {

    bool return_value = true;

    if (mdgui->event_queue_last >= 0) {

        return_value = mdgui->event_queue[mdgui->event_queue_last]->event(mdgui->event_queue[mdgui->event_queue_last]->data);
        MDGUI__event_t *to_delete = mdgui->event_queue[mdgui->event_queue_last];

        free(to_delete);

        mdgui->event_queue_last--;
    }

    return return_value;
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

void MDGUI__log (const char *log, MDGUI__manager_t *mdgui) {

    MDGUI__terminal tinfo = mdgui->tinfo;

    move (tinfo.lines - 1, 0);
    clrtoeol ();

    int i = 0;

    while (true) {

        if (i >= tinfo.cols) break;

        if (log[i] == 0) break;

        mvprintw (tinfo.lines - 1, i, "%c", log[i]);

        i++;
    }

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

void *MDGUI__wait_for_keypress (void *data) {

    struct MDGUI__keypress_data *keypress_data = (struct MDGUI__keypress_data *)data;
    MDGUI__manager_t *mdgui = keypress_data->mdgui;
    bool (*key_pressed) (MDGUI__manager_t *mdgui, char [3]) = keypress_data->key_pressed;
    void (*on_completion) (MDGUI__manager_t *mdgui) = keypress_data->on_completion;

    fd_set input_set, output_set;

    for(;;) {

        usleep(mdgui->refresh_rate * 0.5);

        MDGUI__mutex_lock (mdgui);

        if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

            MDGUI__mutex_unlock (mdgui);

            break;
        }
        MDGUI__mutex_unlock (mdgui);


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

    return NULL;
}

void MDGUI__draw (MDGUI__manager_t *mdgui) {

    MDGUI__draw_logo (mdgui);
    MDGUIFB__draw (&mdgui->filebox);
    MDGUIPB__draw (&mdgui->playlistbox);
    MDGUIMB__draw (&mdgui->metabox);

    return;
}

bool MDGUI__handle_metadata_event (void *data) {

    MDGUI__meta_box_t *metabox = (MDGUI__meta_box_t *)data;

    MDGUIMB__redraw (metabox);

    return true;
}

void MD__handle_metadata (MD__metadata_t metadata, void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__mutex_lock (mdgui);

    MDGUIMB__load (&mdgui->metabox, metadata);

    MDGUI__add_event_raw (mdgui, MDGUI__handle_metadata_event, &mdgui->metabox);

    MDGUI__mutex_unlock (mdgui);

    return;
}

bool MDGUI__redraw_playlist_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUIPB__redraw (&mdgui->playlistbox);

    return true;
}

void MDGUI__play_complete (void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__mutex_lock (mdgui);

    MDGUI__log ("Done playing!", mdgui);

    MDGUIMB__unload (&mdgui->metabox);

    MDGUI__log ("Unloaded metabox.", mdgui);

    MD__file_t *to_free = mdgui->curr_playing;
    mdgui->curr_playing = NULL;
    free (to_free);

    MDGUI__log ("Freed current MD__file.", mdgui);

    if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

        mdgui->current_play_state = MDGUI__READY_TO_EXIT;

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    if ((mdgui->playlistbox.num_playing < mdgui->playlistbox.filenames.cnum) && !mdgui->stop_all_signal) {

        mdgui->playlistbox.num_playing++;

        MDGUI__log ("Playing next.", mdgui);
    }
    else if (mdgui->playlistbox.num_playing >= mdgui->playlistbox.filenames.cnum
          || mdgui->playlistbox.num_playing < 0) {

        if (mdgui->stop_all_signal) mdgui->stop_all_signal = false;

        MDGUI__log ("Stopping playlist completely.", mdgui);

        mdgui->playlistbox.num_playing = -1;

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        MDGUI__add_event_raw (mdgui, MDGUI__redraw_playlist_event, user_data);

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    if (mdgui->stop_all_signal) {

        mdgui->stop_all_signal = false;

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        MDGUI__start_playing (user_data);

        MDGUI__mutex_unlock (mdgui);

        return;
    }

    MDGUI__log ("Starting to play again.", mdgui);

    MDGUI__add_event_raw (mdgui, MDGUI__redraw_playlist_event, user_data);

    MDGUI__mutex_unlock (mdgui);

    MDGUI__start_playing (user_data);

    return;
}

void MDGUI__handle_error (char *error, void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__log (error, mdgui);

    return;
}

void MDGUI__started_playing (void *user_data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)user_data;

    MDGUI__mutex_lock (mdgui);

    mdgui->current_play_state = MDGUI__PLAYING;

    MDGUIMB__start_countdown (&mdgui->metabox);

    char buff[PATH_MAX + 10];

    snprintf (buff, PATH_MAX + 10, "Playing: %s", MDGUIPB__get_curr_filename (&mdgui->playlistbox));

    MDGUI__log (buff, mdgui);

    MDGUI__mutex_unlock (mdgui);

    return;
}

void MDGUI__buff_underrun (void *user_data) {

    return;
}

void *MDGUI__play (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__mutex_lock (mdgui);

    MD__file_t *current_file = malloc (sizeof(*current_file));

    MDGUI__log ("Query filename.", mdgui);

    char *filename_temp = MDGUIPB__get_curr_filename (&mdgui->playlistbox);

    if (!filename_temp) {

        MDGUI__log ("No filename.", mdgui);

        MDGUI__mutex_unlock (mdgui);

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        return NULL;
    }

    MDGUI__log ("Copying filename.", mdgui);

    int filename_size = MDGUI__get_string_size (filename_temp) + 1;

    char *filename = malloc (filename_size * sizeof (*filename));

    for (int i=0; i<filename_size; i++) filename[i] = filename_temp[i];

    MDGUI__log ("Got filename.", mdgui);

    if (MD__initialize_with_user_data (current_file, filename, data)) {

        mdgui->curr_playing = current_file;

        mdgui->curr_playing->MD__buffer_transform = MDGUIMB__transform;

        MDGUI__log ("File initalized.", mdgui);

        MDGUI__mutex_unlock (mdgui);

        if (!MD__play_raw_with_decoder (current_file, MD__handle_metadata, MDGUI__started_playing,
                                        MDGUI__handle_error, MDGUI__buff_underrun, MDGUI__play_complete)) {

            MDGUI__mutex_lock (mdgui);

            MDGUI__log ("(!) Unknown file type!",mdgui);

            mdgui->current_play_state = MDGUI__NOT_PLAYING;
            mdgui->curr_playing = NULL;

            free (filename);

            MDGUI__mutex_unlock (mdgui);

            return NULL;
        }

    } else {

        MDGUI__log ("(!) Could not open file!", mdgui);

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

bool MDGUI__escape_pressed_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    switch (mdgui->selected_component) {

        case MDGUI__NONE:

            // TODO: this is a hopeless case

            return true;

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
                MDGUIMB__draw (&mdgui->metabox);

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

    return true;
}

bool MDGUI__exit_event (void *data) {

    return false;
}

bool MDGUI__return_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

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
            MDGUIMB__draw (&mdgui->metabox);

            break;

        case MDGUI__LOGO:

            MDGUI__draw_logo (mdgui);

            break;

        default:
            break;
        }

        break;

    case MDGUI__FILEBOX:

        if (MDGUIFB__return (&mdgui->filebox)) {

            if (mdgui->curr_playing && mdgui->current_play_state != MDGUI__NOT_PLAYING) {

                if (mdgui->current_play_state != MDGUI__WAITING_TO_STOP) {
                    MD__stop_raw (mdgui->curr_playing);
                    mdgui->current_play_state = MDGUI__WAITING_TO_STOP;
                    mdgui->stop_all_signal = true;
                }
            }

            struct MDGUI__prepend_info pr_info;

            pr_info.curr_dir = NULL;

            MDGUIFB__serialize_curr_dir (&mdgui->filebox, &pr_info.curr_dir);
            pr_info.curr_dir_str_size = MDGUI__get_string_size (pr_info.curr_dir);

            MDGUI__log (pr_info.curr_dir, mdgui);

            MDGUI__str_array_copy_raw (&mdgui->filebox.listbox.str_array,
                                       &mdgui->playlistbox.filenames,
                                        mdgui->filebox.listbox.num_selected,
                                        mdgui->filebox.listbox.str_array.cnum - 1,
                                       &pr_info,
                                        MDGUI__str_transform_prepend_dir);

            free (pr_info.curr_dir);

            mdgui->playlistbox.num_playing = 0;
            mdgui->playlistbox.listbox.num_selected = -1;

            MDGUIPB__redraw (&mdgui->playlistbox);

            if (mdgui->current_play_state == MDGUI__NOT_PLAYING) {

                MDGUI__start_playing (mdgui);
                break;
            }

            break;
        }

        break;

    case MDGUI__PLAYLIST:

        if (mdgui->curr_playing && mdgui->current_play_state != MDGUI__NOT_PLAYING) {

            if (mdgui->current_play_state != MDGUI__WAITING_TO_STOP) {
                MD__stop_raw (mdgui->curr_playing);
                mdgui->current_play_state = MDGUI__WAITING_TO_STOP;
                mdgui->stop_all_signal = true;
            }
        }

        mdgui->playlistbox.num_playing = mdgui->playlistbox.listbox.num_selected;

        MDGUIPB__redraw (&mdgui->playlistbox);

        MDGUI__log ("Set new play item.", mdgui);

        if (mdgui->current_play_state == MDGUI__NOT_PLAYING) {

            MDGUI__start_playing (mdgui);
            break;
        }

        break;

    default:

        break;
    }

    return true;
}

bool MDGUI__down_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    switch (mdgui->selected_component) {

    case MDGUI__NONE:

        if (mdgui->potential_component == MDGUI__LOGO) {

            mdgui->potential_component = MDGUI__METABOX;

            MDGUI__draw_logo (mdgui);
            MDGUI__highlight_box (&mdgui->metabox.box);
            MDGUIMB__draw (&mdgui->metabox);
        }
        else if (mdgui->potential_component == MDGUI__NONE) {

            mdgui->potential_component = MDGUI__METABOX;

            MDGUI__highlight_box (&mdgui->metabox.box);
            MDGUIMB__draw (&mdgui->metabox);
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

    return true;
}

bool MDGUI__up_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    switch (mdgui->selected_component) {

    case MDGUI__NONE:

        if (mdgui->potential_component == MDGUI__METABOX) {

            mdgui->potential_component = MDGUI__LOGO;

            MDGUI__draw_logo (mdgui);
            MDGUI__unhighlight_box (&mdgui->metabox.box);
            MDGUIMB__draw (&mdgui->metabox);
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

    return true;
}

bool MDGUI__left_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

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
            MDGUIMB__draw (&mdgui->metabox);
            MDGUI__highlight_box (&mdgui->filebox.listbox.box);

            break;

        case MDGUI__PLAYLIST:

            mdgui->potential_component = MDGUI__METABOX;

            MDGUI__unhighlight_box (&mdgui->playlistbox.listbox.box);
            MDGUI__highlight_box (&mdgui->metabox.box);
            MDGUIMB__draw (&mdgui->metabox);

            break;

        default:

            break;
        }

        break;

    default:

        break;
    }

    return true;
}

bool MDGUI__right_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

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
            MDGUIMB__draw (&mdgui->metabox);

            break;

        case MDGUI__LOGO:

            mdgui->potential_component = MDGUI__PLAYLIST;

            MDGUI__draw_logo (mdgui);
            MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);

            break;

        case MDGUI__METABOX:

            mdgui->potential_component = MDGUI__PLAYLIST;

            MDGUI__unhighlight_box (&mdgui->metabox.box);
            MDGUIMB__draw (&mdgui->metabox);
            MDGUI__highlight_box (&mdgui->playlistbox.listbox.box);

            break;

        default:

            break;
        }

        break;

    default:

        break;
    }

    return true;
}

bool MDGUI__pause_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    if (mdgui->current_play_state == MDGUI__PLAYING
     || mdgui->current_play_state == MDGUI__PAUSE) {

        MD__toggle_pause_raw (mdgui->curr_playing);

        mdgui->current_play_state = mdgui->curr_playing->MD__pause_playing
                                  ? MDGUI__PAUSE
                                  : MDGUI__PLAYING;

        MDGUI__log (mdgui->current_play_state == MDGUI__PAUSE
                    ? "Playing paused."
                    : "Playing resumed.", mdgui);

    }

    return true;
}

bool MDGUI__stop_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    if (mdgui->curr_playing && mdgui->current_play_state != MDGUI__NOT_PLAYING) {

        if (mdgui->current_play_state != MDGUI__WAITING_TO_STOP) {
            MD__stop_raw (mdgui->curr_playing);
            mdgui->current_play_state = MDGUI__WAITING_TO_STOP;
            mdgui->stop_all_signal = true;

            mdgui->playlistbox.num_playing = -1;
        }
    }

    return true;
}

bool key_pressed (MDGUI__manager_t *mdgui, char key[3]) {

    if (key[0] == 27 && key[1] == 0 && key[2] == 0) {

        // ESCAPE

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            MDGUI__add_event (mdgui, MDGUI__exit_event, mdgui);

            return false;

        default:

            MDGUI__add_event (mdgui, MDGUI__escape_pressed_event, mdgui);
        }

    }
    else if ((key [0] == 13 || key [0] == 10) && key [1] == 0 && key[2] == 0) {

        // RETURN

        MDGUI__add_event (mdgui, MDGUI__return_event, mdgui);

    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 66) {

        // DOWN ARROW

        MDGUI__add_event (mdgui, MDGUI__down_event, mdgui);

    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 65) {

        // UP ARROW

        MDGUI__add_event (mdgui, MDGUI__up_event, mdgui);

    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 67) {

        // RIGHT ARROW

        MDGUI__add_event (mdgui, MDGUI__right_event, mdgui);

    } else if (key[0] == 27 && key[1] == 91 && key[2] == 68) {

        // LEFT ARROW

        MDGUI__add_event (mdgui, MDGUI__left_event, mdgui);

    }
    else if ((key[0] == 'p' || key[0] == 'P') && key[1] == 0 && key[2] == 0) {

        // PAUSE

        MDGUI__add_event (mdgui, MDGUI__pause_event, mdgui);

    }
    else if ((key[0] == 's' || key[0] == 'S') && key[1] == 0 && key[2] == 0) {

        // STOP

        MDGUI__add_event (mdgui, MDGUI__stop_event, mdgui);
    }

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

bool MDGUI__terminal_change_event (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__update_size (mdgui);

    clear();

    MDGUI__draw (mdgui);

    unsigned int tinfo_size = snprintf (NULL, 0, "Changed size to %d x %d.",
                                        mdgui->tinfo.cols, mdgui->tinfo.lines) + 1;

    char *tinfo_string = malloc (sizeof (*tinfo_string) * tinfo_size);

    snprintf (tinfo_string, tinfo_size, "Changed size to %d x %d.", mdgui->tinfo.cols,
              mdgui->tinfo.lines);

    MDGUI__log (tinfo_string, mdgui);

    curs_set(0);

    if (tinfo_string) free (tinfo_string);

    return true;

}

void *terminal_change (void *data) {

    MDGUI__manager_t *mdgui = (MDGUI__manager_t *)data;

    MDGUI__terminal previous_tinfo = mdgui->tinfo;

    while (true) {

        usleep(mdgui->refresh_rate);

        MDGUI__mutex_lock (mdgui);

        if (mdgui->current_play_state == MDGUI__PROGRAM_EXIT) {

            MDGUI__mutex_unlock (mdgui);

            break;
        }

        mdgui->tinfo = MDGUI__get_terminal_information ();

        if (mdgui->tinfo.cols != previous_tinfo.cols
         || mdgui->tinfo.lines != previous_tinfo.lines) {

            MDGUI__add_event_raw (mdgui, MDGUI__terminal_change_event, mdgui);

            previous_tinfo = mdgui->tinfo;
        }

        MDGUI__mutex_unlock (mdgui);
    }

    return NULL;
}

void MDGUI__start_playing (MDGUI__manager_t *mdgui) {

    if (mdgui->playlistbox.num_playing < 0
     || mdgui->playlistbox.num_playing >= mdgui->playlistbox.filenames.cnum) {

        mdgui->playlistbox.num_playing = -1;

        mdgui->current_play_state = MDGUI__NOT_PLAYING;

        MDGUI__log ("Can't play - out of bounds.", mdgui);

        return;
    }

    mdgui->current_play_state = MDGUI__INITIALIZING;

    pthread_t melodeer_thread;

    MDGUI__log ("Will play.", mdgui);

    if (pthread_create (&mdgui->melodeer_thread, NULL, MDGUI__play, (void *)mdgui))

        MDGUI__log ("(!) Could not create thread!", mdgui);

    return;
}

void MDGUI__complete (MDGUI__manager_t *mdgui) {

    MDGUI__mutex_lock (mdgui);

    if (mdgui->current_play_state == MDGUI__PLAYING
     || mdgui->current_play_state == MDGUI__PAUSE) {

        mdgui->current_play_state = MDGUI__PROGRAM_EXIT;

        if (mdgui->curr_playing) MD__stop_raw (mdgui->curr_playing);

        MDGUI__log ("Waiting to exit...\n", mdgui);

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
    pthread_mutexattr_destroy (&mdgui->mutex_attributes);

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

        // atomicity will be ensured by the thread calling buffer_transform
        MDGUIMB__fft_queue (&mdgui->metabox, new_sample, secs);
    }
}
