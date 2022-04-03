#include <GLES3/gl31.h>
#include <common/xr_linear.h>
#include "util_egl.h"
#include "util_oxr.h"
#include "util_shader.h"
#include "util_matrix.h"
#include "util_debugstr.h"

static shader_obj_t     s_sobj;

static GLfloat s_vtx[] =
{
    -0.5f, 0.5f,
    -0.5f,-0.5f,
     0.5f, 0.5f,
};

static GLfloat s_col[] =
{
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
};


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
    init_dbgstr (0, 0);

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

    return 0;
}

int
draw_triangle (float *matStage)
{
    shader_obj_t *sobj = &s_sobj;
    float matM[16], matPVM[16];

    matrix_identity (matM);
    matrix_translate (matM, 0.0f, 1.0f, -1.0f);
    matrix_mult (matPVM, matStage, matM);

    glUseProgram (sobj->program);

    glEnableVertexAttribArray (sobj->loc_vtx);
    glEnableVertexAttribArray (sobj->loc_clr);
    glVertexAttribPointer (sobj->loc_vtx, 2, GL_FLOAT, GL_FALSE, 0, s_vtx);
    glVertexAttribPointer (sobj->loc_clr, 4, GL_FLOAT, GL_FALSE, 0, s_col);

    glUniformMatrix4fv (sobj->loc_mtx,  1, GL_FALSE, matPVM);

    glDrawArrays (GL_TRIANGLE_STRIP, 0, 3);

    return 0;
}


int
render_gles_scene (XrCompositionLayerProjectionView &layerView,
                   render_target_t &rtarget, XrPosef &stagePose,
                   uint32_t viewID)
{
    int view_x = layerView.subImage.imageRect.offset.x;
    int view_y = layerView.subImage.imageRect.offset.y;
    int view_w = layerView.subImage.imageRect.extent.width;
    int view_h = layerView.subImage.imageRect.extent.height;

    glBindFramebuffer(GL_FRAMEBUFFER, rtarget.fbo_id);

    glViewport(view_x, view_y, view_w, view_h);

    glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


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
    draw_triangle (matStage);

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


