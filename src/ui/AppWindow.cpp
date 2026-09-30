#include "ui/AppWindow.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>

#include "core/Logger.h"
#include "ui/Theme.h"

namespace ss {

AppWindow::AppWindow(int width, int height, const char* title) {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) throw std::runtime_error("glfwCreateWindow failed");
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Typography: Segoe UI on Windows; ImGui default as fallback.
    ImGuiIO& io = ImGui::GetIO();
    if (auto* f = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf",
                                             17.0f))
        io.FontDefault = f;
    title_font_ = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeuib.ttf",
                                               22.0f);
    small_font_ = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf",
                                               14.0f);

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 12.f;
    s.ChildRounding = 10.f;
    s.FrameRounding = 7.f;
    s.PopupRounding = 8.f;
    s.ScrollbarRounding = 8.f;
    s.GrabRounding = 7.f;
    s.TabRounding = 6.f;
    s.WindowPadding = ImVec2(16, 14);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(8, 6);
    s.ScrollbarSize = 11.f;
    s.WindowBorderSize = 0.f;
    s.ChildBorderSize = 1.f;
    s.FrameBorderSize = 0.f;
    s.PopupBorderSize = 1.f;

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg] = theme::kBg;
    c[ImGuiCol_ChildBg] = theme::kPanel;
    c[ImGuiCol_PopupBg] = theme::kPanelAlt;
    c[ImGuiCol_Border] = ImVec4(theme::kBorder.x, theme::kBorder.y,
                                theme::kBorder.z, 0.55f);
    c[ImGuiCol_Text] = theme::kText;
    c[ImGuiCol_TextDisabled] = theme::kMuted;
    c[ImGuiCol_FrameBg] = theme::kTrack;
    c[ImGuiCol_FrameBgHovered] = theme::kPanelAlt;
    c[ImGuiCol_FrameBgActive] = theme::kPanelAlt;
    c[ImGuiCol_Button] = theme::kAccentDim;
    c[ImGuiCol_ButtonHovered] = theme::kAccent;
    c[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.55f, 0.62f, 1.f);
    c[ImGuiCol_CheckMark] = theme::kAccent;
    c[ImGuiCol_SliderGrab] = theme::kAccent;
    c[ImGuiCol_Header] = theme::kAccentDim;
    c[ImGuiCol_HeaderHovered] = theme::kAccentSoft;
    c[ImGuiCol_HeaderActive] = theme::kAccentSoft;
    c[ImGuiCol_Separator] = theme::kBorder;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.33f, 0.38f, 0.8f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.42f, 0.48f, 0.9f);
    c[ImGuiCol_TitleBgActive] = theme::kAccentDim;

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    log()->info("UI window created ({}x{})", width, height);
}

AppWindow::~AppWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool AppWindow::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void AppWindow::beginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void AppWindow::endFrame() {
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.075f, 0.08f, 0.095f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}

} // namespace ss
