#include "mdguifilebox.h"

char MDGUIFB__small_cap (char c) {

    return (c >= 65 && c <= 90) ? c + 32 : c;
}

bool MDGUIFB__compare (char *string1, char *string2) {

    int string1_size = MDGUI__get_string_size (string1);
    int string2_size = MDGUI__get_string_size (string2);

    int min_size = string1_size > string2_size ? string2_size : string1_size;

    for (int i = 0; i < min_size; i++) {

        char curr1 = MDGUIFB__small_cap(string1[i]);
        char curr2 = MDGUIFB__small_cap(string2[i]);

        if (curr1 == curr2) continue;

        return (curr1 > curr2);
    }

    return string1_size > string2_size;
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

bool MDGUIFB__get_dir_contents (char **carray[], int *cnum, char *curr_dir) {

    DIR *d;
    struct dirent *dir;

    char **result = NULL;
    int last = 0;

    d = opendir (curr_dir);

    if (d) {

        while ((dir = readdir(d)) != NULL) {

            if (result == NULL) result = malloc(sizeof(*result));
            else {

                char **old = malloc (sizeof (*old) * (last + 1));

                for (int j=0; j<last; j++) {

                    int old_j_string_size = MDGUI__get_string_size (result[j]);
                    old[j] = malloc (sizeof (**old) * old_j_string_size + 1);

                    for (int i=0; i <= old_j_string_size; i++) old[j][i] = result[j][i];

                    char *oldres = result[j];
                    free (oldres);
                }

                result = realloc (result, sizeof (*result) * (last + 2));

                for (int j=0; j<last; j++) {

                    int result_j_string_size = MDGUI__get_string_size (old[j]);
                    result[j] = malloc (sizeof(**result) * result_j_string_size + 1);

                    for (int i=0; i <= result_j_string_size; i++) result[j][i] = old[j][i];

                    char *oldold = old[j];
                    free (oldold);
                }

                free (old);
            }
            int dir_name_size = MDGUI__get_string_size (dir->d_name) + 1;

            // NOTE: first byte reserved for file info, 'f' if file, 'd' if dir

            result[last] = malloc (sizeof (**result) * dir_name_size + 1);

            if (dir->d_type == DT_DIR) result[last][0] = 'd';
            else result[last][0] = 'f';

            for (int i=0; i < dir_name_size; i++) result[last][i+1] = dir->d_name[i];

            last++;
        }

        closedir (d);

        char **old = *carray;
        *carray = result;

        if (old != NULL) {

            for(int i=0; i < *cnum; i++) free (old[i]);

            free (old);
        }

        *cnum = last;

        MDGUI__sort (*carray, *cnum, MDGUIFB__compare);

        return true;
    }

    return false;
}

void MDGUIFB__print_string_array (char *carray[], int cnum, bool dirflag,
                             int num_first, int num_lines,
                             int num_highlighted, int num_selected,
                             int term_pos_x, int term_pos_y, int width) {

    int line = 0;

    int start = num_first + num_lines >= cnum && cnum - num_lines >= 0
              ? cnum - num_lines
              : num_first;

    int end = (start + num_lines >= cnum) ? cnum : start + num_lines;

    for (int i = start, j = 0; i < end; i++, j++) {

        if (i == num_highlighted) attron (A_REVERSE);

        int str_size = MDGUI__get_string_size (carray[i]);

        int get_last_slash = 0;

        if (!dirflag) for (int k = str_size - 1; k >= 0; k--) if (carray[i][k] == '/') {

            get_last_slash = k + 1;
            str_size -= get_last_slash;

            break;
        }

        int chars_to_print = str_size >= width ? (dirflag ? width : width - 1) : str_size;

        if (carray[i][0] == 'd' && dirflag) attron (A_BOLD);

        if (!dirflag && i == num_selected) attron (A_BOLD);

        int maxprint = dirflag ? chars_to_print - 1 : chars_to_print;

        for (int k = 0; k < maxprint; k++)

            mvprintw (term_pos_y + j, term_pos_x + k, "%c",
                      carray[i][get_last_slash + (dirflag ? k+1 : k)]);

        if (i == num_highlighted) attroff (A_REVERSE);

        if (carray[i][0] == 'd' && dirflag) attroff (A_BOLD);

        if (!dirflag && i == num_selected) attroff (A_BOLD);
    }

    refresh();
}

void MDGUIFB__draw_file_box (char *carray[], int cnum, bool box_selected, bool dirflag,
                             int num_first, int num_highlighted, int num_selected,
                             int term_pos_x, int term_pos_y, int width, int height) {

    MDGUI__draw_box (box_selected, term_pos_x, term_pos_y, width, height);
    MDGUIFB__print_string_array (carray, cnum, dirflag,
                                 num_first, height - 4, num_highlighted, num_selected,
                                 term_pos_x + 2, term_pos_y + 2, width - 4);
}
