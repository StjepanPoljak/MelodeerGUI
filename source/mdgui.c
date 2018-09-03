#include "mdgui.h"

pthread_mutex_t MDGUI__mutex;

bool MDGUI__needs_redraw = false;

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

void MDGUI__wait_for_keypress (bool (key_pressed)(char [3]), void (on_needs_redraw)(void)) {

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

        if (FD_ISSET(STDIN_FILENO, &input_set))
        {
            int buffread = read(STDIN_FILENO, buff, 3);

            if (!key_pressed(buff)) break;
        }
    }

    tcsetattr (fileno (stdin), TCSANOW, &orig_term_attr);

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
        if (curr1 < curr2) return false;
        else return true;
    }

    // if equal - does not matter much really
    return false;
}
