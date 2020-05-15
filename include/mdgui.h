#ifndef MDGUI_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <pthread.h>

#include <melodeer/mdcore.h>

#include "mdguistrarr.h"
#include "mdguifilebox.h"
#include "mdguimeta.h"
#include "mdguiplaylist.h"

struct MDGUI__terminal {
	int cols;
	int lines;
};

typedef struct  MDGUI__terminal MDGUI__terminal;

enum MDGUI__component {
	MDGUI__NONE,
	MDGUI__FILEBOX,
	MDGUI__METABOX,
	MDGUI__PLAYLIST,
	MDGUI__LOGO,

	MDGUI__META_ALERT
};

typedef enum MDGUI__component MDGUI__component;

enum MDGUI__play_state {
	MDGUI__NOT_PLAYING,
	MDGUI__PAUSE,
	MDGUI__PLAYING,
	MDGUI__WAITING_TO_STOP,
	MDGUI__INITIALIZING,
	MDGUI__PROGRAM_EXIT,
	MDGUI__READY_TO_EXIT
};

typedef enum MDGUI__play_state MDGUI__play_state;

struct MDGUI__event {
	bool (*event) (void *data);
	void *data;
};

typedef struct MDGUI__event MDGUI__event_t;

struct MDGUI__manager {

	MDGUI__event_t **event_queue;
	int event_queue_last;
	int event_queue_size;

	MD__file_t *curr_playing;

	MDGUI__terminal tinfo;

	MDGUI__component selected_component;
	MDGUI__component potential_component;
	MDGUI__component previous_potential_component;

	pthread_t melodeer_thread;
	pthread_t terminal_thread;
	pthread_t keyboard_input_thread;
	pthread_t clock_thread;

	pthread_mutex_t mutex;
	pthread_mutexattr_t mutex_attributes;

	MDGUI__file_box_t filebox;
	MDGUI__playlist_box_t playlistbox;
	MDGUI__meta_box_t metabox;

	char *playlist_dir;

	MDGUI__play_state volatile current_play_state;

	bool stop_all_signal;

	int (*get_next)(int curr, int size);

	// related to window positions and padding

	int top;
	int bottom;
	int left;
	int right;
	int meta_top;
	int meta_bottom;

	int refresh_rate;

	int max_events;
};

typedef struct MDGUI__manager MDGUI__manager_t;

bool	MDGUI__init		(MDGUI__manager_t *mdgui);
void	MDGUI__set_refresh_rate	(MDGUI__manager_t *mdgui,
				 int refresh_rate);
bool	MDGUI__start		(MDGUI__manager_t *mdgui);
void	MDGUI__log		(const char *log,
				 MDGUI__manager_t *mdgui);
void	MDGUI__draw		(MDGUI__manager_t *mdgui);
void	MDGUI__mutex_lock	(MDGUI__manager_t *mdgui);
void	MDGUI__mutex_unlock	(MDGUI__manager_t *mdgui);

MDGUI__terminal MDGUI__get_terminal_information ();

void	MDGUI__deinit		(MDGUI__manager_t *mdgui);

#endif

#define MDGUI_H
