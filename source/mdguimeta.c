#include "mdguimeta.h"

#include "mdguistrarr.h"

void MDGUIMB__draw_contents (MDGUI__meta_box_t *metabox);

MDGUI__meta_box_t MDGUIMB__create (char *name, int x, int y, int height, int width) {

    MDGUI__meta_box_t new_box;

    new_box.metadata_present = false;

    new_box.box = MDGUI__box_create (name, x, y, height, width);

    return new_box;
}

void MDGUIMB__draw (MDGUI__meta_box_t *metabox) {

    MDGUI__draw_box (&metabox->box);
    MDGUIMB__draw_contents (metabox);
}

void MDGUIMB__redraw (MDGUI__meta_box_t *metabox) {

    MDGUI__draw_box_opt (&metabox->box, true);
    MDGUIMB__draw_contents (metabox);
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

    unsigned int total_seconds  = total_samples / sample_rate;
    unsigned int hours          = total_seconds / 3600;
    unsigned int minutes        = (total_seconds / 60) - (hours * 60);
    unsigned int seconds        = total_seconds - 60 * minutes;

    unsigned int data_size      = total_samples * (bps / 8) * channels;
    float kb                    = data_size / 1024;
    float mb                    = kb / 1024;
    bool data_test              = mb >= 1.0;

    unsigned int sample_rate_string_size = snprintf(NULL, 0, "%d Hz %d bps", sample_rate, bps) + 1;
    char *sample_rate_string = malloc (sizeof(*sample_rate_string) * sample_rate_string_size);
    snprintf (sample_rate_string, sample_rate_string_size, "%d Hz %d bps", sample_rate, bps);

    char *channels_string = (channels == 2) ? "Stereo" : (channels == 1 ? "Mono" : "Unknown");
    unsigned int channels_string_size = MDGUI__get_string_size (channels_string);

    unsigned int data_size_string_size = snprintf(NULL, 0, "%.2f %s", data_test ? mb : kb, data_test ? "Mb" : "Kb") + 1;
    char *data_size_string = malloc (sizeof(*data_size_string) * data_size_string_size);
    snprintf (data_size_string, data_size_string_size, "%.2f %s", data_test ? mb : kb, data_test ? "Mb" : "Kb");

    mvprintw (term_pos_y + 1, term_pos_x + (width - (sample_rate_string_size - 1)) / 2, "%s", sample_rate_string);

    mvprintw (term_pos_y + 3, term_pos_x + (width - channels_string_size + 1) / 2, "%s", channels_string);

    move (term_pos_y + 5, term_pos_x + 1);

    if (hours > 0)      printw("%dh ", hours);
    if (minutes > 0)    printw("%dm ", minutes);
    if (seconds > 0)    printw("%ds", seconds);

    mvprintw (term_pos_y + 5, term_pos_x + width - data_size_string_size - 1, "%s" ,data_size_string);

    refresh ();

    free (sample_rate_string);
    free (data_size_string);

}

void MDGUIMB__load (MDGUI__meta_box_t *metabox, MD__metadata_t metadata) {

    metabox->metadata_present = true;
    metabox->metadata = metadata;

    MDGUIMB__redraw (metabox);
}

void MDGUIMB__unload (MDGUI__meta_box_t *metabox) {
    
    metabox->metadata_present = false;

    MDGUIMB__redraw (metabox);
}

void MDGUIMB__deinit (MDGUI__meta_box_t *metabox) {

    MDGUI__box_deinit (&metabox->box);
}
