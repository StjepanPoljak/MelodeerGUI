#ifndef MDGUIPLAYLIST_H

#include "mdguilistbox.h"

struct MDGUI__playlist_box {

	MDGUI__listbox_t listbox;

	MDGUI__str_array_t filenames;

	int num_playing;
};

typedef struct MDGUI__playlist_box MDGUI__playlist_box_t;

MDGUI__playlist_box_t MDGUIPB__create (char *name, int x, int y, int height, int width);

void MDGUIPB__draw (MDGUI__playlist_box_t *playlistbox);

void MDGUIPB__redraw (MDGUI__playlist_box_t *playlistbox);

void MDGUIPB__update (MDGUI__playlist_box_t *playlistbox, void (*transform)(char *, char **));

void MDGUIPB__deinit (MDGUI__playlist_box_t *playlistbox);

#endif

#define MDGUIPLAYLIST_H