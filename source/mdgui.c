#include "mdgui.h"

pthread_mutex_t MDGUI__mutex;

void MDGUI__clear_screen () {

    printf("\033c");
}

void MDGUI__log (char *log, MDGUI__terminal tinfo) {

    for(int i=0;i<tinfo.cols;i++) printf("\033[%d;%dH ", tinfo.lines, i);
    printf("\033[2m\033[%d;%dH%s\033[0m", tinfo.lines, 0, log);
}

MDGUI__terminal MDGUI__get_terminal_information () {

    MDGUI__terminal tinfo;

    struct winsize w;
    ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);

    tinfo.cols = w.ws_col;
    tinfo.lines = w.ws_row;

    return tinfo;
}

void MDGUI__wait_for_keypress (bool (key_pressed)(int [3])) {

    struct termios orig_term_attr;
    struct termios new_term_attr;

    char chain[3];

    tcgetattr (fileno(stdin), &orig_term_attr);
    memcpy (&new_term_attr, &orig_term_attr, sizeof (struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr (fileno(stdin), TCSANOW, &new_term_attr);

    struct timespec tsp = {0,500};

    for(;;) {

        nanosleep(&tsp, NULL);

        int key = fgetc (stdin);
        int complete[3] = {0,0,0};
        int last = 0;

        if (key!=-1) {

            for (;;) {

                if (last < 3) complete[last++] = key;

                int new = fgetc(stdin);
                if (new == -1)
                    break;
                else
                    key = new;
            }

            if (!key_pressed (complete)) break;
            last = 0;
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

            if (box_selected && curr != ' ') printf("\033[1m");
            printf("\033[%d;%dH%c",i+term_pos_y, j+term_pos_x,curr);
            if (box_selected && curr != ' ') printf("\033[0m");
        }
    }
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
