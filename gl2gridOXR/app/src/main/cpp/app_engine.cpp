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

    m_session  = oxr_create_session (m_instance, m_systemId);
    m_appSpace = oxr_create_ref_space (m_session, XR_REFERENCE_SPACE_TYPE_LOCAL);

    CreateSwapchains ();
}


void
AppEngine::CreateSwapchains()
{
    uint32_t viewCount;
    XrViewConfigurationView *conf_views = oxr_enumerate_viewconfig (m_instance, m_systemId, &viewCount);

    // Create and cache view buffer for xrLocateViews later.
    m_views.resize (viewCount, {XR_TYPE_VIEW});

    // Create a swapchain for each view.
    for (uint32_t i = 0; i < viewCount; i++) {
        const XrViewConfigurationView &vp = conf_views[i];
        uint32_t vp_w = vp.recommendedImageRectWidth;
        uint32_t vp_h = vp.recommendedImageRectHeight;

        LOGI("Swapchain for view %d: WH(%d, %d), SampleCount=%d", i, vp_w, vp_h, vp.recommendedSwapchainSampleCount);
        oxr_create_swapchain (&m_scobj[i], m_session, vp_w, vp_h);
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

    RenderFrame();
}


/* ------------------------------------------------------------------------------------ *
 *  RenderFrame (Frame/Layer/View)
 * ------------------------------------------------------------------------------------ */
void
AppEngine::RenderFrame()
{
    std::vector<XrCompositionLayerBaseHeader*> all_layers;

    XrTime dpy_time;
    oxr_begin_frame (m_session, &dpy_time);

    std::vector<XrCompositionLayerProjectionView> projLayerViews;
    XrCompositionLayerProjection                  projLayer;
    RenderLayer (dpy_time, projLayerViews, projLayer);

    all_layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projLayer));

    /* Compose all layers */
    oxr_end_frame (m_session, dpy_time, all_layers);
}

bool
AppEngine::RenderLayer(XrTime dpy_time,
                       std::vector<XrCompositionLayerProjectionView> &layerViews,
                       XrCompositionLayerProjection                  &layer)
{
    /* Acquire View Location */
    uint32_t viewCount = (uint32_t)m_views.size();
    oxr_locate_views (m_session, dpy_time, m_appSpace, &viewCount, m_views.data());

    layerViews.resize (viewCount);

    /* Render each view */
    for (uint32_t i = 0; i < viewCount; i++) {
        XrSwapchainImageOpenGLESKHR glesImg;
        XrSwapchainSubImage         subImg;

        oxr_acquire_swapchain_image (&m_scobj[i], &glesImg, &subImg);

        render_gles_scene (&glesImg, subImg.imageRect);

        oxr_release_swapchain_image (&m_scobj[i]);

        layerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        layerViews[i].pose     = m_views[i].pose;
        layerViews[i].fov      = m_views[i].fov;
        layerViews[i].subImage = subImg;
    }
    layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    layer.space     = m_appSpace;
    layer.viewCount = (uint32_t)layerViews.size();
    layer.views     = layerViews.data();

    return true;
}

