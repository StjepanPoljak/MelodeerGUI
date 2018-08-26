#ifndef MDGUI

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct MDGUI__terminal {

    int cols;
    int lines;
};

typedef struct MDGUI__terminal MDGUI__terminal;

void MDGUI__log (char *log, MDGUI__terminal tinfo);

MDGUI__terminal MDGUI__get_terminal_information ();

void MDGUI__wait_for_keypress (bool (key_pressed)(int [3]));

void MDGUI__draw_box (bool box_selected, int term_pos_x, int term_pos_y, int width, int height);

void MDGUI__clear_screen ();

int MDGUI__get_string_size (char *string);


#endif

#define MDGUI
