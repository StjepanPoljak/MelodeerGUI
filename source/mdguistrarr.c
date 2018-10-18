#include "mdguistrarr.h"

//without terminal zero
int MDGUI__get_string_size (char *string) {

    int string_size = -1;

    while (string [++string_size] != 0) { }

    return string_size;
}

void MDGUI__str_array_init (MDGUI__str_array_t *str_array) {

    str_array->carray = NULL;
    str_array->cnum = 0;
    str_array->csize = 0;

    return;
}

void MDGUI__str_array_append (MDGUI__str_array_t *str_array, char *string) {

    char **temp = NULL;
    int prev_cnum = str_array->cnum++;

    if (str_array->csize > 0 && prev_cnum == str_array->csize) {

        str_array->csize = str_array->csize * 2;

        temp = malloc (sizeof (*(str_array->carray)) * str_array->csize);

        for (int i=0; i<prev_cnum; i++) {
    //
    // MDGUI__init (&mdgui);
    //
    // MDGUI__draw (&mdgui);
            int str_size = MDGUI__get_string_size (str_array->carray[i]) + 1;

            temp[i] = malloc (sizeof (**(str_array->carray)) * str_size);

            for (int j=0; j<str_size; j++) temp[i][j] = str_array->carray[i][j];
        }

        str_array->carray = realloc (str_array->carray,
                             sizeof (*(str_array->carray)) * (str_array->csize));
    }
    else if (str_array->csize == 0) {

        str_array->csize = 1;
        str_array->carray = malloc (sizeof (*(str_array->carray)));
    }

    int string_size = MDGUI__get_string_size (string) + 1;

    str_array->carray[prev_cnum] = malloc (sizeof (**str_array->carray) * string_size);

    for (int i=0; i<string_size; i++) str_array->carray[prev_cnum][i] = string[i];

    if (temp) {

        for (int i=0; i<prev_cnum; i++) {

            int i_string_size = MDGUI__get_string_size (str_array->carray[i]) + 1;

            str_array->carray[i] = realloc (str_array->carray[i], i_string_size);

            for (int j=0; j<i_string_size; j++) str_array->carray[i][j] = temp[i][j];

            free (temp[i]);
        }

        free (temp);
    }

    return;
}

char MDGUI__small_cap (char c) {

    return (c >= 65 && c <= 90) ? c + 32 : c;
}

int MDGUI__partition (MDGUI__str_array_t *str_array, bool(*compare_f)(char *, char*), int lo, int hi) {

    char *pivot = str_array->carray[hi];
    int i = lo - 1;

    for(int j = lo; j <= hi - 1; j++) {

        if (!compare_f(str_array->carray[j], pivot)) {

            i++;

            char *temp = str_array->carray[i];

            str_array->carray[i] = str_array->carray[j];
            str_array->carray[j] = temp;
        }
    }

    char *temp = str_array->carray[i+1];

    str_array->carray[i+1] = str_array->carray[hi];
    str_array->carray[hi] = temp;

    return i+1;
}

void MDGUI__sort_step (MDGUI__str_array_t *str_array, bool(*compare_f)(char *, char*), int lo, int hi) {

    if (lo < hi) {

        int p = MDGUI__partition (str_array, compare_f, lo, hi);
        MDGUI__sort_step (str_array, compare_f, lo, p - 1);
        MDGUI__sort_step (str_array, compare_f, p + 1, hi);
    }

    return;
}

void MDGUI__sort (MDGUI__str_array_t *str_array, bool(*compare_f)(char *, char*)) {

    MDGUI__sort_step (str_array, compare_f, 0, str_array->cnum - 1);

    return;
}

void MDGUI__str_array_empty (MDGUI__str_array_t *str_array) {

    for (int i=0; i<str_array->cnum; i++) {

        char *temp = str_array->carray[i];

        if (temp) free (temp);
    }

    str_array->cnum = 0;

    return;
}


bool MDGUI__str_array_is_empty (MDGUI__str_array_t *str_array) {

    return str_array->cnum <= 0;
}

void MDGUI__str_array_copy (MDGUI__str_array_t *str_array_source, MDGUI__str_array_t *str_array_dest, int from_i, int to_i, int ignore) {

    if (MDGUI__str_array_is_empty (str_array_source)) return;

    if (from_i < 0 || to_i >= str_array_source->cnum) return;

    int size = to_i - from_i + 1;

    if (size <= 0) return;

    if (!MDGUI__str_array_is_empty (str_array_dest)) MDGUI__str_array_empty (str_array_dest);

    if (str_array_dest->csize < size) {

        int allocsize = sizeof(*str_array_dest->carray) * size;

        if (str_array_dest->carray) str_array_dest->carray = realloc (str_array_dest->carray, allocsize);

        else str_array_dest->carray = malloc (allocsize);

        str_array_dest->cnum = size;
        str_array_dest->csize = size;
    }

    for (int i=0; i<size; i++) {

        int string_size = MDGUI__get_string_size (&(str_array_source->carray[i+from_i][ignore])) + 1;

        str_array_dest->carray[i] = malloc (sizeof(*str_array_dest->carray) * string_size);

        for (int j=0; j<string_size; j++) str_array_dest->carray[i][j] = str_array_source->carray[i+from_i][j+ignore];
    }

    return;
}

void MDGUI__str_array_copy_all_from (MDGUI__str_array_t *str_array_source, MDGUI__str_array_t *str_array_dest, int from_i, int ignore) {

    MDGUI__str_array_copy (str_array_source, str_array_dest, from_i, str_array_source->cnum - 1, ignore);

    return;
}

void MDGUI__str_array_copy_all (MDGUI__str_array_t *str_array_source, MDGUI__str_array_t *str_array_dest, int ignore) {

    MDGUI__str_array_copy_all_from (str_array_source, str_array_dest, 0, ignore);

    return;
}

void MDGUI__str_array_deinit (MDGUI__str_array_t *str_array) {

    if (str_array->carray) {

        for (int i=0; i<str_array->cnum; i++) free (str_array->carray[i]);

        free (str_array->carray);
    }

    return;
}
