#include "mdgui.h"

pthread_mutex_t MDGUI__mutex;

void MDGUI__clear_screen () {

    clear();
}

void MDGUI__log (char *log, MDGUI__terminal tinfo) {

    move (tinfo.lines - 1, 0);
    clrtoeol ();
    mvprintw (tinfo.lines - 1, 0, "%s", log);
    refresh ();
}

MDGUI__terminal MDGUI__get_terminal_information () {

    MDGUI__terminal tinfo;

    struct winsize w;
    ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);

    tinfo.cols = w.ws_col;
    tinfo.lines = w.ws_row;

    return tinfo;
}

void MDGUI__wait_for_keypress (bool (key_pressed)(char [3]), void (on_completion)(void)) {

    struct termios orig_term_attr;
    struct termios new_term_attr;

    char chain[3];

    tcgetattr (fileno(stdin), &orig_term_attr);
    memcpy (&new_term_attr, &orig_term_attr, sizeof (struct termios));
    new_term_attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
    tcsetattr (fileno(stdin), TCSANOW, &new_term_attr);

    fd_set input_set, output_set;

    for(;;) {

        char buff[3];
        memset (buff, 0, 3);

        FD_ZERO (&input_set);
        FD_SET (STDIN_FILENO, &input_set);

        int readn = select(1, &input_set, NULL, NULL, NULL);

        if (FD_ISSET (STDIN_FILENO, &input_set))
        {
            int buffread = read(STDIN_FILENO, buff, 3);

            if (!key_pressed(buff)) break;
        }
    }

    on_completion ();

    tcsetattr (fileno (stdin), TCSANOW, &orig_term_attr);

    clear ();

    curs_set (1);

    endwin();

    return;
}

void MDGUI__draw_box (bool box_selected, int term_pos_x, int term_pos_y, int width, int height) {

    char curr = ' ';

    for (int i=0; i<height; i++) {

        for (int j=0; j<width; j++) {

            if(j==0 || j==width-1) {

                if(i==0 || i==height-1) curr = '*';
                else curr = '|';
            }
            else if(i==0 || i==height-1) curr = '-';

            else curr = ' ';

            if (box_selected && curr != ' ') attron(A_BOLD);

            mvprintw(i + term_pos_y, j + term_pos_x, "%c", curr);

            if (box_selected && curr != ' ') attroff(A_BOLD);
        }
    }

    refresh();
}

//without terminal zero
int MDGUI__get_string_size (char *string) {

    int string_size = -1;

    while (string [++string_size] != 0) { }

    return string_size;
}

int MDGUI__partition (char **array[], int array_size, bool(*compare_f)(char *, char*), int lo, int hi) {

    char **pivot = *array+hi;
    int i = lo - 1;
    //int j = hi - 1;

    for(int j = lo; j <= hi - 1; j++)
    {
        if (!compare_f(*(*array+j), *pivot))
        {
            i++;

            char *temp = *(*array+i);

            *(*array+i) = *(*array+j);
            *(*array+j) = temp;
        }
    }

    char *temp = *(*array+(i+1));

    *(*array+(i+1)) = *(*array+hi);
    *(*array+hi) = temp;

    return (i+1);
}

void MDGUI__sort_step (char **array[], int array_size, bool(*compare_f)(char *, char*), int lo, int hi) {

    if (lo < hi) {
        int p = MDGUI__partition (array, array_size, compare_f, lo, hi);
        MDGUI__sort_step (array, array_size, compare_f, lo, p - 1);
        MDGUI__sort_step (array, array_size, compare_f, p + 1, hi);
    }
}

void MDGUI__sort (char **array[], int array_size, bool(*compare_f)(char *, char*)) {

    MDGUI__sort_step (array, array_size, compare_f, 0, array_size - 1);
}
