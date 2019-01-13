#include "mdguimeta.h"

#include <unistd.h>
#include <limits.h>

#include "mdguistrarr.h"

MDGUI__meta_box_t MDGUIMB__create (char *name, int x, int y, int height, int width) {

    MDGUI__meta_box_t new_box;

    new_box.fft_first = NULL;
    new_box.fft_last = NULL;

    MDGUIMB__reset_variables (&new_box);

    new_box.box = MDGUI__box_create (name, x, y, height, width);

    return new_box;
}

void MDGUIMB__reset_variables (MDGUI__meta_box_t *metabox) {

    metabox->metadata_present = false;
    metabox->pause = false;
    metabox->total_seconds = 0;
    metabox->end_signal = false;
    metabox->curr_sec = 0;
    metabox->prev_state = -1;

    metabox->curr_seconds = 0;
    metabox->curr_minutes = 0;
    metabox->curr_hours = 0;

    metabox->hours = 0;
    metabox->minutes = 0;
    metabox->seconds = 0;
}

void MDGUIMB__draw (MDGUI__meta_box_t *metabox) {

    MDGUI__draw_box (&metabox->box);
    MDGUIMB__draw_contents (metabox);

    metabox->prev_state = -1;
    metabox->resized = true;

    MDGUIMB__draw_progress_bar (metabox);

    metabox->resized = true;

    MDGUIMB__draw_time (metabox);
}

void MDGUIMB__redraw (MDGUI__meta_box_t *metabox) {

    MDGUI__draw_box_opt (&metabox->box, true);
    MDGUIMB__draw_contents (metabox);

    MDGUIMB__draw_progress_bar (metabox);
    MDGUIMB__draw_time (metabox);
}

void MDGUIMB__draw_time (MDGUI__meta_box_t *metabox) {

    if (metabox->total_seconds == 0 || !metabox->metadata_present) return;

    int curr_hours      = (int)(metabox->curr_sec / 3600);
    int curr_minutes    = (int)((metabox->curr_sec / 60)
                        - (float)(curr_hours * 60));
    int curr_seconds    = (int)(metabox->curr_sec
                        - (float)(60 * curr_minutes));

    if (curr_hours != metabox->curr_hours
     || curr_minutes != metabox->curr_minutes
     || curr_seconds != metabox->curr_seconds
     || metabox->resized) {

        metabox->curr_hours = curr_hours;
        metabox->curr_minutes = curr_minutes;
        metabox->curr_seconds = curr_seconds;

        metabox->resized = false;
    }
    else return;

    char *total_string = NULL;

    int total_string_size = 0;

    if (metabox->hours > 0) {

        int total_string_size = snprintf (NULL, 0, "%d:%02d:%02d", metabox->hours, metabox->minutes, metabox->seconds) + 1;
        total_string = malloc(sizeof(*total_string) * total_string_size);
        snprintf (total_string, total_string_size, "%d:%02d:%02d", metabox->hours, metabox->minutes, metabox->seconds);
    }
    else {

        int total_string_size = snprintf (NULL, 0, "%d:%02d", metabox->minutes, metabox->seconds) + 1;
        total_string = malloc(sizeof(*total_string) * total_string_size);
        snprintf (total_string, total_string_size, "%d:%02d", metabox->minutes, metabox->seconds);
    }

    char *final_string = NULL;
    int final_string_size = 0;

    if (curr_hours > 0) {

        final_string_size = snprintf (NULL, 0, "%d:%02d:%02d / %s", curr_hours, curr_minutes, curr_seconds, total_string) + 1;
        final_string = malloc(sizeof(*final_string) * final_string_size);
        snprintf (final_string, final_string_size, "%d:%02d:%02d / %s", curr_hours, curr_minutes, curr_seconds, total_string);
    }
    else {

        final_string_size = snprintf (NULL, 0, "%d:%02d / %s", curr_minutes, curr_seconds, total_string) + 1;
        final_string = malloc(sizeof(*final_string) * final_string_size);
        snprintf (final_string, final_string_size, "%d:%02d / %s", curr_minutes, curr_seconds, total_string);
    }

    if (final_string_size < metabox->box.width - 2) {

        mvprintw (metabox->box.y + metabox->box.height - 1, metabox->box.x + (metabox->box.width - (final_string_size + 1)) / 2, " %s ", final_string);
        refresh ();
    }

    if (final_string) free (final_string);
    if (total_string) free (total_string);
}

void MDGUIMB__draw_contents (MDGUI__meta_box_t *metabox) {

    if (!metabox->metadata_present) return;

    unsigned int total_samples  = metabox->metadata.total_samples;
    unsigned int channels = metabox->metadata.channels;
    unsigned int bps = metabox->metadata.bps;
    unsigned int sample_rate = metabox->metadata.sample_rate;

    int term_pos_x = metabox->box.x;
    int term_pos_y = metabox->box.y;
    int width = metabox->box.width;

    char *channels_string = (channels == 2) ? "Stereo" : (channels == 1 ? "Mono" : "Unknown");

    char *sample_rate_string = NULL;

    unsigned int sample_rate_string_size = snprintf (NULL, 0, "%dHz %dbps %s", sample_rate, bps, channels_string) + 1;

    if (sample_rate_string_size < metabox->box.width - 2) {

        sample_rate_string = malloc (sizeof(*sample_rate_string) * sample_rate_string_size);
        snprintf (sample_rate_string, sample_rate_string_size, "%dHz %dbps %s", sample_rate, bps, channels_string);
    }
    else {

        sample_rate_string_size = snprintf (NULL, 0, "%dHz %dbps", sample_rate, bps) + 1;

        if (sample_rate_string_size < metabox->box.width - 2) {

            sample_rate_string = malloc (sizeof(*sample_rate_string) * sample_rate_string_size);
            snprintf (sample_rate_string, sample_rate_string_size, "%dHz %dbps", sample_rate, bps);
        }
    }

    if (sample_rate_string) mvprintw (term_pos_y + metabox->box.height - 3, term_pos_x + (width - (sample_rate_string_size - 1)) / 2, "%s", sample_rate_string);

    refresh ();

    free (sample_rate_string);
}

void MDGUIMB__erase_fft_data (MDGUI__meta_box_t *metabox) {

    struct MDGUIMB__FFT *fft_curr = metabox->fft_first;

    while (true) {
        
        if (!fft_curr) break;

        struct MDGUIMB__FFT *to_delete = fft_curr;
        fft_curr = fft_curr->next;

        free (to_delete->fft_sample);
        free (to_delete);
    }

    metabox->fft_first = NULL;
    metabox->fft_last = NULL;

    return;
}

void MDGUIMB__start_countdown (MDGUI__meta_box_t *metabox) {

    metabox->curr_sec = 0;
}

void MDGUIMB__draw_progress_bar (MDGUI__meta_box_t *metabox) {

    if (metabox->metadata.total_samples == 0 || metabox->metadata.sample_rate == 0) return;

    float absolute_progress = metabox->curr_sec / metabox->total_seconds;

    int boxes = (metabox->total_seconds - metabox->curr_sec < 1) && metabox->metadata_present
              ? metabox->box.width - 2
              : absolute_progress * (metabox->box.width - 2);

    if (boxes == metabox->prev_state && metabox->metadata_present && !metabox->resized) return;

    metabox->resized = false;

    for (int i=0; i<metabox->box.width - 2; i++) {

        char curr = i < boxes ? '#' : '-';

        mvprintw (metabox->box.height + metabox->box.y + 1, metabox->box.x + i + 1, "%c", curr);
    }

    metabox->prev_state = boxes;

    refresh ();
}

void MDGUIMB__load (MDGUI__meta_box_t *metabox, MD__metadata_t metadata) {

    MDGUIMB__reset_variables (metabox);

    metabox->metadata = metadata;

    metabox->total_seconds  = (float)metabox->metadata.total_samples / (float)metabox->metadata.sample_rate;

    metabox->hours          = (int)(metabox->total_seconds / 3600);
    metabox->minutes        = (int)((metabox->total_seconds / 60) - (float)(metabox->hours * 60));
    metabox->seconds        = (int)(metabox->total_seconds - (float)(60 * metabox->minutes));

    metabox->metadata_present = true;

    MDGUIMB__redraw (metabox);
}


void MDGUIMB__unset_pause (MDGUI__meta_box_t *metabox) {

    metabox->pause = false;
}


void MDGUIMB__set_pause (MDGUI__meta_box_t *metabox) {

    metabox->pause = true;
}


void MDGUIMB__unload (MDGUI__meta_box_t *metabox) {

    metabox->end_signal = true;

    if (metabox->pause) metabox->pause = false;

    MDGUIMB__reset_variables (metabox);
    
    MDGUI__draw_box_opt (&metabox->box, true);

    MDGUIMB__draw (metabox);
}

void MDGUIMB__draw_fft (MDGUI__meta_box_t *metabox) {

    if (metabox->box.width < 21) return;

    if (!metabox->fft_last) return;

    struct MDGUIMB__FFT *fft_curr = metabox->fft_last;

    float *new_sample = fft_curr->fft_sample;

    int fft_height = metabox->box.height - 6;

    for (int col = 0; col < 16; col += 2) {

        for (int row = 0; row < fft_height; row++){

            char curr = new_sample[col/2] * (fft_height + 1) >= (fft_height - row) ? '#' : '|';

            mvprintw (metabox->box.y + row + 2, metabox->box.x + (metabox->box.width - 16)/2 + col + 1, "%c", curr);
        }
    }

    refresh ();
}

void MDGUIMB__deinit (MDGUI__meta_box_t *metabox) {

    MDGUI__box_deinit (&metabox->box);
}

void MDGUIMB__fft_queue (MDGUI__meta_box_t *metabox, float *sample, float seconds) {

    struct MDGUIMB__FFT *new_el = malloc (sizeof (*new_el));

    new_el->fft_sample = sample;
    new_el->offset_seconds = seconds;
    new_el->next = NULL;

    if (!metabox->fft_first) {

        metabox->fft_first = new_el;
        metabox->fft_last = new_el;
    }
    else {
        metabox->fft_last->next = new_el;
        metabox->fft_last = new_el;
    }
}
