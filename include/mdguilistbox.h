#ifndef MDGUILB_H

#include "mdguibox.h"
#include "mdguistrarr.h"

struct MDGUI__listbox {

    MDGUI__box_t box;
    MDGUI__str_array_t str_array;

    int num_first;
    int num_selected;

    bool fd_prefix;

    void (*on_return) (struct MDGUI__listbox *listbox);
};

typedef struct  MDGUI__listbox MDGUI__listbox_t;

MDGUI__listbox_t    MDGUILB__create         (char *name, int x, int y,
                                             int height, int width, bool fd_prefix);

void                MDGUILB__up_arrow       (MDGUI__listbox_t *listbox);
void                MDGUILB__down_arrow     (MDGUI__listbox_t *listbox);
void				MDGUILB__return         (MDGUI__listbox_t *listbox);

void                MDGUILB__print_out      (MDGUI__listbox_t *listbox, int num_selected);

void                MDGUILB__draw           (MDGUI__listbox_t *listbox, int num_selected);

void                MDGUILB__deinit         (MDGUI__listbox_t *listbox);

#endif

#define MDGUILB_H
