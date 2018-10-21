#ifndef MDGUIFB_H

#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#include "mdguilistbox.h"

struct MDGUI__file_box_stack {

    char *dir;
    int num_highlighted;
    int num_first;

    struct MDGUI__file_box_stack *previous;
};

struct MDGUI__file_box {

    MDGUI__listbox_t listbox;

    struct MDGUI__file_box_stack *last;
};

typedef struct MDGUI__file_box MDGUI__file_box_t;

MDGUI__file_box_t   MDGUIFB__create     (char *name, int x, int y,
                                        int height, int width);

void    MDGUIFB__draw                   (MDGUI__file_box_t *filebox);

void    MDGUIFB__redraw                 (MDGUI__file_box_t *filebox);

bool    MDGUIFB__return                 (MDGUI__file_box_t *filebox);

bool    MDGUIFB__get_dir_contents       (MDGUI__file_box_t *filebox);

void    MDGUIFB__serialize_curr_dir     (MDGUI__file_box_t *filebox, char **curr_dir);

void    MDGUIFB__deinit                 (MDGUI__file_box_t *filebox);

#endif

#define MDGUIFB_H
