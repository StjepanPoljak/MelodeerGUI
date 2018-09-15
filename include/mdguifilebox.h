#ifndef MDGUIFB

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>

#include "mdgui.h"

void    MDGUIFB__append_to_dirname      (char **new, char *string, char *append);

void    MDGUIFB__get_parent_dir         (char **new, char *curr);

void    MDGUIFB__get_current_dir        (char **currdir);

bool    MDGUIFB__get_dir_contents       (char **carray[], int *cnum, char *curr_dir);

void    MDGUIFB__print_string_array     (char *carray[], int cnum, bool dirflag,
                                         int num_first, int num_lines,
                                         int num_highlighted, int num_selected,
                                         int term_pos_x, int term_pos_y, int width);

void    MDGUIFB__draw_file_box          (char *carray[], int cnum,
                                         bool box_selected, bool dirflag,
                                         int num_first, int num_highlighted, int num_selected,
                                         int term_pos_x, int term_pos_y,
                                         int width, int height);
#endif

#define MDGUIFB
