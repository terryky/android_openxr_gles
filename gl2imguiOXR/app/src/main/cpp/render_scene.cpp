#include <GLES3/gl31.h>
#include <common/xr_linear.h>
#include "util_egl.h"
#include "util_oxr.h"
#include "util_shader.h"
#include "util_matrix.h"
#include "util_debugstr.h"
#include "util_render_target.h"
#include "teapot.h"
#include "render_scene.h"
#include "render_texplate.h"
#include "render_imgui.h"
#include "assertgl.h"


static shader_obj_t     s_sobj;
static render_target_t  s_rtarget;

#define UI_WIN_W 300
#define UI_WIN_H 350


static char s_strVS[] = "                                   \n\
                                                            \n\
attribute vec4  a_Vertex;                                   \n\
attribute vec4  a_Color;                                    \n\
varying   vec4  v_color;                                    \n\
uniform   mat4  u_PMVMatrix;                                \n\
                                                            \n\
void main(void)                                             \n\
{                                                           \n\
    gl_Position = u_PMVMatrix * a_Vertex;                   \n\
    v_color     = a_Color;                                  \n\
}                                                           ";

static char s_strFS[] = "                                   \n\
precision mediump float;                                    \n\
varying   vec4  v_color;                                    \n\
                                                            \n\
void main(void)                                             \n\
{                                                           \n\
    gl_FragColor = v_color;                                 \n\
}                                                           ";



int
init_gles_scene ()
{
    generate_shader (&s_sobj, s_strVS, s_strFS);
    init_teapot ();
    init_texplate ();
    init_dbgstr (0, 0);
    init_imgui (UI_WIN_W, UI_WIN_H);

    create_render_target (&s_rtarget, UI_WIN_W, UI_WIN_H, RTARGET_COLOR);

    return 0;
}


int
draw_line (float *mtxPV, float *p0, float *p1, float *color)
{
    GLfloat floor_vtx[6];
    for (int i = 0; i < 3; i ++)
    {
        floor_vtx[0 + i] = p0[i];
        floor_vtx[3 + i] = p1[i];
    }

    shader_obj_t *sobj = &s_sobj;
    glUseProgram (sobj->program);

    glEnableVertexAttribArray (sobj->loc_vtx);
    glVertexAttribPointer (sobj->loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, floor_vtx);

    glDisableVertexAttribArray (sobj->loc_clr);
    glVertexAttrib4fv (sobj->loc_clr, color);

    glUniformMatrix4fv (sobj->loc_mtx,  1, GL_FALSE, mtxPV);

    glEnable (GL_DEPTH_TEST);
    glDrawArrays (GL_LINES, 0, 2);

    return 0;
}

int
draw_stage (float *matStage)
{
    float col_r[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float col_g[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float col_b[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    float col_gray[] = {0.5f, 0.5f, 0.5f, 1.0f};
    float p0[3]  = {0.0f, 0.0f, 0.0f};
    float py[3]  = {0.0f, 1.0f, 0.0f};

    for (int x = -10; x <= 10; x ++)
    {
        float *col = (x == 0) ? col_b : col_gray;
        float p0[3]  = {1.0f * x, 0.0f, -10.0f};
        float p1[3]  = {1.0f * x, 0.0f,  10.0f};
        draw_line (matStage, p0, p1, col);
    }
    for (int z = -10; z <= 10; z ++)
    {
        float *col = (z == 0) ? col_r : col_gray;
        float p0[3]  = {-10.0f, 0.0f, 1.0f * z};
        float p1[3]  = { 10.0f, 0.0f, 1.0f * z};
        draw_line (matStage, p0, p1, col);
    }

    draw_line (matStage, p0, py, col_g);
    GLASSERT();

    return 0;
}


int
draw_uiplane (float *matPVMbase,
              XrCompositionLayerProjectionView &layerView,
              scene_data_t &sceneData)
{
    /* save current FBO */
    render_target_t rtarget0;
    get_render_target (&rtarget0);

    /* render to UIPlane-FBO */
    set_render_target (&s_rtarget);
    glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
    glClear (GL_COLOR_BUFFER_BIT);

    {
        static uint32_t prev_us = 0;
        if (sceneData.viewID == 0)
        {
            sceneData.interval_ms = (sceneData.elapsed_us - prev_us) / 1000.0f;
            prev_us = sceneData.elapsed_us;
        }

        sceneData.gl_version = glGetString (GL_VERSION);
        sceneData.gl_vendor  = glGetString (GL_VENDOR);
        sceneData.gl_render  = glGetString (GL_RENDERER);
        sceneData.viewport   = layerView.subImage.imageRect;
        invoke_imgui (&sceneData);
    }

    /* restore FBO */
    set_render_target (&rtarget0);

    glEnable (GL_DEPTH_TEST);

    {
        float matPVM[16], matT[16];
        float win_x = 1.0f;
        float win_y = 0.0f;
        float win_z =-2.0f;
        float win_w = 1.0f;
        float win_h = win_w * ((float)UI_WIN_H / (float)UI_WIN_W);
        matrix_identity (matT);
        matrix_translate (matT, win_x, win_y, win_z);
        matrix_rotate (matT, -30.0f, 0.0f, 1.0f, 0.0f);
        matrix_scale (matT, win_w, win_h, 1.0f);
        matrix_mult (matPVM, matPVMbase, matT);
        draw_tex_plate (s_rtarget.texc_id, matPVM, RENDER2D_FLIP_V);
    }

    return 0;
}


int
render_gles_scene (XrCompositionLayerProjectionView &layerView,
                   render_target_t                  &rtarget,
                   XrPosef                          &viewPose,
                   XrPosef                          &stagePose,
                   scene_data_t                     &sceneData)
{
    int view_x = layerView.subImage.imageRect.offset.x;
    int view_y = layerView.subImage.imageRect.offset.y;
    int view_w = layerView.subImage.imageRect.extent.width;
    int view_h = layerView.subImage.imageRect.extent.height;

    glBindFramebuffer(GL_FRAMEBUFFER, rtarget.fbo_id);

    glViewport(view_x, view_y, view_w, view_h);

    glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable (GL_DEPTH_TEST);

    /* ------------------------------------------- *
     *  Matrix Setup
     *    (matPV)  = (proj) x (view)
     *    (matPVM) = (proj) x (view) x (model)
     * ------------------------------------------- */
    XrMatrix4x4f matP, matV, matC, matM, matPV, matPVM;

    /* Projection Matrix */
    XrMatrix4x4f_CreateProjectionFov (&matP, GRAPHICS_OPENGL_ES, layerView.fov, 0.05f, 100.0f);

    /* View Matrix (inverse of Camera matrix) */
    XrVector3f scale = {1.0f, 1.0f, 1.0f};
    const auto &vewPose = layerView.pose;
    XrMatrix4x4f_CreateTranslationRotationScale (&matC, &vewPose.position, &vewPose.orientation, &scale);
    XrMatrix4x4f_InvertRigidBody (&matV, &matC);

    /* Stage Space Matrix */
    XrMatrix4x4f_CreateTranslationRotationScale (&matM, &stagePose.position, &stagePose.orientation, &scale);

    XrMatrix4x4f_Multiply (&matPV, &matP, &matV);
    XrMatrix4x4f_Multiply (&matPVM, &matPV, &matM);


    /* ------------------------------------------- *
     *  Render
     * ------------------------------------------- */
    float *matStage = reinterpret_cast<float*>(&matPVM);

    draw_stage (matStage);


    /* teapot */
    float col[] = {1.0f, 0.0f, 0.0f};
    draw_teapot (sceneData.elapsed_us / 1000, col, (float *)&matP, (float *)&matV);


    /* UI plane always view front */
    {
        XrVector3f    scale = {1.0f, 1.0f, 1.0f};
        XrVector3f    &pos  = viewPose.position;
        XrQuaternionf &qtn  = viewPose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);

        XrMatrix4x4f matPVM;
        XrMatrix4x4f_Multiply (&matPVM, &matPV, &matM);

        draw_uiplane ((float *)&matPVM, layerView, sceneData);
    }

    {
        XrVector3f    &pos = layerView.pose.position;
        XrQuaternionf &rot = layerView.pose.orientation;
        XrFovf        &fov = layerView.fov;
        int x = 100;
        int y = 100;
        char strbuf[128];

        update_dbgstr_winsize (view_w, view_h);

        sprintf (strbuf, "VIEWPOS(%6.4f, %6.4f, %6.4f)", pos.x, pos.y, pos.z);
        draw_dbgstr(strbuf, x, y); y += 22;

        sprintf (strbuf, "VIEWROT(%6.4f, %6.4f, %6.4f, %6.4f)", rot.x, rot.y, rot.z, rot.w);
        draw_dbgstr(strbuf, x, y); y += 22;

        sprintf (strbuf, "VIEWFOV(%6.4f, %6.4f, %6.4f, %6.4f)",
            fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown);
        draw_dbgstr(strbuf, x, y); y += 22;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 0;
}


