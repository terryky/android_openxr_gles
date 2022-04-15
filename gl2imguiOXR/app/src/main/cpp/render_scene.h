#pragma once
#include <string>
#include "app_engine.h"

typedef struct scene_data_t
{
    std::string         runtime_name;
    std::string         system_name;
    const GLubyte       *gl_version;
    const GLubyte       *gl_vendor;
    const GLubyte       *gl_render;

    uint32_t            elapsed_us;
    float               interval_ms;

    XrRect2Di           viewport;
    std::vector<XrView> views;
    uint32_t            viewID;
} scene_data_t;


int init_gles_scene ();
int render_gles_scene (XrCompositionLayerProjectionView &layerView,
                       render_target_t &rtarget, XrPosef &viewPose, XrPosef &stagePose,
                       scene_data_t &sceneData);
