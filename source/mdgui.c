#include "mdgui.h"

void MDGUI__wait_for_keypress (MDGUI__manager_t *mdgui, bool (key_pressed)(MDGUI__manager_t *mdgui, char [3]), void (on_completion)(MDGUI__manager_t *mdgui));
void mdgui_completion (MDGUI__manager_t *mdgui);
bool key_pressed (MDGUI__manager_t *mdgui, char key[3]);
void *terminal_change (void *data);

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
    mdgui->bottom = 2;
    mdgui->left = 2;
    mdgui->right = 2;

    MDAL__initialize (4096, 4, 4);

    pthread_mutex_init (&mdgui->mutex, NULL);

    mdgui->tinfo = MDGUI__get_terminal_information ();

    mdgui->curr_playing = NULL;

    mdgui->selected_component = MDGUI__NONE;
    mdgui->potential_component = MDGUI__NONE;
    mdgui->previous_potential_component = MDGUI__NONE;

    mdgui->stop_all_signal = false;

    int box_width = MDGUI__get_box_width (mdgui);
    int box_height = MDGUI__get_box_height (mdgui);

    mdgui->filebox = MDGUIFB__create ("files", MDGUI__get_box_x (mdgui, 0), mdgui->top, box_width, box_height);
    
    mdgui->metabox = MDGUIMB__create ("metadata", MDGUI__get_box_x (mdgui, 1), mdgui->top + 7, box_width, box_height - 7);

    mdgui->playlistbox = MDGUILB__create ("playlist", MDGUI__get_box_x (mdgui, 2), mdgui->top, box_width, box_height, false);

    MDGUI__play_state volatile current_play_state = MDGUI__NOT_PLAYING;

    initscr ();

    clear ();

    curs_set (0);

    MDGUI__log ("Use arrow keys to move, ENTER to select and ESC to deselect/exit.", mdgui->tinfo);

    if (pthread_create (&mdgui->terminal_thread, NULL, terminal_change, mdgui)) {
        
        return false;
    }

    MDGUI__wait_for_keypress (mdgui, key_pressed, mdgui_completion);

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

void MDGUI__wait_for_keypress (MDGUI__manager_t *mdgui, bool (key_pressed)(MDGUI__manager_t *mdgui, char [3]), void (on_completion)(MDGUI__manager_t *mdgui)) {

    struct termios orig_term_attr;
    struct termios new_term_attr;

    char chain[3];

    tcgetattr (fileno(stdin), &orig_term_attr);
    memcpy (&new_term_attr, &orig_term_attr, sizeof (struct termios));
    new_term_attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
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

    clear ();

    curs_set (1);

    endwin();

    return;
}

bool key_pressed (MDGUI__manager_t *mdgui, char key[3]) {

    if (key[0] == 27 && key[1] == 0 && key[2] == 0) {

        // ESCAPE

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            return false;

        default:

            mdgui->selected_component = MDGUI__NONE;

            mdgui->potential_component = mdgui->previous_potential_component;

            // draw_all ();
        }

    }
    else if ((key [0] == 13 || key [0] == 10) && key [1] == 0 && key[2] == 0) {

        // RETURN

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            mdgui->selected_component = mdgui->potential_component;

            mdgui->previous_potential_component = mdgui->potential_component;

            mdgui->potential_component = MDGUI__NONE;

            // draw_all();

            break;

        case MDGUI__FILEBOX:

            if (mdgui->current_play_state == MDGUI__WAITING_TO_STOP || mdgui->current_play_state == MDGUI__INITIALIZING) break;

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

            if (mdgui->potential_component == MDGUI__LOGO)

                mdgui->potential_component = MDGUI__METABOX;

            else if (mdgui->potential_component == MDGUI__NONE)

                mdgui->potential_component = MDGUI__FILEBOX;

            // draw_all();

            break;

        case MDGUI__FILEBOX:

            break;

        case MDGUI__PLAYLIST:
            //
            // if (MDGUI__playlist_highlighted < MDGUI__playlist_first
            // &&  MDGUI__playlist_highlighted >= 0)
            //     MDGUI__playlist_first = MDGUI__playlist_highlighted;
            //
            // else if (MDGUI__playlist_highlighted > MDGUI__playlist_first + MDGUI__file_box_h - 2) {
            //
            //     MDGUI__playlist_highlighted = MDGUI__playlist_first + MDGUI__file_box_h - 3;
            //     // redraw_playlist_box ();
            //     break;
            // }
            //
            // if (MDGUI__playlist_highlighted < MDGUI__playlist_size - 1) MDGUI__playlist_highlighted++;
            // if (MDGUI__playlist_highlighted == MDGUI__playlist_first + MDGUI__file_box_h - 2) MDGUI__playlist_first++;
            //
            // // redraw_playlist_box ();

            break;

        default:

            break;
        }

    }
    else if (key[0] == 27 && key[1] == 91 && key[2] == 65) {

        // UP ARROW

        switch (mdgui->selected_component) {

        case MDGUI__NONE:

            if (mdgui->potential_component == MDGUI__METABOX)

                mdgui->potential_component = MDGUI__LOGO;

            else if (mdgui->potential_component == MDGUI__NONE)

                mdgui->potential_component = MDGUI__FILEBOX;

            // draw_all();

            break;

        case MDGUI__FILEBOX:



            // redraw_file_box ();

            break;

        case MDGUI__PLAYLIST:

            // if (MDGUI__playlist_highlighted < MDGUI__playlist_first && MDGUI__playlist_highlighted >= 0) MDGUI__playlist_first = MDGUI__playlist_highlighted;
            // else if (MDGUI__playlist_highlighted > MDGUI__playlist_first + MDGUI__file_box_h - 2) MDGUI__playlist_highlighted = MDGUI__playlist_first + MDGUI__file_box_h - 2;
            //
            // if (MDGUI__playlist_highlighted < 0) MDGUI__playlist_highlighted = 0;
            // if (MDGUI__playlist_highlighted > 0) MDGUI__playlist_highlighted--;
            // if (MDGUI__playlist_highlighted == MDGUI__playlist_first - 1 && MDGUI__playlist_first != 0) MDGUI__playlist_first--;

            // redraw_playlist_box ();

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

                mdgui->potential_component = MDGUI__FILEBOX;
                break;

            case MDGUI__FILEBOX:

                mdgui->potential_component = MDGUI__METABOX;
                break;

            case MDGUI__LOGO:

                mdgui->potential_component = MDGUI__PLAYLIST;
                break;

            case MDGUI__METABOX:

                mdgui->potential_component = MDGUI__PLAYLIST;
                break;

            case MDGUI__PLAYLIST:

                mdgui->potential_component = MDGUI__PLAYLIST;
                break;

            default:

                break;
            }

            // draw_all ();
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
                break;

            case MDGUI__LOGO:

                mdgui->potential_component = MDGUI__FILEBOX;
                break;

            case MDGUI__METABOX:

                mdgui->potential_component = MDGUI__FILEBOX;
                break;

            case MDGUI__PLAYLIST:

                mdgui->potential_component = MDGUI__METABOX;
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

            // layout ();

            if (mdgui->filebox.listbox.box.height >= mdgui->filebox.listbox.str_array.cnum) mdgui->filebox.listbox.num_first = 0;

            // if (MDGUI__file_box_x >= MDGUI__playlist_size) MDGUI__playlist_first = 0;

            clear();

            // draw_all ();

            unsigned int tinfo_size = snprintf (NULL, 0, "Changed size to %d x %d.", mdgui->tinfo.cols, mdgui->tinfo.lines) + 1;
            char *tinfo_string = malloc (sizeof (*tinfo_string) * tinfo_size);
            snprintf (tinfo_string, tinfo_size, "Changed size to %d x %d.", mdgui->tinfo.cols, mdgui->tinfo.lines);

            MDGUI__log (tinfo_string, mdgui->tinfo);
        }

        MDGUI__mutex_unlock (mdgui);
    }

    return NULL;
}

void mdgui_completion (MDGUI__manager_t *mdgui) {

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

    // MD__cleanup ();

    MDAL__close();
}


