/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "render_imgui.h"

#define DISPLAY_SCALE_X 1
#define DISPLAY_SCALE_Y 1
#define _X(x)       ((float)(x) / DISPLAY_SCALE_X)
#define _Y(y)       ((float)(y) / DISPLAY_SCALE_Y)

static int  s_win_w;
static int  s_win_h;

static ImVec2 s_win_size[10];
static ImVec2 s_win_pos [10];
static int    s_win_num = 0;
static ImVec2 s_mouse_pos;

int
init_imgui (int win_w, int win_h)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL3_Init(NULL);

    io.DisplaySize = ImVec2 (_X(win_w), _Y(win_h));
    io.DisplayFramebufferScale = {DISPLAY_SCALE_X, DISPLAY_SCALE_Y};

    s_win_w = win_w;
    s_win_h = win_h;

    return 0;
}

void
imgui_mousebutton (int button, int state, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(_X(x), (float)_Y(y));

    if (state)
        io.MouseDown[button] = true;
    else
        io.MouseDown[button] = false;

    s_mouse_pos.x = x;
    s_mouse_pos.y = y;
}

void
imgui_mousemove (int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(_X(x), _Y(y));

    s_mouse_pos.x = x;
    s_mouse_pos.y = y;
}

bool
imgui_is_anywindow_hovered ()
{
#if 1
    int x = _X(s_mouse_pos.x);
    int y = _Y(s_mouse_pos.y);
    for (int i = 0; i < s_win_num; i ++)
    {
        int x0 = s_win_pos[i].x;
        int y0 = s_win_pos[i].y;
        int x1 = x0 + s_win_size[i].x;
        int y1 = y0 + s_win_size[i].y;
        if ((x >= x0) && (x < x1) && (y >= y0) && (y < y1))
            return true;
    }
    return false;
#else
    return ImGui::IsAnyWindowHovered();
#endif
}

static void
render_gui (imgui_data_t *imgui_data)
{
    int win_w = 300;
    int win_h = 300;
    int win_x = 0;
    int win_y = 0;

    s_win_num = 0;

    /* Show main window */
    ImGui::SetNextWindowPos (ImVec2(_X(win_x), _Y(win_y)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(_X(win_w), _Y(win_h)), ImGuiCond_FirstUseEver);
    ImGui::Begin("Information");
    {
        ImGui::Text("Elapsed  : %d [ms]", imgui_data->elapsed_ms);
        ImGui::Text("Interval : %6.3f [ms]", imgui_data->interval_ms);
        ImGui::Text("Viewport : (%d, %d, %d, %d)", 
            imgui_data->viewport.offset.x,     imgui_data->viewport.offset.y,
            imgui_data->viewport.extent.width, imgui_data->viewport.extent.height);

        for (uint32_t i = 0; i < 2; i ++)
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("View")) 
            {
                const XrVector3f    &pos = imgui_data->views[i].pose.position;
                const XrQuaternionf &rot = imgui_data->views[i].pose.orientation;
                const XrFovf        &fov = imgui_data->views[i].fov;
                ImGui::Text("pos (%6.3f,%6.3f,%6.3f)", pos.x, pos.y, pos.z);
                ImGui::Text("rot (%6.3f,%6.3f,%6.3f,%6.3f)", rot.x, rot.y, rot.z, rot.w);
                ImGui::Text("fov (%6.3f,%6.3f,%6.3f,%6.3f)", fov.angleLeft, fov.angleRight, fov.angleUp, fov.angleDown);

                ImGui::TreePop();
            }
        }

        s_win_pos [s_win_num] = ImGui::GetWindowPos  ();
        s_win_size[s_win_num] = ImGui::GetWindowSize ();
        s_win_num ++;
    }
    ImGui::End();
}

int
invoke_imgui (imgui_data_t *imgui_data)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    render_gui (imgui_data);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return 0;
}
