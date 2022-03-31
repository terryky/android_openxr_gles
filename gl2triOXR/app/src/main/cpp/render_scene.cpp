#include <GLES3/gl31.h>
#include "util_egl.h"
#include "util_oxr.h"
#include "util_shader.h"


static GLuint           s_fbo_swapchain;
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


static char s_strVS[] = "                         \
                                                  \
attribute    vec4    a_Vertex;                    \
attribute    vec4    a_Color;                     \
varying      vec4    v_color;                     \
                                                  \
void main (void)                                  \
{                                                 \
    gl_Position = a_Vertex;                       \
    v_color     = a_Color;                        \
}                                                ";

static char s_strFS[] = "                         \
                                                  \
precision mediump float;                          \
varying     vec4     v_color;                     \
                                                  \
void main (void)                                  \
{                                                 \
    gl_FragColor = v_color;                       \
}                                                 ";

struct engine {
    struct android_app* app;
    shader_obj_t sobj;
};


int
init_gles_scene ()
{
    generate_shader (&s_sobj, s_strVS, s_strFS);

    glGenFramebuffers (1, &s_fbo_swapchain);

    return 0;
}

int
render_gles_scene (XrSwapchainImageOpenGLESKHR *swapchainImg, XrRect2Di imgRect)
{
    int view_x = imgRect.offset.x;
    int view_y = imgRect.offset.y;
    int view_w = imgRect.extent.width;
    int view_h = imgRect.extent.height;
    uint32_t texc = swapchainImg->image;

    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo_swapchain);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texc, 0);

    glViewport(view_x, view_y, view_w, view_h);

    glClearColor (0.1f, 0.1f, 0.1f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);

    /* ------------------------------------------- *
     *  Render
     * ------------------------------------------- */
    shader_obj_t *sobj = &s_sobj;
    glUseProgram (sobj->program);

    glEnableVertexAttribArray (sobj->loc_vtx);
    glEnableVertexAttribArray (sobj->loc_clr);
    glVertexAttribPointer (sobj->loc_vtx, 2, GL_FLOAT, GL_FALSE, 0, s_vtx);
    glVertexAttribPointer (sobj->loc_clr, 4, GL_FLOAT, GL_FALSE, 0, s_col);

    glDrawArrays (GL_TRIANGLE_STRIP, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 0;
}


