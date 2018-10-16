#ifndef MDGUIMB_H

#include "mdguibox.h"
#include <melodeer/mdcore.h>

struct MDGUI__meta_box {

    MDGUI__box_t box;

    bool metadata_present;
    MD__metadata_t metadata;
};

typedef struct MDGUI__meta_box MDGUI__meta_box_t;

MDGUI__meta_box_t   MDGUIMB__create     (char *name, int x, int y, int height, int width);

void    MDGUIMB__draw                   (MDGUI__meta_box_t *metabox);

void    MDGUIMB__deinit                 ();

#endif

#define MDGUIMB_H
