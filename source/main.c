#include "mdgui.h"



void            MD__handle_metadata     (MD__metadata_t metadata);
unsigned int    MD__get_seconds         (volatile MD__buffer_chunk_t *curr_chunk,
                                         unsigned int sample_rate,
                                         unsigned int channels,
                                         unsigned int bps);
void transform (volatile MD__buffer_chunk_t *curr_chunk,
             unsigned int sample_rate,
             unsigned int channels,
             unsigned int bps);

MDGUI__manager_t mdgui;

void MDGUI__play_complete ();

void MDGUI__started_playing () {

    // mdgui.current_play_state = MDGUI__PLAYING;

    // redraw_playlist_box ();

    char buff[PATH_MAX + 10];

    //snprintf (buff, PATH_MAX + 10, "Playing: %s", MDGUI__playlist[MDGUI__playlist_current - 1]);

    MDGUI__log(buff, mdgui.tinfo);

    return;
}

void MDGUI__handle_error (char *error) {

    MDGUI__log (error, mdgui.tinfo);
    // current_play_state = MDGUI__NOT_PLAYING;

    return;
}

char *will_play = NULL;


int main (int argc, char *argv[]) {

    MDGUI__init (&mdgui);

    MDGUI__start (&mdgui);

    MDGUI__deinit (&mdgui);

    return 0;
}


void MDGUI__play_complete () {

    // MDGUI__mutex_lock (&mdgui);

    // if (mdgui.stop_all_signal) {

    //     mdgui.stop_all_signal = false;

    //     mdgui.current_play_state = MDGUI__NOT_PLAYING;

    //     mdgui.metabox.metadata_present = false;

    //     MDGUI__mutex_unlock (&mdgui);

    //     return;
    // }

    // if (mdgui.current_play_state == MDGUI__PROGRAM_EXIT) {

    //     mdgui.current_play_state = MDGUI__READY_TO_EXIT;

    //     MDGUI__mutex_unlock (&mdgui);

    //     return;
    // }

    // MDGUI__log ("Done playing!", mdgui.tinfo);

    // mdgui.current_play_state = MDGUI__NOT_PLAYING;
    // mdgui.metabox.metadata_present = false;

    // MDGUI__mutex_unlock (&mdgui);
    // // MDGUI__draw_meta_box_wrap ();

    // MDGUI__start_playing ();

    return;
}

void MD__handle_metadata (MD__metadata_t metadata) {

    mdgui.metabox.metadata = metadata;
    mdgui.metabox.metadata_present = true;
}

void transform (volatile MD__buffer_chunk_t *curr_chunk,
                unsigned int sample_rate,
                unsigned int channels,
                unsigned int bps) {

    for (int i=0; i<curr_chunk->size/((bps/8)*channels); i++) {

        for (int c=0; c<channels; c++) {

            // this will depend on bps...
            short data = 0;

            for (int b=0; b<bps/8; b++) {

                data = data + ((short)(curr_chunk->chunk[i*channels*(bps/8)+(c*channels)+b])<<(8*b));
            }

            // transform

            for (int b=0; b<bps/8; b++) {

                curr_chunk->chunk[i*channels*(bps/8)+(c*channels)+b] = data >> 8*b;
            }
        }
    }
}

unsigned int MD__get_seconds (volatile MD__buffer_chunk_t *curr_chunk,
                              unsigned int sample_rate,
                              unsigned int channels,
                              unsigned int bps) {

    return (curr_chunk->order * curr_chunk->size / (bps * channels / 8)) / sample_rate;

}
