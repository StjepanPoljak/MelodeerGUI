#ifndef MDGUIMB_H

#include "mdguibox.h"

#include <pthread.h>

#include <melodeer/mdcore.h>
#include <melodeer/mdutils.h>

struct MDGUIMB__FFT {

    float offset_seconds;
    float *fft_sample;
    struct MDGUIMB__FFT *next;
};

enum MDGUIMB__state { MDGUIMB__NONE, MDGUIMB__FFT, MDGUIMB__ALERT };

typedef enum MDGUIMB__state MDGUIMB__state_t;

struct MDGUI__meta_box {

    MDGUI__box_t box;

    bool metadata_present;
    MD__metadata_t metadata;

    int prev_state;

    volatile bool end_signal;
    float curr_sec;
    volatile bool pause;

    float total_seconds;

    int hours;
    int minutes;
    int seconds;

    int curr_hours;
    int curr_minutes;
    int curr_seconds;

    bool resized;

    struct MDGUIMB__FFT *fft_first;
    struct MDGUIMB__FFT *fft_last;
};

typedef struct MDGUI__meta_box MDGUI__meta_box_t;

MDGUI__meta_box_t   MDGUIMB__create     (char *name, int x, int y, int height, int width);

void    MDGUIMB__load                   (MDGUI__meta_box_t *metabox, MD__metadata_t metadata);

void    MDGUIMB__draw                   (MDGUI__meta_box_t *metabox);

void    MDGUIMB__redraw                 (MDGUI__meta_box_t *metabox);

void    MDGUIMB__draw_contents          (MDGUI__meta_box_t *metabox);

void    MDGUIMB__draw_progress_bar      (MDGUI__meta_box_t *metabox);

void    MDGUIMB__reset_variables        (MDGUI__meta_box_t *metabox);

void    MDGUIMB__draw_time              (MDGUI__meta_box_t *metabox);

void    MDGUIMB__draw_fft               (MDGUI__meta_box_t *metabox);

void    MDGUIMB__unload                 (MDGUI__meta_box_t *metabox);

void    MDGUIMB__start_countdown        (MDGUI__meta_box_t *metabox);

void    MDGUIMB__set_pause              (MDGUI__meta_box_t *metabox);

void    MDGUIMB__unset_pause            (MDGUI__meta_box_t *metabox);

void    MDGUIMB__fft_queue              (MDGUI__meta_box_t *metabox, float *sample, float seconds);

void    MDGUIMB__erase_fft_data         (MDGUI__meta_box_t *metabox);

void    MDGUIMB__deinit                 (MDGUI__meta_box_t *metabox);

#endif

#define MDGUIMB_H
