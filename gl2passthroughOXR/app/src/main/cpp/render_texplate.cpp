#include <GLES3/gl31.h>
#include "util_egl.h"
#include "util_shader.h"
#include "util_matrix.h"
#include "render_texplate.h"
#include "assertgl.h"


static shader_obj_t s_sobj;
static int s_loc_mtx;
static int s_loc_color;


static float varray[] =
{   -0.5,  0.5, 0.0,
    -0.5, -0.5, 0.0,
     0.5,  0.5, 0.0,
     0.5, -0.5, 0.0 };

/* ------------------------------------------------------ *
 *  shader for Texture
 * ------------------------------------------------------ */
static char vs_tex[] = "                              \n\
attribute    vec4    a_Vertex;                        \n\
attribute    vec2    a_TexCoord;                      \n\
varying      vec2    v_TexCoord;                      \n\
uniform      mat4    u_PMVMatrix;                     \n\
                                                      \n\
void main (void)                                      \n\
{                                                     \n\
    gl_Position = u_PMVMatrix * a_Vertex;             \n\
    v_TexCoord  = a_TexCoord;                         \n\
}                                                     \n";

static char fs_tex[] = "                              \n\
precision mediump float;                              \n\
varying     vec2      v_TexCoord;                     \n\
uniform     sampler2D u_sampler;                      \n\
uniform     vec4      u_Color;                        \n\
                                                      \n\
void main (void)                                      \n\
{                                                     \n\
    gl_FragColor = texture2D (u_sampler, v_TexCoord); \n\
    gl_FragColor *= u_Color;                          \n\
}                                                     \n";


int
init_texplate ()
{
    generate_shader (&s_sobj, vs_tex, fs_tex);

    s_loc_mtx    = glGetUniformLocation (s_sobj.program, "u_PMVMatrix");
    s_loc_color  = glGetUniformLocation (s_sobj.program, "u_Color");

    return 0;
}


typedef struct _texparam
{
    int          textype;
    int          texid;
    int          upsidedown;
    float        color[4];
    float        *matPVM;
    float        rot;               /* degree */
    float        px, py;            /* pivot */
    int          blendfunc_en;
    unsigned int blendfunc[4];      /* src_rgb, dst_rgb, src_alpha, dst_alpha */
    float        *user_texcoord;
} texparam_t;


static void
flip_texcoord (float *uv, unsigned int flip_mode)
{
    if (flip_mode & RENDER2D_FLIP_V)
    {
        uv[1] = 1.0f - uv[1];
        uv[3] = 1.0f - uv[3];
        uv[5] = 1.0f - uv[5];
        uv[7] = 1.0f - uv[7];
    }

    if (flip_mode & RENDER2D_FLIP_H)
    {
        uv[0] = 1.0f - uv[0];
        uv[2] = 1.0f - uv[2];
        uv[4] = 1.0f - uv[4];
        uv[6] = 1.0f - uv[6];
    }
}


static int
draw_texture_in (texparam_t *tparam)
{
    //int ttype = tparam->textype;
    int texid = tparam->texid;
    shader_obj_t *sobj = &s_sobj;
    float tarray[] = {
        0.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        1.0, 1.0 };
    float *uv = tarray;

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

    glUseProgram (sobj->program);
    glActiveTexture (GL_TEXTURE0);
    glUniform1i(sobj->loc_tex, 0);

    glBindTexture (GL_TEXTURE_2D, texid);

    flip_texcoord (uv, tparam->upsidedown);

    if (tparam->user_texcoord)
    {
        uv = tparam->user_texcoord;
    }

    if (sobj->loc_uv >= 0)
    {
        glEnableVertexAttribArray (sobj->loc_uv);
        glVertexAttribPointer (sobj->loc_uv, 2, GL_FLOAT, GL_FALSE, 0, uv);
    }

    glEnable (GL_BLEND);

    if (tparam->blendfunc_en)
    {
        glBlendFuncSeparate (tparam->blendfunc[0], tparam->blendfunc[1],
                   tparam->blendfunc[2], tparam->blendfunc[3]);
    }
    else
    {
        glBlendFuncSeparate (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                   GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    glUniformMatrix4fv (s_loc_mtx, 1, GL_FALSE, tparam->matPVM);
    glUniform4fv (s_loc_color, 1, tparam->color);

    if (sobj->loc_vtx >= 0)
    {
        glEnableVertexAttribArray (sobj->loc_vtx);
        glVertexAttribPointer (sobj->loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, varray);
    }

    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

    glDisable (GL_BLEND);

    GLASSERT ();
    return 0;
}


int
draw_tex_plate (int texid, float *matPVM, int upsidedown)
{
    texparam_t tparam = {0};
    tparam.texid   = texid;
    tparam.textype = 0;
    tparam.color[0]= 1.0f;
    tparam.color[1]= 1.0f;
    tparam.color[2]= 1.0f;
    tparam.color[3]= 1.0f;
    tparam.upsidedown = upsidedown;
    tparam.matPVM  = matPVM;
    draw_texture_in (&tparam);

    return 0;
}

