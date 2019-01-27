#include "mdguidraw.h"

#include <termios.h>

bool term_state[2] = { false, false };

void mvprintw (int x, int y, const char* format, ...) {

    va_list arglist;

    va_start (arglist, format);
    printf("\033[%d;%dH", x + 1, y + 1);
    vprintf (format, arglist);
    va_end (arglist);
}

void move (int x, int y) {

    printf("\033[%d;%dH", x + 1 , y + 1);
}

void clrtoeol () {

    printf("\033[K");
}

void clear () {

    printf("\033c");
    fflush (stdout);
}

void curs_set (bool state) {

    switch (state) {
    case 1:
        printf("\033[?25h");
    case 0:
        printf("\033[?25l");
    }
}

void refresh () {
    fflush (stdout);
}

void attron (term_attr_t attr) {
    switch (attr) {
    case A_BOLD:
        printf("\033[1m");
        term_state[0] = true;
        break;
    case A_REVERSE:
        printf("\033[7m");
        term_state[1] = true;
        break;
    default:
        break;
    }
}

void attroff (term_attr_t attr) {

    switch (attr) {
    case A_BOLD:
        term_state[0] = false;
        printf("\033[0m");
        if (term_state[1]) printf("\033[7m");
        break;
    case A_REVERSE:
        term_state[1] = false;
        printf("\033[0m");
        if (term_state[0]) printf("\033[1m");
        break;
    default:
        break;
    }
}
