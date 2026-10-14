#include <wk/core.hpp>

#include <GLFW/glfw3.h>

using namespace wk;

int main(int argc, char* argv[]) {
    slog::configure("woki", slog::Level::Debug, "woki.log");
    slog::info("Starting up....");

    if (!glfwInit()) {
        slog::critical("fail to start glfw");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 800, "woki", nullptr, nullptr);
    if (!window) {
        slog::critical("window is null");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
