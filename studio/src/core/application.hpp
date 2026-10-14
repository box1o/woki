#pragma once

#include <wk/core.hpp>

#include <GLFW/glfw3.h>

namespace wk::studio {

class application {
public:
    application();
    ~application();

    application(const application&) = delete;
    application& operator=(const application&) = delete;

    bool run();

private:
    ref<GLFWwindow> window_;
    bool running_ = false;
};

} // namespace wk::studio
