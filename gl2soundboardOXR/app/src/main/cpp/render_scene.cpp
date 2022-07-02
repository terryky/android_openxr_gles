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
#include "render_stage.h"
#include "render_texplate.h"
#include "render_imgui.h"
#include "assertgl.h"

typedef struct fvec2d_t
{
    float   x, y;
} fvec2d_t;

typedef struct uiplane_t
{
    float       matM[16];
    float       width;
    float       height;
    fvec2d_t    hit[2];
    render_target_t rtarget;
} uiplane_t;

static uiplane_t        s_uiplane[5];
static shader_obj_t     s_sobj;
static snece_state_t    s_sstate;

#define UI_WIN_W 300
#define UI_WIN_H 740


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
    init_stage ();
    init_texplate ();
    init_2d_renderer (0, 0);
    init_dbgstr (0, 0);
    init_imgui (UI_WIN_W, UI_WIN_H);

    {
        uiplane_t *uiplane;

        uiplane = &s_uiplane[0];
        uiplane->width  = UI_WIN_W;
        uiplane->height = UI_WIN_H;
        create_render_target (&uiplane->rtarget, uiplane->width, uiplane->height, RTARGET_COLOR);

        for (int i = 1; i < 5; i ++)
        {
            uiplane = &s_uiplane[i];
            uiplane->width  = 1600;
            uiplane->height = 1000;
            create_render_target (&uiplane->rtarget, uiplane->width, uiplane->height, RTARGET_COLOR);
        }
    }
    
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

    glLineWidth (1.0f);
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


static void
update_uiplane_matrix (float *matM, uiplane_t *uiplane, scene_data_t &sceneData)
{
    float matT[16];
    float win_w = uiplane->width  / 300.0f;
    float win_h = uiplane->height / 300.0f;
    float win_x = 1.0f + sceneData.inputState.stickVal[1].x * 0.5f;
    float win_y = 0.0f + sceneData.inputState.stickVal[1].y * 0.5f;
    float win_z =-2.0f;

    matrix_identity (matT);
    matrix_translate (matT, win_x, win_y, win_z);
    matrix_rotate (matT, -30.0f, 0.0f, 1.0f, 0.0f);
    matrix_scale (matT, win_w, win_h, 1.0f);

    matrix_mult (uiplane->matM, matM, matT);
}

static void
update_plane_matrix (float *matM, uiplane_t *uiplane, int plane_id)
{
    float matT[16];
    float win_w = uiplane->width  / 200.0f;
    float win_h = uiplane->height / 200.0f;
    float win_x =  0.0f;
    float win_y = win_h * 0.5f;
    float win_z = -5.0f;

    matrix_identity (matT);
    matrix_rotate (matT, - (plane_id - 2) * 90.0f, 0.0f, 1.0f, 0.0f);
    matrix_translate (matT, win_x, win_y, win_z);
    matrix_scale (matT, win_w, win_h, 1.0f);

    matrix_mult (uiplane->matM, matM, matT);
}


int
draw_uiplane (float *matP, float *matV, float *matM,
              XrCompositionLayerProjectionView &layerView,
              scene_data_t &sceneData)
{
    uiplane_t *uiplane = &s_uiplane[0];
    float matPVM[16], matVM[16];

    update_uiplane_matrix (matM, uiplane, sceneData);
    matrix_mult (matVM, matV, uiplane->matM);
    matrix_mult (matPVM, matP, matVM);

    /* save current FBO */
    render_target_t rtarget0;
    get_render_target (&rtarget0);

    /* render to UIPlane-FBO */
    set_render_target (&uiplane->rtarget);
    {
        glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
        glClear (GL_COLOR_BUFFER_BIT);

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
#if 0
        float hitx = uiplane->hit[1].x;
        float hity = uiplane->hit[1].y;

        imgui_mousemove (hitx, hity);
        imgui_mousebutton (0, sceneData.inputState.clickA, hitx, hity);
#endif
        invoke_imgui (&sceneData);
    }

    /* restore FBO */
    set_render_target (&rtarget0);

    glEnable (GL_DEPTH_TEST);
    draw_tex_plate (uiplane->rtarget.texc_id, matPVM, RENDER2D_FLIP_V);

    return 0;
}

/*
 *      |   |   | |   |   |   |   | |   | |   |   |
 *      |   |   | |   |   |   |   | |   | |   |   |
 *      |   | 1 | | 3 |   |   | 6 | | 8 | |10 |   |
 *      |   |   | |   |   |   |   | |   | |   |   |
 *      |   +-+-+ +-+-+   |   +-+-+ +-+-+ +-+-+   |
 *      |     |     |     |     |     |     |     |
 *      |  C  |  D  |  E  |  F  |  G  |  A  |  B  |
 *      |  0  |  2  |  4  |  5  |  7  |  9  |  11 |
 *      +-----+-----+-----+-----+-----+-----+-----+
 */
static int
note_en (int note)
{
    if ((s_sstate.hitnote[0] == note) ||
        (s_sstate.hitnote[1] == note))
        return 1;
    return 0;
}

static void
draw_keyboard (int win_w, int win_h, int plane_id)
{
    float col_white[]     = {1.00f, 1.00f, 1.00f, 1.0f};
    float col_black[]     = {0.00f, 0.00f, 0.00f, 1.0f};
    float col_white_hit[] = {1.00f, 0.50f, 0.50f, 1.0f};
    float col_black_hit[] = {0.10f, 0.00f, 0.00f, 1.0f};

    float *col;
    int wkey_w = win_w / 7;
    int bkey_w = wkey_w * 0.5;
    int octave = (plane_id - 1) * 12;
    int white_note[] = { 0, 2, 4,  5, 7, 9, 11};
    int black_note[] = {-1, 1, 3, -1, 6, 8, 10};

    set_2d_projection_matrix (win_w, win_h);

    /* white key */
    col = col_white;
    draw_2d_fillrect (0, 0, win_w, win_h, col);

    for (int i = 0; i < 7; i ++)
    {
        if (note_en (white_note[i] + octave))
            draw_2d_fillrect (i * wkey_w, 0, wkey_w, win_h, col_white_hit);
    }

    col = col_black;
    draw_2d_rect (0, 0, win_w, win_h, col, 10.0f);
    for (int i = 0; i < 7; i ++)
        draw_2d_line (i * wkey_w, 0, i * wkey_w, win_h, col, 5.0f);

    /* black key */
    for (int i = 0; i < 7; i ++)
    {
        if (black_note[i] < 0)
            continue;

        col = col_black;
        if (note_en (black_note[i] + octave))
            col = col_black_hit;

        int x = i * wkey_w - (bkey_w / 2);
        draw_2d_fillrect (x, 0, bkey_w, win_h * 0.6, col);
    }
}


static int
draw_plane (float *matP, float *matV, float *matM, int plane_id, scene_data_t &sceneData)
{
    uiplane_t *uiplane = &s_uiplane[plane_id];
    float win_w = uiplane->width;
    float win_h = uiplane->height;
    float matPVM[16], matVM[16];

    update_plane_matrix (matM, uiplane, plane_id);
    matrix_mult (matVM, matV, uiplane->matM);
    matrix_mult (matPVM, matP, matVM);

    /* save current FBO */
    render_target_t rtarget0;
    get_render_target (&rtarget0);

    /* render to UIPlane-FBO */
    set_render_target (&uiplane->rtarget);
    {
        glClearColor (0.1f, 0.1f, 0.2f, 1.0f);
        glClear (GL_COLOR_BUFFER_BIT);

        set_2d_projection_matrix (win_w, win_h);
        update_dbgstr_winsize (win_w, win_h);

        draw_keyboard (win_w, win_h, plane_id);

        for (int ihand = 0; ihand < 2; ihand ++)
        {
            float color[2][4] = {{1.0f, 0.0f, 1.0f, 0.3f},
                                 {0.0f, 1.0f, 0.0f, 0.3f}};
            int   rad  = 30;

            float hitx  = uiplane->hit[ihand].x;
            float hity  = uiplane->hit[ihand].y;
            if (hitx > 0 && hity > 0)
            {
                float *col = color[ihand];
                //draw_2d_line (0, hity, win_w, hity, col, 1.0f);
                //draw_2d_line (hitx, 0, hitx, win_h, col, 1.0f);
                draw_2d_fillcircle (hitx, hity, rad, col);

                char strbuf[128];
                sprintf (strbuf, "(%5.1f, %5.1f)", hitx, hity);

                draw_dbgstr (strbuf, hitx, hity);
            }
        }
    }

    /* restore FBO */
    set_render_target (&rtarget0);

    glEnable (GL_DEPTH_TEST);
    draw_tex_plate (uiplane->rtarget.texc_id, matPVM, RENDER2D_FLIP_V);

    return 0;
}


static int
draw_beam (float *matP, float *matV, float *matM)
{
    float matPVM[16], matVM[16];
    matrix_mult (matVM, matV, matM);
    matrix_mult (matPVM, matP, matVM);

    float col[4] = {0.0f, 1.0f, 1.0f, 1.0f};
    float p0[3]  = {0.0f, 0.0f,     0.0f};
    float p1[3]  = {0.0f, 0.0f, -1000.0f};

    draw_line (matPVM, p0, p1, col);
    GLASSERT();

    return 0;
}


static void
update_hittest (float *matMaim, int ihand, scene_data_t &sceneData)
{
    float p0[3] = {0.0f, 0.0f,     0.0f};
    float p1[3] = {0.0f, 0.0f, -1000.0f};
    float ray0[3];
    float ray1[3];

    s_sstate.hitnote[ihand] = -1;

    /* transform ray vector into Global space */
    matrix_multvec3 (matMaim, p0, ray0);
    matrix_multvec3 (matMaim, p1, ray1);

    /*
     * - hittest in Global space
     * - get the intersection ponit in the Plane space.
     */
    int numplane = sizeof(s_uiplane) / sizeof (s_uiplane[0]);
    for (int i = 1; i < numplane; i ++)     /* ignore imgui plane */
    {
        uiplane_t *uiplane = &s_uiplane[i];
        float *matM = uiplane->matM;
        float cp[3];
        int ret = hittest_tex_plate (matM, ray0, ray1, cp);
        if (ret)
        {
            float hitx = cp[0] * uiplane->width;
            float hity = cp[1] * uiplane->height;
            uiplane->hit[ihand].x = hitx;
            uiplane->hit[ihand].y = hity;

            /* keyboard hittest */
            if ((sceneData.inputState.triggerVal[ihand] > 0)   ||
                ((ihand == 0) && sceneData.inputState.clickX)  ||
                ((ihand == 1) && sceneData.inputState.clickA))
            {
                int   note   = -1;
                float wkey_w = 1.0f / 7.0f;
                float bkey_w = wkey_w * 0.5f * 0.5;

                /* white key */
                if      (cp[0] < 1 * wkey_w) note =  0;  /* C */
                else if (cp[0] < 2 * wkey_w) note =  2;  /* D */
                else if (cp[0] < 3 * wkey_w) note =  4;  /* E */
                else if (cp[0] < 4 * wkey_w) note =  5;  /* F */
                else if (cp[0] < 5 * wkey_w) note =  7;  /* G */
                else if (cp[0] < 6 * wkey_w) note =  9;  /* A */
                else                         note = 11;  /* B */

                /* black key */
                if (cp[1] < 0.6f)
                {
                    if ((1 * wkey_w - bkey_w < cp[0]) && (cp[0] < 1 * wkey_w + bkey_w)) note =  1;
                    if ((2 * wkey_w - bkey_w < cp[0]) && (cp[0] < 2 * wkey_w + bkey_w)) note =  3;
                    if ((4 * wkey_w - bkey_w < cp[0]) && (cp[0] < 4 * wkey_w + bkey_w)) note =  6;
                    if ((5 * wkey_w - bkey_w < cp[0]) && (cp[0] < 5 * wkey_w + bkey_w)) note =  8;
                    if ((6 * wkey_w - bkey_w < cp[0]) && (cp[0] < 6 * wkey_w + bkey_w)) note = 10;
                }

                note += (i - 1) * 12;
                s_sstate.hitnote[ihand] = note;
            }
        }
        else
        {
            uiplane->hit[ihand].x = -1;
            uiplane->hit[ihand].y = -1;
        }
    }
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
#if 0
    /* Axis of global origin */
    {
        XrVector3f    scale = {0.2f, 0.2f, 0.2f};
        XrVector3f    pos   = {0.0f, 0.0f, 0.0f};
        XrQuaternionf qtn   = {0.0f, 0.0f, 0.0f, 1.0f};
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);
        draw_axis ((float *)&matP, (float *)&matV, (float *)&matM);
    }

    /* Axis of stage origin */
    {
        XrVector3f    scale = {0.2f, 0.2f, 0.2f};
        XrVector3f    &pos  = stagePose.position;
        XrQuaternionf &qtn  = stagePose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);
        draw_axis ((float *)&matP, (float *)&matV, (float *)&matM);
    }
#endif
    /* teapot */
    float col[] = {1.0f, 0.0f, 0.0f};
    draw_teapot (sceneData.elapsed_us / 1000, col, (float *)&matP, (float *)&matV);


    /* Axis of hand grip */
    for (XrSpaceLocation loc : sceneData.handLoc)
    {
        XrVector3f    scale = {0.2f, 0.2f, 0.2f};
        XrVector3f    &pos  = loc.pose.position;
        XrQuaternionf &qtn  = loc.pose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);
        draw_axis ((float *)&matP, (float *)&matV, (float *)&matM);
        GLASSERT();
    }

    /* Axis of hand aim */
    for (XrSpaceLocation loc : sceneData.aimLoc)
    {
        XrVector3f    scale = {0.05f, 0.05f, 0.05f};
        XrVector3f    &pos  = loc.pose.position;
        XrQuaternionf &qtn  = loc.pose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);
        draw_axis ((float *)&matP, (float *)&matV, (float *)&matM);
        GLASSERT();
    }

    /* Beam of hand aim */
    for (int ihand = 0; ihand < sceneData.aimLoc.size(); ihand ++)
    {
        XrSpaceLocation &loc = sceneData.aimLoc[ihand];
        XrVector3f    scale = {1.0f, 1.0f, 1.0f};
        XrVector3f    &pos  = loc.pose.position;
        XrQuaternionf &qtn  = loc.pose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);

        if (sceneData.viewID == 0)
            update_hittest ((float *)&matM, ihand, sceneData);

        draw_beam ((float *)&matP, (float *)&matV, (float *)&matM);
    }

    /* plane for hittest*/
    {
        XrVector3f    scale = {1.0f, 1.0f, 1.0f};
        XrVector3f    &pos  = stagePose.position;
        XrQuaternionf &qtn  = stagePose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);

        int numplane = sizeof(s_uiplane) / sizeof (s_uiplane[0]);
        for (int i = 1; i < numplane; i ++)
            draw_plane ((float *)&matP, (float *)&matV, (float *)&matM, i, sceneData);
    }

    /* UI plane always view front */
    {
        XrVector3f    scale = {1.0f, 1.0f, 1.0f};
        XrVector3f    &pos  = viewPose.position;
        XrQuaternionf &qtn  = viewPose.orientation;
        XrMatrix4x4f_CreateTranslationRotationScale (&matM, &pos, &qtn, &scale);

        draw_uiplane ((float *)&matP, (float *)&matV, (float *)&matM, layerView, sceneData);
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


int
get_scene_state (scene_state_t &state)
{
    state = s_sstate;
    return 0;
}
