
#include "application.hpp"

#include <cstdlib>

namespace wk::studio {

application::application() {
    if (!glfwInit()) {
        slog::critical("Failed to initialize GLFW");
        std::abort();
    }

    window_ = ref<GLFWwindow>(
        glfwCreateWindow(800, 800, "woki", nullptr, nullptr),
        glfwDestroyWindow
    );

    if (!window_) {
        slog::critical("Failed to create GLFW window");
        glfwTerminate();
        std::abort();
    }

    glfwMakeContextCurrent(window_.get());
    glfwSwapInterval(1);

    running_ = true;
}

application::~application() {
    window_.reset();
    glfwTerminate();
}

bool application::run() {
    if (!running_ || glfwWindowShouldClose(window_.get()))
        return false;

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window_.get());
    glfwPollEvents();

    return true;
}

} // namespace wk::studio
