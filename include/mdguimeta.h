#ifndef MDGUIMB_H

#include "mdguibox.h"

#include <pthread.h>

#include <melodeer/mdcore.h>

struct MDGUI__meta_box {

    MDGUI__box_t box;

    bool metadata_present;
    MD__metadata_t metadata;

    int prev_state;

    volatile bool end_signal;
    float curr_sec;
    volatile bool pause;

    float total_seconds;

    pthread_t clock_thread;
    pthread_mutex_t mutex;
};

typedef struct MDGUI__meta_box MDGUI__meta_box_t;

MDGUI__meta_box_t   MDGUIMB__create     (char *name, int x, int y, int height, int width);

void    MDGUIMB__load                   (MDGUI__meta_box_t *metabox, MD__metadata_t metadata);

void    MDGUIMB__draw                   (MDGUI__meta_box_t *metabox);

void    MDGUIMB__redraw                 (MDGUI__meta_box_t *metabox);

void    MDGUIMB__unload                 (MDGUI__meta_box_t *metabox);

void    MDGUIMB__start_countdown        (MDGUI__meta_box_t *metabox);

void    MDGUIMB__set_pause              (MDGUI__meta_box_t *metabox);

void    MDGUIMB__unset_pause            (MDGUI__meta_box_t *metabox);

void    MDGUIMB__deinit                 (MDGUI__meta_box_t *metabox);

#endif

#define MDGUIMB_H
