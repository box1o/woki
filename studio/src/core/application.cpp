#include "application.hpp"

#include <cstdlib>

namespace wk::studio {

application::application() {
    if (!glfwInit()) {
        slog::critical("Failed to initialize GLFW");
        std::abort();
    }

    window_ = ref<GLFWwindow>(glfwCreateWindow(800, 800, "woki", nullptr, nullptr), glfwDestroyWindow);

    if (!window_) {
        slog::critical("Failed to create GLFW window");
        glfwTerminate();
        std::abort();
    }

    glfwMakeContextCurrent(window_.get());
    glfwSwapInterval(1);

    running_ = true;

    const math::vec3f position(2.0F, 1.0F, -3.0F);
    const math::vec3f direction = math::normalize(math::vec3f(1.0F, 2.0F, 0.0F));
    const math::mat4f model = math::translate(position) * math::rotate_z(math::radians(30.0F))
                              * math::scale(math::vec3f(1.5F, 1.5F, 1.5F));

    slog::info("position: {}, direction: {}", position, direction);
    slog::info("model matrix:\n{}", model);
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
