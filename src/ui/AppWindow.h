#pragma once

#include <string>

struct GLFWwindow;

namespace ss {

// GLFW + OpenGL3 + Dear ImGui frame lifecycle.
class AppWindow {
public:
    AppWindow(int width, int height, const char* title);
    ~AppWindow();

    bool shouldClose() const;
    void beginFrame();
    void endFrame();
    GLFWwindow* handle() const { return window_; }

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

private:
    GLFWwindow* window_ = nullptr;
};

} // namespace ss
