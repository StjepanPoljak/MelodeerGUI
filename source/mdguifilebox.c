#include "mdguifilebox.h"

void MDGUIFB__get_current_dir (MDGUI__file_box_t *filebox);

MDGUI__file_box_t MDGUIFB__create (char *name, int x, int y, int height, int width) {

    MDGUI__file_box_t new_fb;

    new_fb.listbox = MDGUILB__create (name, x, y, height, width, true);

    new_fb.curr_dir = NULL;

    MDGUIFB__get_current_dir (&new_fb);

    MDGUIFB__get_dir_contents (&new_fb);

    return new_fb;
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

        if ((selected_size == 1 && selected[1] == '.') || selected_size == 0) return false;

        else if (selected_size == 2 && selected[1] == '.' && selected[2] == '.') {

            MDGUIFB__get_parent_dir (filebox);

        }
        else {

            MDGUIFB__append_to_dirname (filebox, &(selected[1]), &filebox->curr_dir);

        }

        MDGUIFB__get_dir_contents (filebox);

        filebox->listbox.num_selected = 0;
        filebox->listbox.num_first = 0;

        MDGUIFB__redraw (filebox);

        break;
    }

    return false;
}

void MDGUIFB__get_current_dir (MDGUI__file_box_t *filebox) {

    char cwd[PATH_MAX];

    if (getcwd (cwd, sizeof(cwd)) == NULL) {

        if (filebox->curr_dir) {

            char *oldcurrdir = filebox->curr_dir;
            filebox->curr_dir = NULL;
            free (oldcurrdir);
        }

        return;
    }

    int cwd_str_size = MDGUI__get_string_size (cwd) + 1;

    if (filebox->curr_dir) filebox->curr_dir = realloc (filebox->curr_dir, sizeof (filebox->curr_dir) * cwd_str_size);

    else filebox->curr_dir = malloc (sizeof (filebox->curr_dir) * cwd_str_size);

    for(int i=0; i<cwd_str_size; i++) filebox->curr_dir[i] = cwd[i];

    return;
}

void MDGUIFB__append_to_dirname (MDGUI__file_box_t *filebox, char *append, char *result[]) {

    int string_count = MDGUI__get_string_size (filebox->curr_dir);
    int append_count = MDGUI__get_string_size (append);

    char *string = malloc (sizeof (*filebox->curr_dir) * (string_count + 1));

    for (int i=0; i<=string_count; i++) string[i] = filebox->curr_dir[i];

    int new_size = string_count + append_count + 1;

    bool needs_slash = string[string_count - 1] != '/' || append[0] != '/';

    int final_size = needs_slash ? ++new_size : new_size;

    if (*result) *result = realloc (*result, sizeof (**result) * final_size);
    else *result = malloc (sizeof (**result) * final_size);

    for (int i=0; i < string_count; i++) (*result)[i] = string[i];

    if (needs_slash) (*result) [string_count] = '/';

    for (int i=0; i < append_count; i++) (*result)[i + string_count + (needs_slash ? 1 : 0)] = append[i];

    (*result) [new_size - 1] = 0;
}

void MDGUIFB__get_parent_dir (MDGUI__file_box_t *filebox) {

    int last = MDGUI__get_string_size (filebox->curr_dir);

    char *curr = malloc (sizeof (*curr) * (last + 1));

    for (int i=0; i<=last; i++) curr[i] = filebox->curr_dir[i];

    if (last <= 1) {

        if (filebox->curr_dir) filebox->curr_dir = realloc (filebox->curr_dir, sizeof (*filebox->curr_dir) * (last + 1));

        else filebox->curr_dir = malloc (sizeof (*filebox->curr_dir) * (last + 1));

        filebox->curr_dir[0] = curr[0];

        if (last == 1) filebox->curr_dir[1] = curr[1];

        if (curr) free (curr);

        return;
    }

    int new_last = last;

    while (curr[--new_last] != '/' || new_last == last - 1) { }

    if (new_last == 0) new_last = 1;

    if (filebox->curr_dir) filebox->curr_dir = realloc (filebox->curr_dir, sizeof (*filebox->curr_dir) * (new_last + 1));

    else filebox->curr_dir = malloc (sizeof (*filebox->curr_dir) * (new_last + 1));

    if (new_last == 1) {

        if (curr[0] == '/') filebox->curr_dir[0] = '/';

        else filebox->curr_dir[0] = '.';

    } else for (int i=0; i<new_last; i++) filebox->curr_dir[i] = curr[i];

    filebox->curr_dir [new_last] = 0;

    if (curr) free (curr);

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

bool MDGUIFB__get_dir_contents (MDGUI__file_box_t *filebox) {

    DIR *d;
    struct dirent *dir;

    int last = 0;

    d = opendir (filebox->curr_dir);

    if (d) {

        MDGUI__str_array_empty (&filebox->listbox.str_array);

        while ((dir = readdir(d)) != NULL) {

            int dir_name_size = MDGUI__get_string_size (dir->d_name) + 2;

            // NOTE: first byte reserved for file info, 'f' if file, 'd' if dir

            char *tempdir = malloc (sizeof (*tempdir) * dir_name_size + 1);

            if (dir->d_type == DT_DIR) tempdir[0] = 'd';
            else tempdir[0] = 'f';

            for (int i=0; i < dir_name_size; i++) tempdir[i+1] = dir->d_name[i];

            MDGUI__str_array_append (&filebox->listbox.str_array, tempdir);

            free (tempdir);
        }

        MDGUI__sort (&filebox->listbox.str_array, MDGUI__compare);

        closedir (d);

        return true;
    }

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

    if (filebox->curr_dir) free (filebox->curr_dir);
}
