#include "gui.h"
#include <imgui.h>
#include <cpu/cpu.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

void GUI::init(GLFWwindow* _window)
{
	window = _window;
    log.open("log.txt");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    const char* glsl_version = "#version 330";

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void GUI::start()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUI::display()
{
    ImGui::Render();
    int display_w, display_h;
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        
        glfwMakeContextCurrent(backup_current_context);
    }
}

void GUI::dockspace()
{
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", (bool*)0, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Docking")) {
            if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
            ImGui::Separator();
            ImGui::EndMenu();
        }        

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void GUI::decompiler(CPU* cpu)
{
    ImGui::Begin("Decompiler");

    auto opcode = cpu->instr.opcode();
    std::string instr;
    if (opcode == 0)
        instr = cpu->str_special[opcode];
    else
        instr = cpu->str_lookup[opcode];

    ImGui::Text("pc: 0x%x", cpu->pc);
    ImGui::Text("insruction: 0x%x - %s", opcode, instr.c_str());

    if (ImGui::Button("Start Log")) {
        should_log = true;
    }

    if (ImGui::Button("Stop Log")) {
        should_log = false;
    }

    if (should_log) {
        log << "pc: 0x" << std::hex << cpu->pc;
        log << " instruction: %s" << instr.c_str() << '\n';
    }    

    auto vres = cpu->bus->gpu->state.status.vres;
    auto hres = cpu->bus->gpu->state.status.hres;

    int width = 0, height = 0;
    switch (hres & 0x1) {
    case 0: {
        switch (hres >> 1) {
        case 0: width = 256; break;
        case 1: width = 320; break;
        case 2: width = 512; break;
        case 3: width = 640; break;
        }
        break;
    }
    case 1: {
        width = 368;
    }
    }

    switch (vres) {
    case 0: height = 240; break;
    case 1: height = 480; break;
    }

    ImGui::Text("Resolution X: %d Y: %d", width, height);

    ImGui::End();
}

void GUI::viewport(uint32_t texture)
{
    ImGui::Begin("Viewport");
    
    ImVec2 pos = ImGui::GetCursorScreenPos();

    auto size = ImGui::GetWindowSize();
    
    Bus* bus = (Bus*)glfwGetWindowUserPointer(window);

    auto vres = bus->gpu->state.status.vres;
    auto hres = bus->gpu->state.status.hres;

    int width = 0, height = 0;
    switch (hres & 0x1) {
    case 0: {
        switch (hres >> 1) {
        case 0: width = 256; break;
        case 1: width = 320; break;
        case 2: width = 512; break;
        case 3: width = 640; break;
        }
        break;
    }
    case 1: {
        width = 368;
    }
    }

    switch (vres) {
    case 0: height = 240; break;
    case 1: height = 480; break;
    }

    double x = width / size.x;
    double y = height / size.y;

    if (display_vram) {
        x = 1;
        y = 1;
    }

    ImGui::GetWindowDrawList()->AddImage(
        (void*)texture, ImVec2(ImGui::GetCursorScreenPos()),
        ImVec2(ImGui::GetCursorScreenPos().x + size.x, ImGui::GetCursorScreenPos().y + size.y), ImVec2(0, 0), ImVec2(x, y));

    ImGui::End();
}

void GUI::cpu_regs(CPU* cpu)
{
    ImGui::Begin("CPU registers");

    for (int i = 0; i < 32; i++) {
        ImGui::Text("reg 0x%x: 0x%x", i, cpu->registers[i]);
    }

    ImGui::End();
}
