#ifndef RENDER_TEXPLATE_H_
#define RENDER_TEXPLATE_H_

#include "util_render2d.h"

int init_texplate ();
int draw_tex_plate (int texid, float *matPVM, int upsidedown);
int hittest_tex_plate (float *matVM, float *ray0, float *ray1, float *out);

#endif
