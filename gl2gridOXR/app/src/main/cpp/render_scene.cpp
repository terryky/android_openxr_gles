#include <GLES3/gl31.h>
#include "util_egl.h"
#include "util_oxr.h"
#include "util_render2d.h"
#include "util_debugstr.h"

static GLuint           s_fbo_swapchain;


static void
draw_grid_lefttop (int win_w, int win_h)
{
    float col_gray[]   = {0.73f, 0.75f, 0.75f, 1.0f};
    float col_blue[]   = {0.00f, 0.00f, 1.00f, 1.0f};
    float col_red []   = {1.00f, 0.00f, 0.00f, 1.0f};
    float col_cyan[]   = {0.00f, 1.00f, 1.00f, 1.0f};
    float *col;
    int cx = win_w / 2;
    int cy = win_h / 2;

    set_2d_projection_matrix (win_w, win_h);

    col = col_gray;
    for (int y = 0; y < win_h; y += 10)
        draw_2d_line (0, y, win_w, y, col, 1.0f);

    for (int x = 0; x < win_w; x += 10)
        draw_2d_line (x, 0, x, win_h, col, 1.0f);

    col = col_blue;
    for (int y = 0; y < win_h; y += 100)
        draw_2d_line (0, y, win_w, y, col, 1.0f);

    for (int x = 0; x < win_w; x += 100)
        draw_2d_line (x, 0, x, win_h, col, 1.0f);
    
    col = col_red;
    for (int y = 0; y < win_h; y += 500)
        draw_2d_line (0, y, win_w, y, col, 1.0f);

    for (int x = 0; x < win_w; x += 500)
        draw_2d_line (x, 0, x, win_h, col, 1.0f);

    float col_green[]  = {0.00f, 1.00f, 0.00f, 1.0f};
    draw_2d_line ( 0, cy, win_w, cy,    col_green, 1.0f);
    draw_2d_line (cx, 0,  cx,    win_h, col_green, 1.0f);

    int radius = std::max (win_w, win_h);
    for (int r = 0; r < radius; r += 100)
        draw_2d_circle (cx, cy, r, 50, col_blue, 1.0f);

    for (int r = 0; r < radius; r += 500)
        draw_2d_circle (cx, cy, r, 50, col_red, 1.0f);
}


static void
draw_grid (int win_w, int win_h)
{
    float col_gray[]   = {0.73f, 0.75f, 0.75f, 1.0f};
    float col_blue[]   = {0.00f, 0.00f, 1.00f, 1.0f};
    float col_red []   = {1.00f, 0.00f, 0.00f, 1.0f};
    float col_cyan[]   = {0.00f, 1.00f, 1.00f, 1.0f};
    float *col;
    int cx = win_w / 2;
    int cy = win_h / 2;

    set_2d_projection_matrix (win_w, win_h);

    col = col_gray;
    for (int y = 0; y < cy; y += 10)
    {
        draw_2d_line (0, cy + y, win_w, cy + y, col, 1.0f);
        draw_2d_line (0, cy - y, win_w, cy - y, col, 1.0f);
    }

    for (int x = 0; x < cx; x += 10)
    {
        draw_2d_line (cx + x, 0, cx + x, win_h, col, 1.0f);
        draw_2d_line (cx - x, 0, cx - x, win_h, col, 1.0f);
    }

    col = col_blue;
    for (int y = 0; y < cy; y += 100)
    {
        draw_2d_line (0, cy + y, win_w, cy + y, col, 1.0f);
        draw_2d_line (0, cy - y, win_w, cy - y, col, 1.0f);
    }

    for (int x = 0; x < cx; x += 100)
    {
        draw_2d_line (cx + x, 0, cx + x, win_h, col, 1.0f);
        draw_2d_line (cx - x, 0, cx - x, win_h, col, 1.0f);
    }

    col = col_red;
    for (int y = 0; y < cy; y += 500)
    {
        draw_2d_line (0, cy + y, win_w, cy + y, col, 1.0f);
        draw_2d_line (0, cy - y, win_w, cy - y, col, 1.0f);
    }

    for (int x = 0; x < cx; x += 500)
    {
        draw_2d_line (cx + x, 0, cx + x, win_h, col, 1.0f);
        draw_2d_line (cx - x, 0, cx - x, win_h, col, 1.0f);
    }

    /* circle */
    int radius = std::max (win_w, win_h);
    for (int r = 0; r < radius; r += 100)
        draw_2d_circle (cx, cy, r, 50, col_blue, 1.0f);

    for (int r = 0; r < radius; r += 500)
        draw_2d_circle (cx, cy, r, 50, col_red, 1.0f);


    /* strings */
    update_dbgstr_winsize (win_w, win_h);
    for (int x = 0; x < cx; x += 100)
    {
        char strbuf[128];
        sprintf(strbuf, "%d", x);
        draw_dbgstr(strbuf, cx + x, cy);

        sprintf(strbuf, "%d", -x);
        draw_dbgstr(strbuf, cx - x, cy);
    }

    for (int y = 0; y < cy; y += 100)
    {
        char strbuf[128];
        sprintf(strbuf, "%d", y);
        draw_dbgstr(strbuf, cx, cy + y);

        sprintf(strbuf, "%d", -y);
        draw_dbgstr(strbuf, cx, cy - y);
    }
}


int
init_gles_scene ()
{
    init_dbgstr (0, 0);
    init_2d_renderer (0, 0);

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

    draw_grid (view_w, view_h);


    {
        int cx = view_w / 2;
        int cy = view_h / 2;

        char strbuf[128];
        sprintf(strbuf, "Viewport(%d, %d, %d, %d)", view_x, view_y, view_w, view_h);
        draw_dbgstr(strbuf, cx + 100, cy + 100);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 0;
}


