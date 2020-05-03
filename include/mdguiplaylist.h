#ifndef MDGUIPLAYLIST_H

#include "mdguilistbox.h"

struct MDGUI__playlist_box {
	MDGUI__listbox_t listbox;
	MDGUI__str_array_t filenames;
	int num_playing;
};

typedef struct MDGUI__playlist_box MDGUI__playlist_box_t;

MDGUI__playlist_box_t MDGUIPB__create	(char *name, int x, int y,
					 int height, int width);
void	MDGUIPB__draw			(MDGUI__playlist_box_t *playlistbox);
void	MDGUIPB__redraw			(MDGUI__playlist_box_t *playlistbox);
char*	MDGUIPB__get_curr_filename	(MDGUI__playlist_box_t *playlistbox);
void	MDGUIPB__append			(MDGUI__playlist_box_t *playlistbox,
					 char* filename);
void	MDGUIPB__delete			(MDGUI__playlist_box_t *playlistbox,
					 int to_delete);
void	MDGUIPB__deinit			(MDGUI__playlist_box_t *playlistbox);

#endif

#define MDGUIPLAYLIST_H
