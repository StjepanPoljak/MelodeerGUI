#include "mdguifilebox.h"

void MDGUIFB__get_current_dir (MDGUI__file_box_t *filebox);

void MDGUIFB__return (MDGUI__file_box_t *filebox) {

    if (filebox->listbox.num_selected < 0 || filebox->listbox.num_selected >= filebox->listbox.str_array.cnum) return;

    switch (filebox->listbox.str_array.carray[filebox->listbox.num_selected][0]) {

    case 'f':

        break;

    case 'd':

        MDGUIFB__append_to_dirname (filebox, &(filebox->listbox.str_array.carray[filebox->listbox.num_selected][1]));

        break;
    }

    return;
}

MDGUI__file_box_t MDGUIFB__create (char *name, int x, int y, int height, int width) {

    MDGUI__file_box_t new_fb;

    new_fb.listbox = MDGUILB__create (name, x, y, height, width, true);

    new_fb.curr_dir = NULL;

    MDGUIFB__get_current_dir (&new_fb);

    MDGUIFB__get_dir_contents (&new_fb);

    return new_fb;
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

    if (!filebox->curr_dir) {

        char *newdir = malloc(sizeof(newdir)*(MDGUI__get_string_size(cwd)+1));
        for(int i=0;i<MDGUI__get_string_size(cwd); i++) newdir[i] = cwd[i];
        newdir[MDGUI__get_string_size(cwd)] = 0;
        filebox->curr_dir = newdir;
    }
    else {

        char *oldcurrdir = filebox->curr_dir;
        filebox->curr_dir = cwd;
        free (oldcurrdir);
    }

    return;
}

void MDGUIFB__append_to_dirname (MDGUI__file_box_t *filebox, char *append) {

    int string_count = MDGUI__get_string_size (filebox->curr_dir);
    int append_count = MDGUI__get_string_size (append);

    char *string = malloc (sizeof (*filebox->curr_dir) * (string_count + 1));

    for (int i=0; i<=string_count; i++) string[i] = filebox->curr_dir[i];

    int new_size = string_count + append_count + 1;

    bool needs_slash = string[string_count - 1] != '/' || append[0] != '/';

    int final_size = needs_slash ? ++new_size : new_size;

    if (filebox->curr_dir) filebox->curr_dir = realloc (filebox->curr_dir, sizeof (*filebox->curr_dir) * final_size);
    else filebox->curr_dir = malloc (sizeof (*filebox->curr_dir) * final_size);

    for (int i=0; i < string_count; i++) filebox->curr_dir[i] = string[i];

    if (needs_slash) filebox->curr_dir [string_count] = '/';

    for (int i=0; i < append_count; i++)

      filebox->curr_dir[i + string_count + (needs_slash ? 1 : 0)] = append[i];

    filebox->curr_dir [new_size - 1] = 0;
}

void MDGUIFB__get_parent_dir (char **new, char *curr) {

    int last = MDGUI__get_string_size (curr);

    char *result;

    if (last <= 1) {

        result = malloc(sizeof(*result)*(last+1));

        result[0] = curr[0];

        if (last == 1) result[1] = curr[1];

        char *oldnew = *new;

        *new = result;

        if (oldnew != NULL) free (oldnew);

        return;
    }

    int new_last = last;

    while (curr[--new_last] != '/' || new_last == last - 1) { }

    if (new_last == 0) new_last = 1;

    result = malloc (sizeof (*result) * (new_last + 1));

    if (new_last == 1) {

        if (curr[0] == '/') result[0] = '/';
        else result[0] = '.';

    } else for (int i=0; i<new_last; i++) result[i] = curr[i];

    result [new_last] = 0;

    char *oldnew = *new;

    *new = result;

    if (oldnew != NULL) free (oldnew);

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

void MDGUIFB__deinit (MDGUI__file_box_t *filebox) {

    MDGUILB__deinit (&filebox->listbox);

    if (filebox->curr_dir) free (filebox->curr_dir);
}
