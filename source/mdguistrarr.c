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
}

void MDGUI__str_array_append (MDGUI__str_array_t *str_array, char *string) {

    char **temp = NULL;
    int prev_cnum = str_array->cnum++;

    if (str_array->csize > 0 && prev_cnum == str_array->csize) {

        str_array->csize = str_array->csize * 2;

        temp = malloc (sizeof (*(str_array->carray)) * str_array->csize);

        for (int i=0; i<prev_cnum; i++) {

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
}

char MDGUI__small_cap (char c) {

    return (c >= 65 && c <= 90) ? c + 32 : c;
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

int MDGUI__partition (MDGUI__str_array_t *str_array, bool(*compare_f)(char *, char*), int lo, int hi) {

    char *pivot = str_array->carray[hi];
    int i = lo - 1;

    for(int j = lo; j <= hi - 1; j++)
    {
        if (!compare_f(str_array->carray[j], pivot))
        {
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
}

void MDGUI__sort (MDGUI__str_array_t *str_array, bool(*compare_f)(char *, char*)) {

    MDGUI__sort_step (str_array, compare_f, 0, str_array->cnum - 1);
}

void MDGUI__str_array_empty (MDGUI__str_array_t *str_array) {

    for (int i=0; i<str_array->cnum; i++) {

        char *temp = str_array->carray[i];

        if (temp) free (temp);
    }

    str_array->cnum = 0;
}

void MDGUI__str_array_deinit (MDGUI__str_array_t *str_array) {

    if (str_array->carray) {

        for (int i=0; i<str_array->cnum; i++) free (str_array->carray[i]);

        free (str_array->carray);
    }
}
