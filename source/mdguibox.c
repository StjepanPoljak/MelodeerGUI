#include "mdguibox.h"

#include "mdguistrarr.h"

MDGUI__box_t MDGUI__box_create (char *name, int x, int y, int height, int width) {

    MDGUI__box_t new_box;

    int string_size = MDGUI__get_string_size (name) + 1;

    new_box.name = malloc (sizeof(new_box.name) * string_size);

    for (int i=0; i<string_size; i++) new_box.name[i] = name[i];

    new_box.x = x;
    new_box.y = y;
    new_box.height = height;
    new_box.width = width;

    new_box.highlighted = false;
    new_box.selected = false;

    return new_box;
}

void MDGUI__box_deinit (MDGUI__box_t *box) {

    if (box->name) free (box->name);
}

void MDGUI__draw_box (MDGUI__box_t *box) {

    bool box_highlighted = box->highlighted;

    int term_pos_x = box->x;
    int term_pos_y = box->y;
    int width = box->width;
    int height = box->height;

    char curr = ' ';

    if (box_highlighted) attron (A_BOLD);

    for (int i=0; i<height; i++) {

        for (int j=0; j<width; j++) {

            if (j==0 || j==width-1) {

                if (i==0 || i==height-1) curr = '*';
                else curr = '|';
            }
            else if (i==0 || i==height-1) curr = '-';

            else continue;

            mvprintw(i + term_pos_y, j + term_pos_x, "%c", curr);
        }
    }

    if (box_highlighted) attroff(A_BOLD);

    if (box->selected) attron (A_REVERSE);

    int string_size = MDGUI__get_string_size (box->name) + 2;

    mvprintw (term_pos_y, term_pos_x + (width - string_size) / 2, " %s ", box->name);

    if (box->selected) attroff (A_REVERSE);

    refresh();
}
