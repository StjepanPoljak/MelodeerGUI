#ifndef MDCSTRING_H

#include <stdbool.h>
#include <stdlib.h>
#include <ncurses.h>

struct MDGUI__str_array {

    char **carray;
    int cnum;
    int csize;
    int prev_csize;
};

typedef struct MDGUI__str_array MDGUI__str_array_t;

void        MDGUI__str_array_init           (MDGUI__str_array_t *str_array);

void        MDGUI__str_array_append         (MDGUI__str_array_t *str_array, char *string);

int         MDGUI__get_string_size          (char *string);

bool        MDGUI__compare                  (char *string1, char *string2);

void        MDGUI__sort                     (MDGUI__str_array_t *str_array,
                                             bool(*compare_f)(char *, char*));

void        MDGUI__str_array_empty          (MDGUI__str_array_t *str_array);

void        MDGUI__str_array_deinit         (MDGUI__str_array_t *str_array);

#endif

#define MDCSTRING_H
