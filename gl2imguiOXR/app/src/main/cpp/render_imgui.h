/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef UTIL_IMGUI_H_
#define UTIL_IMGUI_H_

#include "util_oxr.h"

typedef struct _imgui_data_t
{
    int   elapsed_ms;
    float interval_ms;

    XrRect2Di           viewport;
    std::vector<XrView> views;

} imgui_data_t;

int  init_imgui (int width, int height);
void imgui_mousebutton (int button, int state, int x, int y);
void imgui_mousemove (int x, int y);

int invoke_imgui (imgui_data_t *imgui_data);

#endif /* UTIL_IMGUI_H_ */
 