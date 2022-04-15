#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES2/gl2.h>
#include "util_egl.h"
#include "assertgl.h"
#include "util_shader.h"
#include "util_matrix.h"
#include "shapes.h"
#include "assertgl.h"


static shader_obj_t s_sobj;
static GLint        s_loc_mtx_mv;
static GLint        s_loc_mtx_pmv;
static GLint        s_loc_mtx_nrm;
static GLint        s_loc_color;
static GLint        s_loc_alpha;
static GLint        s_loc_lightpos;

static shape_obj_t  s_cylinder;
static shape_obj_t  s_cone;
static shape_obj_t  s_sphere;


static char s_strVS[] = "                                   \n\
                                                            \n\
attribute vec4  a_Vertex;                                   \n\
attribute vec3  a_Normal;                                   \n\
attribute vec2  a_TexCoord;                                 \n\
uniform   mat4  u_PMVMatrix;                                \n\
uniform   mat4  u_MVMatrix;                                 \n\
uniform   mat3  u_ModelViewIT;                              \n\
varying   vec3  v_diffuse;                                  \n\
uniform   vec3  u_LightPos;                                 \n\
const     vec3  LightCol = vec3(1.0, 1.0, 1.0);             \n\
                                                            \n\
void DirectionalLight (vec3 normal, vec3 eyePos)            \n\
{                                                           \n\
    vec3  lightDir = normalize (u_LightPos);                \n\
    vec3  halfV    = normalize (u_LightPos - eyePos);       \n\
    float dVP      = max(dot(normal, lightDir), 0.0);       \n\
                                                            \n\
    v_diffuse += dVP * LightCol;                            \n\
}                                                           \n\
                                                            \n\
void main(void)                                             \n\
{                                                           \n\
    gl_Position = u_PMVMatrix * a_Vertex;                   \n\
    vec3 normal = normalize(u_ModelViewIT * a_Normal);      \n\
    vec3 eyePos = vec3(u_MVMatrix * a_Vertex);              \n\
                                                            \n\
    v_diffuse  = vec3(0.5);                                 \n\
    DirectionalLight(normal, eyePos);                       \n\
                                                            \n\
    v_diffuse = clamp(v_diffuse, 0.0, 1.0);                 \n\
}                                                           ";

static char s_strFS[] = "                                   \n\
precision mediump float;                                    \n\
                                                            \n\
uniform vec3    u_color;                                    \n\
uniform float   u_alpha;                                    \n\
varying vec3    v_diffuse;                                  \n\
                                                            \n\
void main(void)                                             \n\
{                                                           \n\
    vec3 color;                                             \n\
    color = u_color * v_diffuse;                            \n\
    gl_FragColor = vec4(color, u_alpha);                    \n\
}                                                           ";


static void
compute_invmat3x3 (float *matMVI3x3, float *matMV)
{
    float matMVI4x4[16];

    matrix_copy (matMVI4x4, matMV);
    matrix_invert   (matMVI4x4);
    matrix_transpose(matMVI4x4);
    matMVI3x3[0] = matMVI4x4[0];
    matMVI3x3[1] = matMVI4x4[1];
    matMVI3x3[2] = matMVI4x4[2];
    matMVI3x3[3] = matMVI4x4[4];
    matMVI3x3[4] = matMVI4x4[5];
    matMVI3x3[5] = matMVI4x4[6];
    matMVI3x3[6] = matMVI4x4[8];
    matMVI3x3[7] = matMVI4x4[9];
    matMVI3x3[8] = matMVI4x4[10];
}



int
init_stage ()
{
    generate_shader (&s_sobj, s_strVS, s_strFS);
    s_loc_mtx_mv  = glGetUniformLocation(s_sobj.program, "u_MVMatrix" );
    s_loc_mtx_pmv = glGetUniformLocation(s_sobj.program, "u_PMVMatrix" );
    s_loc_mtx_nrm = glGetUniformLocation(s_sobj.program, "u_ModelViewIT" );
    s_loc_color   = glGetUniformLocation(s_sobj.program, "u_color" );
    s_loc_alpha   = glGetUniformLocation(s_sobj.program, "u_alpha" );
    s_loc_lightpos= glGetUniformLocation(s_sobj.program, "u_LightPos" );

    shape_create (SHAPE_CYLINDER, 20, 20, &s_cylinder);
    shape_create (SHAPE_CONE,     20, 20, &s_cone);
    shape_create (SHAPE_SPHERE,   20, 20, &s_sphere);
    return 0;
}





int
draw_cylinder (float *matP, float *matV, float *matM, float radius, float length, float *color)
{
    float matB[16], matVM[16], matPVM[16], matVMI3x3[9];

    /* apply radius, length. (matB)=(matM)x(matTmp) */
    {
        float matTmp[16];
        matrix_identity (matTmp);
        matrix_scale    (matTmp, radius, radius, length * 0.5f);
        matrix_translate (matTmp, 0, 0, 1.0f);
        matrix_mult (matB, matM, matTmp);
    }


    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CW);

    glUseProgram (s_sobj.program);

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_nrm);

    matrix_mult (matVM, matV, matB);
    compute_invmat3x3 (matVMI3x3, matVM);
    matrix_mult (matPVM, matP, matVM);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matVM );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPVM);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matVMI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    if (color[3] < 1.0f)
        glEnable (GL_BLEND);

    glBindBuffer (GL_ARRAY_BUFFER, s_cylinder.vbo_vtx);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ARRAY_BUFFER, s_cylinder.vbo_nrm);
    glVertexAttribPointer (s_sobj.loc_nrm, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, s_cylinder.vbo_idx);
    glDrawElements (GL_TRIANGLES, s_cylinder.num_faces * 3, GL_UNSIGNED_SHORT, 0);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

    glFrontFace (GL_CCW);
    glDisable (GL_BLEND);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
    GLASSERT();

    return 0;
}


int
draw_cone (float *matP, float *matV, float *matM, float radius, float length, float *color)
{
    float matB[16], matVM[16], matPVM[16], matVMI3x3[9];

    /* apply radius, length. (matB)=(matM)x(matTmp) */
    {
        float matTmp[16];
        matrix_identity (matTmp);
        matrix_translate (matTmp, 0, 0, 1 + length);
        matrix_scale    (matTmp, radius, radius, length);
        //matrix_translate (matTmp, 0, 0, 1.0f);
        matrix_mult (matB, matM, matTmp);
    }


    glEnable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
    glFrontFace (GL_CW);

    glUseProgram (s_sobj.program);

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_nrm);

    matrix_mult (matVM, matV, matB);
    compute_invmat3x3 (matVMI3x3, matVM);
    matrix_mult (matPVM, matP, matVM);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matVM );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPVM);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matVMI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    if (color[3] < 1.0f)
        glEnable (GL_BLEND);

    glBindBuffer (GL_ARRAY_BUFFER, s_cone.vbo_vtx);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ARRAY_BUFFER, s_cone.vbo_nrm);
    glVertexAttribPointer (s_sobj.loc_nrm, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, s_cone.vbo_idx);
    glDrawElements (GL_TRIANGLES, s_cone.num_faces * 3, GL_UNSIGNED_SHORT, 0);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

    glFrontFace (GL_CCW);
    glDisable (GL_BLEND);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
    GLASSERT();

    return 0;
}


int
draw_sphere (float *matP, float *matV, float *matM, float radius, float *color)
{
    float matB[16], matVM[16], matPVM[16], matVMI3x3[9];

    /* apply radius. (matB)=(matM)x(matTmp) */
    {
        float matTmp[16];
        matrix_identity (matTmp);
        matrix_scale    (matTmp, radius, radius, radius);
        matrix_mult (matB, matM, matTmp);
    }


    glEnable (GL_DEPTH_TEST);
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CW);

    glUseProgram (s_sobj.program);

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_nrm);

    matrix_mult (matVM, matV, matB);
    compute_invmat3x3 (matVMI3x3, matVM);
    matrix_mult (matPVM, matP, matVM);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matVM );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPVM);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matVMI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    if (color[3] < 1.0f)
        glEnable (GL_BLEND);

    glBindBuffer (GL_ARRAY_BUFFER, s_sphere.vbo_vtx);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ARRAY_BUFFER, s_sphere.vbo_nrm);
    glVertexAttribPointer (s_sobj.loc_nrm, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, s_sphere.vbo_idx);
    glDrawElements (GL_TRIANGLES, s_sphere.num_faces * 3, GL_UNSIGNED_SHORT, 0);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

    glFrontFace (GL_CCW);
    glDisable (GL_BLEND);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
    GLASSERT();

    return 0;
}


int
draw_axis (float *matP, float *matV, float *matM)
{
    float col_r[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float col_g[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    float col_b[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    float col_w[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    float matVM[16], matPVM[16];
    matrix_mult (matVM,  matV, matM);
    matrix_mult (matPVM, matP, matVM);

    /* axis arrow */
    {
        float radius = 0.03f;
        float length = 1.0f;
        float matR[16];

        /* X axis */
        matrix_identity (matR);
        matrix_rotate (matR,  90.0f, 0.0f, 1.0f, 0.0f);
        matrix_mult (matR, matM, matR);
        draw_cylinder (matP, matV, matR, radius, length, col_r);
        draw_cone (matP, matV, matR, radius*3, 0.1, col_r);

        /* Y axis */
        matrix_identity (matR);
        matrix_rotate (matR, -90.0f, 1.0f, 0.0f, 0.0f);
        matrix_mult (matR, matM, matR);
        draw_cylinder (matP, matV, matR, radius, length, col_g);
        draw_cone (matP, matV, matR, radius*3, 0.1, col_g);

        /* Z axis */
        matrix_identity (matR);
        matrix_mult (matR, matM, matR);
        draw_cylinder (matP, matV, matR, radius, length, col_b);
        draw_cone (matP, matV, matR, radius*3, 0.1, col_b);
    }

    /* center sphere */
    {
        float radius = 0.1f;
        draw_sphere (matP, matV, matM, radius, col_w);
    }

    return 0;
}
