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

MDGUI__file_box_t   MDGUIFB__create     (char *name, int x, int y,
                                         int height, int width);

void    MDGUIFB__draw                   (MDGUI__file_box_t *filebox);

void    MDGUIFB__redraw                 (MDGUI__file_box_t *filebox);

bool    MDGUIFB__return                 (MDGUI__file_box_t *filebox);

void    MDGUIFB__append_to_dirname      (MDGUI__file_box_t *filebox, char *append, char *result[]);

void    MDGUIFB__get_parent_dir         (MDGUI__file_box_t *filebox);

bool    MDGUIFB__get_dir_contents       (MDGUI__file_box_t *filebox);

void    MDGUIFB__deinit                 (MDGUI__file_box_t *filebox);

#endif

#define MDGUIFB_H
