#include "mdguifilebox.h"

void MDGUIFB__get_current_dir (char **currdir);

MDGUI__file_box_t MDGUIFB__create (char *name, int x, int y, int height, int width) {

    MDGUI__file_box_t new_fb;

    new_fb.listbox = MDGUILB__create (name, x, y, height, width, true);

    new_fb.curr_dir = NULL;

    MDGUIFB__get_current_dir (&new_fb.curr_dir);

    return new_fb;
}

void MDGUIFB__get_current_dir (char **currdir) {

    char cwd[PATH_MAX];

    if (getcwd (cwd, sizeof(cwd)) == NULL) {

        if (*currdir != NULL) {

            char *oldcurrdir = *currdir;
            *currdir = NULL;
            free (oldcurrdir);
        }

        return;
    }

    if (*currdir == NULL) {

        char *newdir = malloc(sizeof(newdir)*(MDGUI__get_string_size(cwd)+1));
        for(int i=0;i<MDGUI__get_string_size(cwd); i++) newdir[i] = cwd[i];
        newdir[MDGUI__get_string_size(cwd)] = 0;
        *currdir = newdir;
    }
    else {

        char *oldcurrdir = *currdir;
        *currdir = cwd;
        free (oldcurrdir);
    }
}

void MDGUIFB__append_to_dirname (char **new, char *string, char *append) {

    int string_count = MDGUI__get_string_size(string);
    int append_count = MDGUI__get_string_size(append);

    int new_size = string_count + append_count + 1;

    bool needs_slash = string[string_count - 1] != '/' || append[0] != '/';

    char *result;

    result = malloc (sizeof (*result) * (needs_slash ? ++new_size : new_size));

    for (int i=0; i < string_count; i++) result[i] = string[i];

    if (needs_slash) result [string_count] = '/';

    for (int i=0; i < append_count; i++)

      result[i + string_count + (needs_slash ? 1 : 0)] = append[i];

    result [new_size - 1] = 0;

    char *old = *new;
    *new = result;
    if (old != NULL) free (old);
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

        closedir (d);

        return true;
    }

    return false;
}

void MDGUIFB__draw_file_box (MDGUI__file_box_t *filebox) {

    MDGUILB__draw (&filebox->listbox, -1);
}

void MDGUIFB__deinit (MDGUI__file_box_t *filebox) {

    MDGUILB__deinit (&filebox->listbox);

    if (filebox->curr_dir) free (filebox->curr_dir);
}
