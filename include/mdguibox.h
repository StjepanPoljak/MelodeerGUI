#ifndef MDGUIBOX_H

#include <stdlib.h>
#include <stdbool.h>
#include "mdguidraw.h"
// #include <ncurses.h>

struct MDGUI__box {

    char *name;

    int x;
    int y;
    int height;
    int width;

    bool selected;
    bool highlighted;
};

typedef struct MDGUI__box MDGUI__box_t;

MDGUI__box_t    MDGUI__box_create       (char *name, int x, int y, int height, int width);

void            MDGUI__draw_box         (MDGUI__box_t *box);
void            MDGUI__draw_box_opt     (MDGUI__box_t *box, bool clear_contents);

void            MDGUI__unhighlight_box  (MDGUI__box_t *box);
void            MDGUI__highlight_box    (MDGUI__box_t *box);
void            MDGUI__select_box       (MDGUI__box_t *box);
void            MDGUI__deselect_box     (MDGUI__box_t *box);

void            MDGUI__box_deinit       ();

#endif

#define MDGUIBOX_H
