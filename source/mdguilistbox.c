#include "mdguilistbox.h"

MDGUI__listbox_t MDGUILB__create (char *name, int x, int y,
                                  int height, int width, bool fd_prefix) {

    MDGUI__listbox_t listbox;

    listbox.box = MDGUI__box_create (name, x, y, height, width);

    listbox.num_first = 0;
    listbox.num_selected = -1;

    listbox.fd_prefix = fd_prefix;

    listbox.on_return = NULL;

    MDGUI__str_array_init (&listbox.str_array);

    return listbox;
}

void MDGUILB__return (MDGUI__listbox_t *listbox) {

    if (listbox->on_return) listbox->on_return (listbox);
}

int MDGUILB__num_lines (MDGUI__listbox_t *listbox) {

    return listbox->box.height - 4;
}

void MDGUILB__up_arrow (MDGUI__listbox_t *listbox) {

    int selected_file = listbox->num_selected;

    if (listbox->num_selected < listbox->num_first && listbox->num_selected >= 0)

        listbox->num_first = listbox->num_selected;

    else if (listbox->num_selected > listbox->num_first + MDGUILB__num_lines (listbox))

        listbox->num_selected = listbox->num_first + MDGUILB__num_lines (listbox);

    if (listbox->num_selected < 0) listbox->num_selected = 0;
    if (listbox->num_selected > 0) listbox->num_selected--;

    if (listbox->num_selected == listbox->num_first - 1 && listbox->num_first != 0)

        listbox->num_first--;

    return;
}

void MDGUILB__down_arrow (MDGUI__listbox_t *listbox) {

    if (listbox->num_selected < listbox->num_first && listbox->num_selected >= 0)

        listbox->num_first = listbox->num_selected;

    else if (listbox->num_selected > listbox->num_first + MDGUILB__num_lines (listbox)) {

        listbox->num_selected = listbox->num_first + MDGUILB__num_lines (listbox) - 1;

        return;
    }

    if (listbox->num_selected < listbox->str_array.cnum - 1) listbox->num_selected++;

    if (listbox->num_selected == listbox->num_first + MDGUILB__num_lines (listbox))

        listbox->num_first++;
}

void MDGUILB__print_out (MDGUI__listbox_t *listbox, int num_selected) {

    if (listbox->str_array.cnum == 0) return;

    int line = 0;

    int num_lines = MDGUILB__num_lines (listbox);
    int cnum = listbox->str_array.cnum;
    int num_first = listbox->num_first;
    bool dirflag = listbox->fd_prefix;
    int width = listbox->box.width;
    int num_highlighted = listbox->num_selected;

    int start = num_first + num_lines >= cnum && cnum - num_lines >= 0
              ? cnum - num_lines
              : num_first;

    int end = (start + num_lines >= cnum) ? cnum : start + num_lines;

    for (int i = start, j = 0; i < end; i++, j++) {

        if (i == num_highlighted) attron (A_REVERSE);

        int str_size = MDGUI__get_string_size (listbox->str_array.carray[i]);

        int get_last_slash = 0;

        if (!dirflag) for (int k = str_size - 1; k >= 0; k--)

            if (listbox->str_array.carray[i][k] == '/') {

                get_last_slash = k + 1;
                str_size -= get_last_slash;

                break;
            }

        int chars_to_print = str_size >= width ? (dirflag ? width : width - 1) : str_size;

        if (listbox->str_array.carray[i][0] == 'd' && dirflag) attron (A_BOLD);

        if (!dirflag && i == num_selected) attron (A_BOLD);

        int maxprint = dirflag ? chars_to_print - 1 : chars_to_print;

        for (int k = 0; k < maxprint; k++)

            mvprintw (listbox->box.y + j + 2, listbox->box.x + k + 2, "%c",
                      listbox->str_array.carray[i][get_last_slash + (dirflag ? k+1 : k)]);

        if (i == num_highlighted) attroff (A_REVERSE);

        if (listbox->str_array.carray[i][0] == 'd' && dirflag) attroff (A_BOLD);

        if (!dirflag && i == num_selected) attroff (A_BOLD);
    }

    refresh ();
}

void MDGUILB__draw (MDGUI__listbox_t *listbox, int num_highlighted) {

    MDGUI__draw_box (&listbox->box);
    MDGUILB__print_out (listbox, num_highlighted);
}

void MDGUILB__deinit (MDGUI__listbox_t *listbox) {

    MDGUI__box_deinit (&listbox->box);

    MDGUI__str_array_deinit (&listbox->str_array);
}
