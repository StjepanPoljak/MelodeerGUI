#ifndef MDGUI

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

struct MDGUI__terminal {

    int cols;
    int lines;
};

typedef struct MDGUI__terminal MDGUI__terminal;

void MDGUI__log (char *log, MDGUI__terminal tinfo);

MDGUI__terminal MDGUI__get_terminal_information ();

void MDGUI__wait_for_keypress (bool (key_pressed)(char [3]), void (on_completion) (void));

void MDGUI__draw_box (bool box_selected, int term_pos_x, int term_pos_y, int width, int height);

int MDGUI__get_string_size (char *string);

void MDGUI__sort (char **array[], int array_size, bool(*compare_f)(char *, char*));

#endif

#define MDGUI
