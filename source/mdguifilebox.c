#include "mdguifilebox.h"

void MDGUIFB__get_current_dir (MDGUI__file_box_t *filebox);

MDGUI__file_box_t MDGUIFB__create (char *name, int x, int y, int height, int width) {

    MDGUI__file_box_t new_fb;

    new_fb.listbox = MDGUILB__create (name, x, y, height, width, true);

    new_fb.last = NULL;

    MDGUIFB__get_current_dir (&new_fb);

    MDGUIFB__get_dir_contents (&new_fb);

    return new_fb;
}

void MDGUIFB__stack_element_init (struct MDGUI__file_box_stack *stack_el,
                                  char *filename, int num_first, int num_highlighted) {

    int filename_size = MDGUI__get_string_size (filename) + 1;

    stack_el->dir = malloc (sizeof (*filename) * filename_size);

    for (int i = 0; i < filename_size; i++) stack_el->dir[i] = filename[i];

    stack_el->num_highlighted = num_highlighted;
    stack_el->num_first = num_first;
    stack_el->previous = NULL;

    return;
}

void MDGUIFB__stack_element_deinit (struct MDGUI__file_box_stack *stack_el) {

    if (stack_el->dir) free (stack_el->dir);
}

void MDGUIFB__stack_push (MDGUI__file_box_t *filebox) {

    struct MDGUI__file_box_stack *new_stack_el = malloc (sizeof(*new_stack_el));

    char *filename = filebox->listbox.str_array.carray [filebox->listbox.num_selected];

    MDGUIFB__stack_element_init (new_stack_el, &(filename[1]), filebox->listbox.num_first, filebox->listbox.num_selected);

    if (!filebox->last) filebox->last = new_stack_el;

    else {

        new_stack_el->previous = filebox->last;
        filebox->last = new_stack_el;
    }

    filebox->listbox.num_selected = 0;
    filebox->listbox.num_first = 0;

    return;
}

void MDGUIFB__stack_pop (MDGUI__file_box_t *filebox) {

    if (!filebox->last) return;

    struct MDGUI__file_box_stack *to_pop = filebox->last;

    filebox->listbox.num_selected = to_pop->num_highlighted;
    filebox->listbox.num_first = to_pop->num_first;

    if (filebox->last->previous) filebox->last = filebox->last->previous;

    else filebox->last = NULL;

    MDGUIFB__stack_element_deinit (to_pop);
    free (to_pop);

    return;
}

void MDGUIFB__stack_deinit (MDGUI__file_box_t *filebox) {

    struct MDGUI__file_box_stack *current = filebox->last;

    if (!current) return;

    while (true) {

        if (!current) break;

        struct MDGUI__file_box_stack *to_free = current;
        current = current->previous;
        MDGUIFB__stack_element_deinit (to_free);
        free (to_free);
    }

    return;
}

bool MDGUIFB__return (MDGUI__file_box_t *filebox) {

    if (filebox->listbox.num_selected < 0 || filebox->listbox.num_selected >= filebox->listbox.str_array.cnum) return false;

    char *selected = filebox->listbox.str_array.carray[filebox->listbox.num_selected];

    switch (selected[0]) {

    case 'f':

        return true;

    case 'd':

        ;

        int selected_size = MDGUI__get_string_size (&(selected[1]));

        if ((selected_size == 1 && selected[1] == '.') || selected_size == 0)

            return false;

        else if (selected_size == 2 && selected[1] == '.' && selected[2] == '.')

            MDGUIFB__stack_pop (filebox);

        else

            MDGUIFB__stack_push (filebox);

        MDGUIFB__get_dir_contents (filebox);

        MDGUIFB__redraw (filebox);

        break;
    }

    return false;
}

void MDGUIFB__parse_filename (MDGUI__file_box_t *filebox, char *filename) {

    int i_first = -1;
    int i_length = 0;

    int filename_size = MDGUI__get_string_size (filename) + 1;

    for (int i=0; i < filename_size; i++) {

        if (i_length > 0 && (filename[i] == '/' || filename[i] == 0) && i_first != -1) {

            char *to_add = malloc ((i_length+1) * sizeof (char));

            for (int j=0; j < i_length; j++) to_add [j] = filename[j + i_first];

            to_add[i_length] = 0;

            struct MDGUI__file_box_stack *new_stack_el = malloc (sizeof(*new_stack_el));

            MDGUIFB__stack_element_init (new_stack_el, to_add, 0, 0);

            if (!filebox->last) filebox->last = new_stack_el;

            else {

                new_stack_el->previous = filebox->last;
                filebox->last = new_stack_el;
            }

            free (to_add);

            i_length = 0;

            i_first = i + 1;

            continue;
        }
        else if (filename[i] == '/') {

            i_first = i + 1;
            i_length = 0;
        }
        else if (i_first != -1) i_length ++;
    }
}

void MDGUIFB__get_current_dir (MDGUI__file_box_t *filebox) {

    char cwd[PATH_MAX];

    if (getcwd (cwd, sizeof(cwd)))

        MDGUIFB__parse_filename (filebox, cwd);

    return;
}

bool MDGUI__compare (char *string1, char *string2) {

    int string1_size = MDGUI__get_string_size (string1);
    int string2_size = MDGUI__get_string_size (string2);

    int min_size = string1_size > string2_size ? string2_size : string1_size;

    for (int i = 0; i < min_size; i++) {

        char curr1 = MDGUI__small_cap(string1[i]);
        char curr2 = MDGUI__small_cap(string2[i]);

        if (curr1 == curr2) continue;

        return (curr1 > curr2);
    }

    return string1_size > string2_size;
}

void MDGUIFB__serialize_curr_dir (MDGUI__file_box_t *filebox, char **curr_dir) {

    if (!filebox->last) {

        if (*curr_dir) *curr_dir = realloc (*curr_dir, sizeof (**curr_dir) * 2);

        else *curr_dir = malloc (sizeof (**curr_dir) * 2);

        (*curr_dir)[0] = '/';
        (*curr_dir)[1] = 0;

        return;
    }

    struct MDGUI__file_box_stack *current = filebox->last;

    if (!current) return;

    int curr_dir_size = 0;

    while (true) {

        if (!current) break;

        curr_dir_size += MDGUI__get_string_size (current->dir) + 1;

        current = current->previous;
    }

    curr_dir_size++;

    if (*curr_dir) *curr_dir = realloc (*curr_dir, sizeof (**curr_dir) * curr_dir_size);

    else *curr_dir = malloc (sizeof (**curr_dir) * curr_dir_size);

    int i=curr_dir_size - 1;

    (*curr_dir)[i--] = 0;

    current = filebox->last;

    while (true) {

        if (i<=0) break;

        if (!current) break;

        else {
            for (int j=MDGUI__get_string_size (current->dir) - 1; j >= 0; j--) {

                if (i<=0) break;

                (*curr_dir)[i--] = current->dir[j];
            }

            (*curr_dir)[i--] = '/';

            current = current->previous;
        }
    }
}

bool MDGUIFB__get_dir_contents (MDGUI__file_box_t *filebox) {

    DIR *d;
    struct dirent *dir;

    int last = 0;

    char *curr_dir = NULL;

    MDGUIFB__serialize_curr_dir (filebox, &curr_dir);

    d = opendir (curr_dir);

    if (d) {

        MDGUI__str_array_empty (&filebox->listbox.str_array);

        while ((dir = readdir(d)) != NULL) {

            int dir_name_size = MDGUI__get_string_size (dir->d_name) + 2;

            // NOTE: first byte reserved for file info, 'f' if file, 'd' if dir

            char *tempdir = malloc (sizeof (*tempdir) * (dir_name_size + 1));

            if (dir->d_type == DT_DIR) tempdir[0] = 'd';
            else tempdir[0] = 'f';

            for (int i=0; i < dir_name_size; i++) tempdir[i+1] = dir->d_name[i];

            MDGUI__str_array_append (&filebox->listbox.str_array, tempdir);

            free (tempdir);
        }

        MDGUI__sort (&filebox->listbox.str_array, MDGUI__compare);

        closedir (d);

        if (curr_dir) free (curr_dir);

        return true;
    }

    if (curr_dir) free (curr_dir);

    return false;
}

void MDGUIFB__draw (MDGUI__file_box_t *filebox) {

    MDGUILB__draw (&filebox->listbox, -1);
}

void MDGUIFB__redraw (MDGUI__file_box_t *filebox) {

    MDGUILB__redraw (&filebox->listbox, -1);
}

void MDGUIFB__deinit (MDGUI__file_box_t *filebox) {

    MDGUILB__deinit (&filebox->listbox);

    MDGUIFB__stack_deinit (filebox);
}
