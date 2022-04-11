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
render_gui (scene_data_t *scn_data)
{
    struct InputState *input = &scn_data->inputState;
    int win_w = 300;
    int win_h = 0;
    int win_x = 0;
    int win_y = 0;

    s_win_num = 0;

    /* Show main window */
    win_y += win_h;
    win_h = 120;
    ImGui::SetNextWindowPos (ImVec2(_X(win_x), _Y(win_y)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(_X(win_w), _Y(win_h)), ImGuiCond_FirstUseEver);
    ImGui::Begin("Runtime");
    {
        ImGui::Text("OXR_RUNTIME: %s", scn_data->runtime_name.c_str());
        ImGui::Text("OXR_SYSTEM : %s", scn_data->system_name.c_str());
        ImGui::Text("GL_VERSION : %s", scn_data->gl_version);
        ImGui::Text("GL_VENDOR  : %s", scn_data->gl_vendor);
        ImGui::Text("GL_RENDERER: %s", scn_data->gl_render);

        s_win_pos [s_win_num] = ImGui::GetWindowPos  ();
        s_win_size[s_win_num] = ImGui::GetWindowSize ();
        s_win_num ++;
    }
    ImGui::End();

    win_y += win_h;
    win_h = 220;
    ImGui::SetNextWindowPos (ImVec2(_X(win_x), _Y(win_y)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(_X(win_w), _Y(win_h)), ImGuiCond_FirstUseEver);
    ImGui::Begin("View Info");
    {
        ImGui::Text("Elapsed  : %d [ms]",    scn_data->elapsed_us / 1000);
        ImGui::Text("Interval : %6.3f [ms]", scn_data->interval_ms);
        ImGui::Text("Viewport : (%d, %d, %d, %d)", 
            scn_data->viewport.offset.x,     scn_data->viewport.offset.y,
            scn_data->viewport.extent.width, scn_data->viewport.extent.height);

        for (uint32_t i = 0; i < 2; i ++)
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("View")) 
            {
                const XrVector3f    &pos = scn_data->views[i].pose.position;
                const XrQuaternionf &rot = scn_data->views[i].pose.orientation;
                const XrFovf        &fov = scn_data->views[i].fov;
                ImGui::Text("pos (%6.3f,%6.3f,%6.3f)",       pos.x, pos.y, pos.z);
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

    /* Left Hand Controller */
    win_y += win_h;
    win_h = 200;
    ImGui::SetNextWindowPos (ImVec2(_X(win_x), _Y(win_y)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(_X(win_w), _Y(win_h)), ImGuiCond_FirstUseEver);
    ImGui::Begin("Left Hand");
    {
        ImGui::Checkbox("X button",   &input->clickX);
        ImGui::Checkbox("Y button",   &input->clickY);
        ImGui::Checkbox("S button",   &input->clickS[0]);
        ImGui::SliderFloat("squeeze", &input->squeezeVal[0],  0.0f, 1.0f);
        ImGui::SliderFloat("trigger", &input->triggerVal[0],  0.0f, 1.0f);
        ImGui::SliderFloat("stick_x", &input->stickVal[0].x, -1.0f, 1.0f);
        ImGui::SliderFloat("stick_x", &input->stickVal[0].y, -1.0f, 1.0f);

        s_win_pos [s_win_num] = ImGui::GetWindowPos  ();
        s_win_size[s_win_num] = ImGui::GetWindowSize ();
        s_win_num ++;
    }
    ImGui::End();

    /* Right Hand Controller */
    win_y += win_h;
    win_h = 200;
    ImGui::SetNextWindowPos (ImVec2(_X(win_x), _Y(win_y)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(_X(win_w), _Y(win_h)), ImGuiCond_FirstUseEver);
    ImGui::Begin("Right Hand");
    {
        ImGui::Checkbox("A button",   &input->clickA);
        ImGui::Checkbox("B button",   &input->clickB);
        ImGui::Checkbox("S button",   &input->clickS[1]);
        ImGui::SliderFloat("squeeze", &input->squeezeVal[1],  0.0f, 1.0f);
        ImGui::SliderFloat("trigger", &input->triggerVal[1],  0.0f, 1.0f);
        ImGui::SliderFloat("stick_x", &input->stickVal[1].x, -1.0f, 1.0f);
        ImGui::SliderFloat("stick_x", &input->stickVal[1].y, -1.0f, 1.0f);

        s_win_pos [s_win_num] = ImGui::GetWindowPos  ();
        s_win_size[s_win_num] = ImGui::GetWindowSize ();
        s_win_num ++;
    }
    ImGui::End();
}

int
invoke_imgui (scene_data_t *scn_data)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    render_gui (scn_data);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return 0;
}
