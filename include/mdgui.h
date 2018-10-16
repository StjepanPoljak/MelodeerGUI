#ifndef MDGUI_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <pthread.h>

#include "mdguistrarr.h"
#include "mdguibox.h"

struct MDGUI__terminal {

    int cols;
    int lines;
};

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
                        MDGUI__INITIALIZING,
                        MDGUI__PROGRAM_EXIT,
                        MDGUI__READY_TO_EXIT
                };

typedef enum    MDGUI__play_state MDGUI__play_state;

typedef struct  MDGUI__terminal MDGUI__terminal;

void            MDGUI__log                      (const char *log, MDGUI__terminal tinfo);

MDGUI__terminal MDGUI__get_terminal_information ();

void            MDGUI__wait_for_keypress        (bool (key_pressed)(char [3]),
                                                 void (on_completion) (void));

#endif

#define MDGUI_H
