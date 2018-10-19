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

int main (int argc, char *argv[]) {

    MDGUI__init (&mdgui);

    MDGUI__start (&mdgui);

    MDGUI__deinit (&mdgui);

    return 0;
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
