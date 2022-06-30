#pragma once

#include <string>
#include <array>
#include "util_egl.h"
#include "util_oxr.h"
#include "OboePlayer.h"


namespace Side {
const int LEFT  = 0;
const int RIGHT = 1;
const int COUNT = 2;
}

struct InputState {
    XrActionSet actionSet  {XR_NULL_HANDLE};
    XrAction    poseAction {XR_NULL_HANDLE};
    XrAction    aimAction  {XR_NULL_HANDLE};

    XrAction    squzAction {XR_NULL_HANDLE};
    XrAction    trigAction {XR_NULL_HANDLE};
    XrAction    stikAction {XR_NULL_HANDLE};

    XrAction    clisAction {XR_NULL_HANDLE};
    XrAction    cliaAction {XR_NULL_HANDLE};
    XrAction    clibAction {XR_NULL_HANDLE};
    XrAction    clixAction {XR_NULL_HANDLE};
    XrAction    cliyAction {XR_NULL_HANDLE};

    XrAction    vibrAction {XR_NULL_HANDLE};
    XrAction    quitAction {XR_NULL_HANDLE};

    std::array<XrPath,   Side::COUNT> handSubactionPath;

    std::array<XrSpace,  Side::COUNT> handSpace;
    std::array<XrSpace,  Side::COUNT> aimSpace;

    std::array<float,       Side::COUNT> squeezeVal;
    std::array<float,       Side::COUNT> triggerVal;
    std::array<XrVector2f,  Side::COUNT> stickVal;
    std::array<XrBool32,    Side::COUNT> handActive;

    std::array<bool,        Side::COUNT> clickS;
    bool    clickA;
    bool    clickB;
    bool    clickX;
    bool    clickY;
};


class AppEngine {
public:
    explicit AppEngine(android_app* app);
    ~AppEngine();

    // Interfaces to android application framework
    struct android_app* AndroidApp(void) const;

    void InitOpenXR_GLES ();
    void UpdateFrame();


private:
    struct android_app  *m_app;

    XrInstance          m_instance;
    XrSession           m_session;

    XrSpace             m_appSpace;
    XrSpace             m_stageSpace;
    XrSpace             m_viewSpace;

    XrSystemId          m_systemId;
    std::vector<viewsurface_t> m_viewSurface;

    InputState          m_input = {};

    std::string         m_runtime_name;
    std::string         m_system_name;

    OboeSinePlayer      *m_oboePlayer;

    void InitializeActions();
    void PollActions();

    void RenderFrame();
    bool RenderLayer(XrTime dpy_time,
                     XrTime elapsed_us,
                     std::vector<XrCompositionLayerProjectionView> &layerViews,
                     XrCompositionLayerProjection                  &layer);
public:
};

AppEngine *GetAppEngine (void);

