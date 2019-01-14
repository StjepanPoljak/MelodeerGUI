#include "mdgui.h"

MDGUI__manager_t mdgui;

int main (int argc, char *argv[]) {

    MDGUI__init (&mdgui);

    MDGUI__set_refresh_rate (&mdgui, 50000);

    MDGUI__start (&mdgui);

    MDGUI__deinit (&mdgui);

    return EXIT_SUCCESS;
}
