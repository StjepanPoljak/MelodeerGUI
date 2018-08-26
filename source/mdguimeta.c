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
    snprintf(sample_rate_string, sample_rate_string_size, "%d Hz %d bps", sample_rate, bps);

    char *channels_string = (channels == 2) ? "Stereo" : (channels == 1 ? "Mono" : "Unknown");
    unsigned int channels_string_size = MDGUI__get_string_size (channels_string);

    unsigned int data_size_string_size = snprintf(NULL, 0, "%.2f %s", data_test ? mb : kb, data_test ? "Mb" : "Kb") + 1;
    char *data_size_string = malloc (sizeof(*data_size_string) * data_size_string_size);
    snprintf(data_size_string, data_size_string_size, "%.2f %s", data_test ? mb : kb, data_test ? "Mb" : "Kb");

    printf("\033[%d;%dH%s", term_pos_y + 1, term_pos_x + (width - (sample_rate_string_size - 1)) / 2, sample_rate_string);
    printf("\033[%d;%dH%s", term_pos_y + 3, term_pos_x + (width - channels_string_size + 1) / 2, channels_string);
    // printf("\033[%d;%dH %d bps", term_pos_y + 3, term_pos_x + 1, bps);
    printf("\033[%d;%dH ", term_pos_y + 5, term_pos_x + 1);
    if (hours > 0)      printf("%dh ", hours);
    if (minutes > 0)    printf("%dm ", minutes);
    if (seconds > 0)    printf("%ds", seconds);

    printf("\033[%d;%dH%s", term_pos_y + 5, term_pos_x + width - data_size_string_size - 1, data_size_string);

    free (sample_rate_string);
    free (data_size_string);
}
