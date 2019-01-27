#ifndef MDGUIDRAW_H
#define MDGUIDRAW_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

enum term_attr { A_REVERSE, A_BOLD };

typedef enum term_attr term_attr_t;

void mvprintw (int x, int y, const char* format, ...);

void refresh ();

void clear ();

void curs_set (bool state);

void clrtoeol ();

void move ();

void attron (term_attr_t attr);
void attroff (term_attr_t attr);

#endif
