#include "mdgui.h"

pthread_mutex_t MDGUI__mutex;

void MDGUI__clear_screen () {

    clear();
}

void MDGUI__log (const char *log, MDGUI__terminal tinfo) {

    move (tinfo.lines - 1, 0);
    clrtoeol ();

    int i = 0;

    while (true) {

        if (i >= tinfo.cols) break;

        if (log[i] == 0) break;

        mvprintw (tinfo.lines - 1, i, "%c", log[i]);

        i++;
    }

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
