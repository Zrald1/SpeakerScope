#pragma once

#include <memory>
#include "app/SessionController.h"
#include "core/Config.h"

namespace ss {

class AppWindow;

class Application {
public:
    explicit Application(Config cfg);
    ~Application();
    int run(); // returns exit code; blocks until window closes

private:
    Config cfg_;
    SessionController session_;
    std::unique_ptr<AppWindow> window_;
};

} // namespace ss
