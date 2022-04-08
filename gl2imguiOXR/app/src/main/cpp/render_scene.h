#pragma once

typedef struct scene_data_t
{
    uint32_t            elapsed_us;
    std::vector<XrView> views;
    uint32_t            viewID;
} scene_data_t;


int init_gles_scene ();
int render_gles_scene (XrCompositionLayerProjectionView &layerView,
                       render_target_t &rtarget, XrPosef &viewPose, XrPosef &stagePose,
                       scene_data_t &sceneData);
