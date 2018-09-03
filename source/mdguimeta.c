#include "mdguimeta.h"

void MDGUIMD__draw_meta_box (unsigned int total_samples,
                             unsigned int sample_rate,
                             unsigned int channels,
                             unsigned int bps,
                             bool box_selected,
                             int term_pos_x, int term_pos_y,
                             int width, int height) {

    MDGUI__draw_box (box_selected, term_pos_x, term_pos_y, width, height);

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
