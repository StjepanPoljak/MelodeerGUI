#include "mdguimeta.h"
#include <unistd.h>

#include "mdguistrarr.h"

void MDGUIMB__draw_contents (MDGUI__meta_box_t *metabox);
void MDGUIMB__draw_progress_bar (MDGUI__meta_box_t *metabox);
void MDGUIMB__reset_variables (MDGUI__meta_box_t *metabox);

MDGUI__meta_box_t MDGUIMB__create (char *name, int x, int y, int height, int width) {

    MDGUI__meta_box_t new_box;

    MDGUIMB__reset_variables (&new_box);

    pthread_mutex_init (&new_box.mutex, NULL);

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
}

void MDGUIMB__draw (MDGUI__meta_box_t *metabox) {

    MDGUI__draw_box (&metabox->box);
    MDGUIMB__draw_contents (metabox);
    
    metabox->prev_state = -1;
    
    MDGUIMB__draw_progress_bar (metabox);
}

void MDGUIMB__redraw (MDGUI__meta_box_t *metabox) {

    MDGUI__draw_box_opt (&metabox->box, true);
    MDGUIMB__draw_contents (metabox);
    MDGUIMB__draw_progress_bar (metabox);
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

    // unsigned int total_seconds  = total_samples / sample_rate;
    // unsigned int hours          = total_seconds / 3600;
    // unsigned int minutes        = (total_seconds / 60) - (hours * 60);
    // unsigned int seconds        = total_seconds - 60 * minutes;
    //
    // unsigned int data_size      = total_samples * (bps / 8) * channels;
    // float kb                     = data_size / 1024;
    // float mb                     = kb / 1024;
    // bool data_test              = mb >= 1.0;

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

void *MDGUIMB__countdown (void *data) {

    MDGUI__meta_box_t *metabox = (MDGUI__meta_box_t *)data;

    while (true) {

        usleep (50000);

        pthread_mutex_lock (&metabox->mutex);

        if (metabox->pause) {

            pthread_mutex_unlock (&metabox->mutex);
            continue;
        }

        metabox->curr_sec += 0.05;

        MDGUIMB__draw_progress_bar (metabox);

        if (metabox->end_signal) {

            MDGUIMB__reset_variables (metabox);
            pthread_mutex_unlock (&metabox->mutex);

            break;
        }

        pthread_mutex_unlock (&metabox->mutex);
    }
}

void MDGUIMB__start_countdown (MDGUI__meta_box_t *metabox) {

    metabox->curr_sec = 0;

    if (pthread_create (&metabox->clock_thread, NULL, MDGUIMB__countdown, metabox)){

        return;
    }
}


void MDGUIMB__draw_progress_bar (MDGUI__meta_box_t *metabox) {

    if (metabox->metadata.total_samples == 0 || metabox->metadata.sample_rate == 0) return;

    float absolute_progress = metabox->curr_sec / metabox->total_seconds;

    int boxes = (metabox->total_seconds - metabox->curr_sec < 1) && metabox->metadata_present
              ? metabox->box.width - 2
              : absolute_progress * (metabox->box.width - 2);

    if (boxes == metabox->prev_state && metabox->metadata_present) return;

    // NOTE: add previous state to redraw only on need

    for (int i=0; i<metabox->box.width - 2; i++) {

        char curr = i < boxes || (i == 0 && metabox->metadata_present) ? '#' : '-';

        mvprintw (metabox->box.height + metabox->box.y + 1, metabox->box.x + i + 1, "%c", curr);
    }

    metabox->prev_state = boxes;

    refresh ();
}

void MDGUIMB__load (MDGUI__meta_box_t *metabox, MD__metadata_t metadata) {

    MDGUIMB__reset_variables (metabox);

    metabox->metadata_present = true;
    metabox->metadata = metadata;

    metabox->total_seconds = (float)metabox->metadata.total_samples / (float)metabox->metadata.sample_rate;

    MDGUIMB__redraw (metabox);
}


void MDGUIMB__unset_pause (MDGUI__meta_box_t *metabox) {

    pthread_mutex_lock (&metabox->mutex);
    metabox->pause = false;
    pthread_mutex_unlock (&metabox->mutex);
}


void MDGUIMB__set_pause (MDGUI__meta_box_t *metabox) {

    pthread_mutex_lock (&metabox->mutex);
    metabox->pause = true;
    pthread_mutex_unlock (&metabox->mutex);
}


void MDGUIMB__unload (MDGUI__meta_box_t *metabox) {

    pthread_mutex_lock (&metabox->mutex);
    metabox->end_signal = true;
    pthread_mutex_unlock (&metabox->mutex);

    pthread_join (metabox->clock_thread, NULL);

    MDGUIMB__reset_variables (metabox);

    MDGUIMB__redraw (metabox);
}

void MDGUIMB__deinit (MDGUI__meta_box_t *metabox) {

    pthread_mutex_destroy (&metabox->mutex);

    MDGUI__box_deinit (&metabox->box);
}
