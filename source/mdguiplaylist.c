#include "mdguiplaylist.h"

MDGUI__playlist_box_t MDGUIPB__create (char *name, int x, int y, int height, int width) {

    MDGUI__playlist_box_t new_pb;

    new_pb.listbox = MDGUILB__create (name, x, y, height, width, false);

    new_pb.num_playing = -1;

    MDGUI__str_array_init (&new_pb.filenames);

    return new_pb;
}

void MDGUI__str_transform_fname (void *data, char *src, char **dest) {

    int src_size = MDGUI__get_string_size (src) + 1;

    if (src_size <= 2) return;

    int last_slash = -1;

    for (int i=src_size-1; i >= 0; i--) if (src[i] == '/' && i != src_size - 2) {

    	last_slash = i;

    	break;
    }

    if (last_slash == -1) return;

    *dest = malloc (sizeof (**dest) * (last_slash + 1));

    for (int i=0; i<last_slash; i++) (*dest)[i] = src[i];

    (*dest)[last_slash] = 0;

    return;
}

void MDGUIPB__draw (MDGUI__playlist_box_t *playlistbox) {

    MDGUILB__draw (&playlistbox->listbox, playlistbox->num_playing);
}

void MDGUIPB__redraw (MDGUI__playlist_box_t *playlistbox) {

    MDGUILB__redraw (&playlistbox->listbox, playlistbox->num_playing);
}

void MDGUIPB__update (MDGUI__playlist_box_t *playlistbox, void (*transform)(char *src, char **dest)) {

    MDGUI__str_array_copy_raw (&playlistbox->filenames, &playlistbox->listbox.str_array,
                               0, playlistbox->filenames.cnum - 1, NULL, MDGUI__str_transform_fname);

    return;
}

void MDGUIPB__deinit (MDGUI__playlist_box_t *playlistbox) {

    MDGUI__str_array_deinit (&playlistbox->filenames);

    MDGUILB__deinit (&playlistbox->listbox);

    return;
}