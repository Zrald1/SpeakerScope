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
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.FrameRounding = 4.f;
    s.Colors[ImGuiCol_WindowBg] = theme::kBg;
    s.Colors[ImGuiCol_ChildBg] = theme::kPanel;
    s.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.32f, 0.45f, 1.f);
    s.Colors[ImGuiCol_ButtonHovered] = theme::kAccent;

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
