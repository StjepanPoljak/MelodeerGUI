#ifndef MDGUI_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <pthread.h>

#include <melodeer/mdcore.h>

#include "mdguistrarr.h"
#include "mdguifilebox.h"
#include "mdguimeta.h"
#include "mdguiplaylist.h"

struct MDGUI__terminal {

    int cols;
    int lines;
};

typedef struct  MDGUI__terminal MDGUI__terminal;

enum            MDGUI__component {

                        MDGUI__NONE,
                        MDGUI__FILEBOX,
                        MDGUI__METABOX,
                        MDGUI__PLAYLIST,
                        MDGUI__LOGO
                };

typedef enum MDGUI__component MDGUI__component;

enum            MDGUI__play_state {

                        MDGUI__NOT_PLAYING,
                        MDGUI__PAUSE,
                        MDGUI__PLAYING,
                        MDGUI__WAITING_TO_STOP,
                        MDGUI__EVALUATING,
                        MDGUI__INITIALIZING,
                        MDGUI__PROGRAM_EXIT,
                        MDGUI__READY_TO_EXIT
                };

typedef enum    MDGUI__play_state MDGUI__play_state;

struct MDGUI__manager {

    MD__file_t *curr_playing;

    MDGUI__terminal tinfo;

    MDGUI__component selected_component;
    MDGUI__component potential_component;
    MDGUI__component previous_potential_component;

    pthread_t melodeer_thread;

    pthread_t terminal_thread;

    pthread_mutex_t mutex;

    MDGUI__file_box_t filebox;
    MDGUI__playlist_box_t playlistbox;
    MDGUI__meta_box_t metabox;

    char *playlist_dir;

    MDGUI__play_state volatile current_play_state;

    int top;
    int bottom;
    int left;
    int right;
    int meta_top;
    int meta_bottom;

    bool stop_all_signal;
};

typedef struct MDGUI__manager MDGUI__manager_t;

bool            MDGUI__init                     (MDGUI__manager_t *mdgui);

bool            MDGUI__start                    (MDGUI__manager_t *mdgui);

void            MDGUI__log                      (const char *log, MDGUI__terminal tinfo);

void            MDGUI__draw                     (MDGUI__manager_t *mdgui);

void            MDGUI__mutex_lock               (MDGUI__manager_t *mdgui);

void            MDGUI__mutex_unlock             (MDGUI__manager_t *mdgui);

MDGUI__terminal MDGUI__get_terminal_information ();

void            MDGUI__deinit                   (MDGUI__manager_t *mdgui);

#endif

#define MDGUI_H
