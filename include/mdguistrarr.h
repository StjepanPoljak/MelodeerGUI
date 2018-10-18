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

char        MDGUI__small_cap                (char c);

void        MDGUI__sort                     (MDGUI__str_array_t *str_array,
                                             bool(*compare_f)(char *, char*));

void        MDGUI__str_array_empty          (MDGUI__str_array_t *str_array);

bool        MDGUI__str_array_is_empty       (MDGUI__str_array_t *str_array);

void        MDGUI__str_array_copy           (MDGUI__str_array_t *str_array_source,
                                             MDGUI__str_array_t *str_array_dest,
                                             int from_i, int to_i, int ignore);

void        MDGUI__str_array_copy_all       (MDGUI__str_array_t *str_array_source,
                                             MDGUI__str_array_t *str_array_dest,
                                             int ignore);

void        MDGUI__str_array_copy_all_from 	(MDGUI__str_array_t *str_array_source,
                                             MDGUI__str_array_t *str_array_dest,
                                             int from_i, int ignore);

void        MDGUI__str_array_deinit         (MDGUI__str_array_t *str_array);

#endif

#define MDCSTRING_H
