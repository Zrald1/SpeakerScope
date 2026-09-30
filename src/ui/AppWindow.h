#pragma once

#include <string>

struct GLFWwindow;
struct ImFont;

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
    ImFont* titleFont() const { return title_font_; }
    ImFont* smallFont() const { return small_font_; }

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

private:
    GLFWwindow* window_ = nullptr;
    ImFont* title_font_ = nullptr;
    ImFont* small_font_ = nullptr;
};

} // namespace ss
