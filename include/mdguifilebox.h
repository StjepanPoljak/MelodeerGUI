#ifndef MDGUIFB_H

#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#include "mdguilistbox.h"

struct MDGUI__file_box {

    MDGUI__listbox_t listbox;

    char *curr_dir;
};

typedef struct MDGUI__file_box MDGUI__file_box_t;

MDGUI__file_box_t   MDGUIFB__create      (char *name, int x, int y,
                                         int height, int width);

void    MDGUIFB__append_to_dirname      (char **new, char *string, char *append);

void    MDGUIFB__get_parent_dir         (char **new, char *curr);

void    MDGUIFB__get_current_dir        (char **currdir);

bool    MDGUIFB__get_dir_contents       (MDGUI__file_box_t *filebox);

void    MDGUIFB__draw_file_box           (MDGUI__file_box_t *filebox);

void    MDGUIFB__deinit                 (MDGUI__file_box_t *filebox);

#endif

#define MDGUIFB_H
