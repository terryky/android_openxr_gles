/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2022 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef UTIL_OXR_H_
#define UTIL_OXR_H_


#ifdef __ANDROID__
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <jni.h>
#include <sys/system_properties.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#include <vector>

#include "util_log.h"


typedef struct swapchain_obj_t
{
    XrSwapchain handle;
    uint32_t    width;
    uint32_t    height;
    XrSwapchainImageOpenGLESKHR *img_array;
} swapchain_obj_t;



/* Initialize OpenXR Loader */
int         oxr_initialize_loader (void *appVM, void *appCtx);

/* Create OpenXR Instance with Android/OpenGLES binding */
XrInstance  oxr_create_instance   (void *appVM, void *appCtx);

/*  Get OpenXR Sysem */
XrSystemId  oxr_get_system (XrInstance instance);

/* Confirm OpenGLES version */
int         oxr_confirm_gfx_requirements (XrInstance instance, XrSystemId systemId);


/* View operation */
XrViewConfigurationView *
            oxr_enumerate_viewconfig (XrInstance instance, XrSystemId sysid, uint32_t *numview);
int         oxr_locate_views (XrSession session, XrTime dpy_time, XrSpace space, uint32_t *view_cnt, XrView *view_array);


/* Swapchain operation */
int         oxr_create_swapchain        (swapchain_obj_t *scobj, XrSession session, uint32_t width, uint32_t height);
int         oxr_acquire_swapchain_image (swapchain_obj_t *scobj, XrSwapchainImageOpenGLESKHR *glesImg, XrSwapchainSubImage *subImg);
void        oxr_release_swapchain_image (swapchain_obj_t *scobj);


/* Frame operation */
int         oxr_begin_frame (XrSession session, XrTime *dpyTime);
int         oxr_end_frame   (XrSession session, XrTime dpyTime, std::vector<XrCompositionLayerBaseHeader*> &layers);


/* Space operation */
XrSpace     oxr_create_ref_space    (XrSession session, XrReferenceSpaceType ref_space_type);
XrSpace     oxr_create_action_space (XrSession session, XrAction action, XrPath subpath);

XrActionSet oxr_create_actionset (XrInstance instance, const char *name, const char *local_name, int priority);
XrAction    oxr_create_action (XrActionSet actionset, XrActionType type, const char *name, const char *local_name,
                                int subpath_num, XrPath *subpath_array);


/* Session operation */
XrSession   oxr_create_session (XrInstance instance, XrSystemId sysid);
int         oxr_begin_session (XrSession session);
int         oxr_handle_session_state_changed (XrSession session, XrEventDataSessionStateChanged &ev,
                                              bool *exitLoop, bool *reqRestart);
bool        oxr_is_session_running ();

int         oxr_poll_events (XrInstance instance, XrSession session, bool *exit_loop, bool *req_restart);

#endif
