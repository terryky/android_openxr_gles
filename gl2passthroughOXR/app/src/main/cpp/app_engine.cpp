#include "util_egl.h"
#include "util_oxr.h"
#include "app_engine.h"
#include "render_scene.h"


AppEngine::AppEngine (android_app* app)
    : m_app(app)
{
}

AppEngine::~AppEngine()
{
}


/* ---------------------------------------------------------------------------- *
 *  Interfaces to android application framework
 * ---------------------------------------------------------------------------- */
struct android_app *
AppEngine::AndroidApp (void) const
{
    return m_app;
}


/* ---------------------------------------------------------------------------- *
 *  Initialize OpenXR with OpenGLES renderer
 * ---------------------------------------------------------------------------- */
void 
AppEngine::InitOpenXR_GLES ()
{
    void *vm    = m_app->activity->vm;
    void *clazz = m_app->activity->clazz;

    oxr_initialize_loader (vm, clazz);

    m_instance = oxr_create_instance (vm, clazz);
    m_systemId = oxr_get_system (m_instance);

    egl_init_with_pbuffer_surface (3, 24, 0, 0, 16, 16);
    oxr_confirm_gfx_requirements (m_instance, m_systemId);

    init_gles_scene ();

    m_session    = oxr_create_session (m_instance, m_systemId);
    m_appSpace   = oxr_create_ref_space (m_session, XR_REFERENCE_SPACE_TYPE_LOCAL);
    m_stageSpace = oxr_create_ref_space (m_session, XR_REFERENCE_SPACE_TYPE_STAGE);
    m_viewSpace  = oxr_create_ref_space (m_session, XR_REFERENCE_SPACE_TYPE_VIEW);

    m_viewSurface = oxr_create_viewsurface (m_instance, m_systemId, m_session);

    InitializeActions ();

    oxr_create_handtrackers (m_instance, m_session, m_handTracker);
    m_handJointLoc[0] = oxr_create_handjoint_loc ();
    m_handJointLoc[1] = oxr_create_handjoint_loc ();

    oxr_create_passthrough_layer (m_instance, m_session, m_passthrough, m_ptLayer);
    oxr_start_passthrough (m_instance, m_passthrough);

    m_runtime_name = oxr_get_runtime_name (m_instance);
    m_system_name  = oxr_get_system_name (m_instance, m_systemId);
}


/* ---------------------------------------------------------------------------- *
 *  Initialize Hand Controller Action
 * ---------------------------------------------------------------------------- *
 *
 * -----------+-----------+-----------------+-------------------------------------
 *  XrAction  | ActionType|  SubactionPath  | BindingPath
 * -----------+-----------+-----------------+-------------------------------------
 *  poseAction| POSE   IN | /user/hand/left | /user/hand/left /input/grip/pose
 *            |           | /user/hand/right| /user/hand/right/input/grip/pose
 *  aimAction | POSE   IN | /user/hand/left | /user/hand/left /input/aim/pose
 *            |           | /user/hand/right| /user/hand/right/input/aim/pose
 *  squzAction| FLOAT  IN | /user/hand/left | /user/hand/left /input/squeeze/value
 *            |           | /user/hand/right| /user/hand/right/input/squeeze/value
 *  trigAction| FLOAT  IN | /user/hand/left | /user/hand/left /input/trigger/value
 *            |           | /user/hand/right| /user/hand/right/input/trigger/value
 *  stikAction| FLOAT  IN | /user/hand/left | /user/hand/left /input/trigger/value
 *            |           | /user/hand/right| /user/hand/right/input/trigger/value
 * -----------+-----------+-----------------+-------------------------------------
 *  vibrAction|VIBRATE OUT| /user/hand/left | /user/hand/left /output/haptic
 *            |           | /user/hand/right| /user/hand/right/output/haptic
 * -----------+-----------+-----------------+-------------------------------------
 *  clksAction| BOOL   IN | /user/hand/left | /user/hand/left /input/thumbstick/click
 *  clksAction| BOOL   IN | /user/hand/right| /user/hand/right/input/thumbstick/click
 *  clkaAction| BOOL   IN |                 | /user/hand/right/input/a/click
 *  clkbAction| BOOL   IN |                 | /user/hand/right/input/b/click
 *  clkxAction| BOOL   IN |                 | /user/hand/left /input/x/click
 *  clkyAction| BOOL   IN |                 | /user/hand/left /input/y/click
 *  quitAction| BOOL   IN |                 | /user/hand/left /input/menu/click
 * -----------+-----------+-----------------+-------------------------------------
 *
 */

#define HANDL       "/user/hand/left"
#define HANDR       "/user/hand/right"
#define HANDL_IN    "/user/hand/left/input"
#define HANDR_IN    "/user/hand/right/input"
#define HANDL_OT    "/user/hand/left/output"
#define HANDR_OT    "/user/hand/right/output"

void
AppEngine::InitializeActions()
{
    m_input.actionSet = oxr_create_actionset (m_instance, "app_action_set", "AppActionSet", 0);

    xrStringToPath (m_instance, HANDL, &m_input.handSubactionPath[Side::LEFT ]);
    xrStringToPath (m_instance, HANDR, &m_input.handSubactionPath[Side::RIGHT]);

    XrActionSet           &actSet  = m_input.actionSet;
    std::array<XrPath, 2> &subPath = m_input.handSubactionPath;

    // Create actions.
    m_input.poseAction = oxr_create_action (actSet, XR_ACTION_TYPE_POSE_INPUT,       "grip_pose","GripPose", 2, subPath.data());
    m_input.aimAction  = oxr_create_action (actSet, XR_ACTION_TYPE_POSE_INPUT,       "aim_pose", "Aim Pose", 2, subPath.data());
    m_input.squzAction = oxr_create_action (actSet, XR_ACTION_TYPE_FLOAT_INPUT,      "squeeze",  "Aqueeze",  2, subPath.data());
    m_input.trigAction = oxr_create_action (actSet, XR_ACTION_TYPE_FLOAT_INPUT,      "trigger",  "Trigger",  2, subPath.data());
    m_input.stikAction = oxr_create_action (actSet, XR_ACTION_TYPE_VECTOR2F_INPUT,   "thumstick","ThumStick",2, subPath.data());
    m_input.vibrAction = oxr_create_action (actSet, XR_ACTION_TYPE_VIBRATION_OUTPUT, "haptic",   "Haptic",   2, subPath.data());
    m_input.clisAction = oxr_create_action (actSet, XR_ACTION_TYPE_BOOLEAN_INPUT,    "click_s",  "ClickS",   2, subPath.data());
    m_input.cliaAction = oxr_create_action (actSet, XR_ACTION_TYPE_BOOLEAN_INPUT,    "click_a",  "ClickA",   0, NULL);
    m_input.clibAction = oxr_create_action (actSet, XR_ACTION_TYPE_BOOLEAN_INPUT,    "click_b",  "ClickB",   0, NULL);
    m_input.clixAction = oxr_create_action (actSet, XR_ACTION_TYPE_BOOLEAN_INPUT,    "click_x",  "ClickX",   0, NULL);
    m_input.cliyAction = oxr_create_action (actSet, XR_ACTION_TYPE_BOOLEAN_INPUT,    "click_y",  "ClickY",   0, NULL);
    m_input.quitAction = oxr_create_action (actSet, XR_ACTION_TYPE_BOOLEAN_INPUT,    "menu_quit","MenuQuit", 0, NULL);

    // Suggest bindings for the Oculus Touch.
    {
        std::vector<XrActionSuggestedBinding> bindings;
        bindings.push_back ({m_input.poseAction, oxr_str2path (m_instance, HANDL_IN"/grip/pose"    )});
        bindings.push_back ({m_input.poseAction, oxr_str2path (m_instance, HANDR_IN"/grip/pose"    )});
        bindings.push_back ({m_input.aimAction,  oxr_str2path (m_instance, HANDL_IN"/aim/pose"     )});
        bindings.push_back ({m_input.aimAction,  oxr_str2path (m_instance, HANDR_IN"/aim/pose"     )});
        bindings.push_back ({m_input.squzAction, oxr_str2path (m_instance, HANDL_IN"/squeeze/value")});
        bindings.push_back ({m_input.squzAction, oxr_str2path (m_instance, HANDR_IN"/squeeze/value")});
        bindings.push_back ({m_input.trigAction, oxr_str2path (m_instance, HANDL_IN"/trigger/value")});
        bindings.push_back ({m_input.trigAction, oxr_str2path (m_instance, HANDR_IN"/trigger/value")});
        bindings.push_back ({m_input.stikAction, oxr_str2path (m_instance, HANDL_IN"/thumbstick"   )});
        bindings.push_back ({m_input.stikAction, oxr_str2path (m_instance, HANDR_IN"/thumbstick"   )});
        bindings.push_back ({m_input.vibrAction, oxr_str2path (m_instance, HANDL_OT"/haptic"       )});
        bindings.push_back ({m_input.vibrAction, oxr_str2path (m_instance, HANDR_OT"/haptic"       )});
        bindings.push_back ({m_input.clisAction, oxr_str2path (m_instance, HANDR_IN"/thumbstick/click")});
        bindings.push_back ({m_input.clisAction, oxr_str2path (m_instance, HANDL_IN"/thumbstick/click")});
        bindings.push_back ({m_input.cliaAction, oxr_str2path (m_instance, HANDR_IN"/a/click"      )});
        bindings.push_back ({m_input.clibAction, oxr_str2path (m_instance, HANDR_IN"/b/click"      )});
        bindings.push_back ({m_input.clixAction, oxr_str2path (m_instance, HANDL_IN"/x/click"      )});
        bindings.push_back ({m_input.cliyAction, oxr_str2path (m_instance, HANDL_IN"/y/click"      )});
        bindings.push_back ({m_input.quitAction, oxr_str2path (m_instance, HANDL_IN"/menu/click"   )});

        oxr_bind_interaction (m_instance, "/interaction_profiles/oculus/touch_controller", bindings);
    }

    /* attach actions bound to the device. */
    oxr_attach_actionsets (m_session, m_input.actionSet);

    /* create ActionSpace */
    m_input.handSpace[Side::LEFT ] = oxr_create_action_space (m_session, m_input.poseAction, subPath[Side::LEFT ]);
    m_input.handSpace[Side::RIGHT] = oxr_create_action_space (m_session, m_input.poseAction, subPath[Side::RIGHT]);
    m_input.aimSpace [Side::LEFT ] = oxr_create_action_space (m_session, m_input.aimAction,  subPath[Side::LEFT ]);
    m_input.aimSpace [Side::RIGHT] = oxr_create_action_space (m_session, m_input.aimAction,  subPath[Side::RIGHT]);
}


void 
AppEngine::PollActions()
{
    oxr_sync_actions (m_session, m_input.actionSet);

    m_input.handActive = {XR_FALSE, XR_FALSE};

    // Get Pose and Squeeze action state and start Haptic vibrate when hand is 90% squeezed.
    for (auto hand : {Side::LEFT, Side::RIGHT})
    {
        XrPath subPath = m_input.handSubactionPath[hand];

        /* Squeeze */
        {
            XrActionStateFloat stat = oxr_get_action_state_float (m_session, m_input.squzAction, subPath);
            if (stat.isActive == XR_TRUE)
            {
                m_input.squeezeVal[hand] = stat.currentState;
                if (stat.currentState > 0.9f)
                {
                    oxr_apply_haptic_feedback_vibrate (m_session, m_input.vibrAction, subPath,
                        XR_MIN_HAPTIC_DURATION, XR_FREQUENCY_UNSPECIFIED, 0.5f);
                }
            }
        }
        /* Trigger */
        {
            XrActionStateFloat stat = oxr_get_action_state_float (m_session, m_input.trigAction, subPath);
            if (stat.isActive == XR_TRUE)
                m_input.triggerVal[hand] = stat.currentState;
        }
        /* ThumbStick */
        {
            XrActionStateVector2f stat = oxr_get_action_state_vector2 (m_session, m_input.stikAction, subPath);
            if (stat.isActive == XR_TRUE)
                m_input.stickVal[hand] = stat.currentState;
        }
        /* Button-S (ThumbStick Click) */
        {
            XrActionStateBoolean stat = oxr_get_action_state_boolean (m_session, m_input.clisAction, subPath);
            if (stat.isActive == XR_TRUE)
                m_input.clickS[hand] = stat.currentState;
        }
        /* Pose */
        {
            XrActionStatePose stat = oxr_get_action_state_pose (m_session, m_input.poseAction, subPath);
            m_input.handActive[hand] = stat.isActive;
        }
    }

    /* Button-A */
    {
        XrActionStateBoolean stat = oxr_get_action_state_boolean (m_session, m_input.cliaAction, 0);
        if (stat.isActive == XR_TRUE)
            m_input.clickA = stat.currentState;
    }
    /* Button-B */
    {
        XrActionStateBoolean stat = oxr_get_action_state_boolean (m_session, m_input.clibAction, 0);
        if (stat.isActive == XR_TRUE)
            m_input.clickB = stat.currentState;
    }
    /* Button-X */
    {
        XrActionStateBoolean stat = oxr_get_action_state_boolean (m_session, m_input.clixAction, 0);
        if (stat.isActive == XR_TRUE)
            m_input.clickX = stat.currentState;
    }
    /* Button-Y */
    {
        XrActionStateBoolean stat = oxr_get_action_state_boolean (m_session, m_input.cliyAction, 0);
        if (stat.isActive == XR_TRUE)
            m_input.clickY = stat.currentState;
    }
    /* Button-Menu */
    {
        XrActionStateBoolean stat = oxr_get_action_state_boolean (m_session, m_input.quitAction, 0);
        if ((stat.isActive             == XR_TRUE) &&
            (stat.changedSinceLastSync == XR_TRUE) &&
            (stat.currentState         == XR_TRUE))
        {
            xrRequestExitSession (m_session);
            LOGI ("----------- xrRequestExitSession() --------");
        }
    }
}


/* ------------------------------------------------------------------------------------ *
 *  Update  Frame (Event handle, Render)
 * ------------------------------------------------------------------------------------ */
void
AppEngine::UpdateFrame()
{
    bool exit_loop, req_restart;
    oxr_poll_events (m_instance, m_session, &exit_loop, &req_restart);

    if (!oxr_is_session_running())
    {
        return;
    }

    PollActions();

    RenderFrame();
}


/* ------------------------------------------------------------------------------------ *
 *  RenderFrame (Frame/Layer/View)
 * ------------------------------------------------------------------------------------ */
void
AppEngine::RenderFrame()
{
    std::vector<XrCompositionLayerBaseHeader*> all_layers;

    XrTime dpy_time, elapsed_us;
    oxr_begin_frame (m_session, &dpy_time);

    static XrTime init_time = -1;
    if (init_time < 0)
        init_time = dpy_time;
    elapsed_us = (dpy_time - init_time) / 1000;

    /* Passthrough Layer */
    {
        oxr_resume_passthrough_layer (m_instance, m_ptLayer);

        XrCompositionLayerPassthroughFB ptLayer = {XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
        ptLayer.layerHandle = m_ptLayer;
        all_layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&ptLayer));
    }

    /* Projection Layer */
    {
        std::vector<XrCompositionLayerProjectionView> projLayerViews;
        RenderLayer (dpy_time, elapsed_us, projLayerViews);

        XrCompositionLayerProjection projLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        projLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        projLayer.space      = m_appSpace;
        projLayer.viewCount  = (uint32_t)projLayerViews.size();
        projLayer.views      = projLayerViews.data();
        all_layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projLayer));
    }

    /* Compose all layers */
    oxr_end_frame (m_session, dpy_time, all_layers);
}

bool
AppEngine::RenderLayer(XrTime dpy_time,
                       XrTime elapsed_us,
                       std::vector<XrCompositionLayerProjectionView> &layerViews)
{
    /* Acquire View Location */
    uint32_t viewCount = (uint32_t)m_viewSurface.size();

    std::vector<XrView> views(viewCount);
    oxr_locate_views (m_session, dpy_time, m_appSpace, &viewCount, views.data());

    layerViews.resize (viewCount);

    /* Acquire Stage Location, View Location (rerative to the View Location) */
    XrSpaceLocation stageLoc {XR_TYPE_SPACE_LOCATION};
    XrSpaceLocation viewLoc  {XR_TYPE_SPACE_LOCATION};
    xrLocateSpace (m_stageSpace, m_appSpace, dpy_time, &stageLoc);
    xrLocateSpace (m_viewSpace,  m_appSpace, dpy_time, &viewLoc);


    /* Acquire Hand-Space Location (rerative to the View Location) */
    std::array<XrSpaceLocation, 2> handLoc;
    std::array<XrSpaceLocation, 2> aimLoc;
    for (uint32_t i = 0; i < 2; i ++)
    {
        handLoc[i] = {XR_TYPE_SPACE_LOCATION};
        xrLocateSpace (m_input.handSpace[i], m_appSpace, dpy_time, &handLoc[i]);

        aimLoc[i]  = {XR_TYPE_SPACE_LOCATION};
        xrLocateSpace (m_input.aimSpace[i],  m_appSpace, dpy_time, &aimLoc[i]);
    }

    oxr_locate_handjoints (m_instance, m_handTracker[0], m_appSpace, dpy_time, m_handJointLoc[0]);
    oxr_locate_handjoints (m_instance, m_handTracker[1], m_appSpace, dpy_time, m_handJointLoc[1]);

    /* Render each view */
    for (uint32_t i = 0; i < viewCount; i++) {
        XrSwapchainSubImage subImg;
        render_target_t     rtarget;

        oxr_acquire_viewsurface (m_viewSurface[i], rtarget, subImg);

        layerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        layerViews[i].pose     = views[i].pose;
        layerViews[i].fov      = views[i].fov;
        layerViews[i].subImage = subImg;

        scene_data_t sceneData;
        sceneData.runtime_name  = m_runtime_name;
        sceneData.system_name   = m_system_name;
        sceneData.elapsed_us    = elapsed_us;
        sceneData.viewID        = i;
        sceneData.views         = views;
        sceneData.handLoc       = handLoc;
        sceneData.aimLoc        = aimLoc;
        sceneData.inputState    = m_input;
        sceneData.handJointLoc  = m_handJointLoc;
        render_gles_scene (layerViews[i], rtarget, viewLoc.pose, stageLoc.pose, sceneData);

        oxr_release_viewsurface (m_viewSurface[i]);
    }

    return true;
}

