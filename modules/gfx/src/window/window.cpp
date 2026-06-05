#include "../../include/woki/window/window.hpp"
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <vector>

namespace woki {

struct Window::Impl {
    GLFWwindow* window{nullptr};
    GLFWcursor* cursor{nullptr};

    ~Impl() {
        if (cursor != nullptr) {
            glfwDestroyCursor(cursor);
            cursor = nullptr;
        }
    }
};

namespace {

void GlfwErrorCallback(int error, const char* description) {
    slog::Error("GLFW error ({}): {}", error, description != nullptr ? description : "(null)");
}

GLFWcursor* CreateStandardCursor(CursorType type) noexcept {
    switch (type) {
    case CursorType::kArrow:
        return glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    case CursorType::kIBeam:
        return glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    case CursorType::kCrosshair:
        return glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    case CursorType::kHand:
        return glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    case CursorType::kHResize:
        return glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    case CursorType::kVResize:
        return glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    }

    return nullptr;
}

} // namespace

scope<Window> Window::Create(const WindowOptions& options) {
    auto window = scope<Window>(new Window());
    window->impl_ = createScope<Impl>();

    if (!window->Initialize(options)) {
        return nullptr;
    }

    return window;
}

Window::~Window() { Close(); }

bool Window::Initialize(const WindowOptions& options) noexcept {
    if (!InitializeGlfw()) {
        return false;
    }

    if (!CreateGlfwWindow(options)) {
        return false;
    }

    return true;
}

bool Window::InitializeGlfw() noexcept {
    if (glfw_initialized_) {
        return true;
    }

    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) {
        slog::Critical("GLFW initialization failed");
        return false;
    }

    glfw_initialized_ = true;
    return true;
}

bool Window::CreateGlfwWindow(const WindowOptions& options) noexcept {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FLOATING, options.floating ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, options.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, options.decorated ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor = options.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    impl_->window = glfwCreateWindow(static_cast<int>(options.width),
        static_cast<int>(options.height), options.title.c_str(), monitor, nullptr);

    if (impl_->window == nullptr) {
        slog::Critical("Failed to create GLFW window");
        ShutdownGlfwIfNeeded();
        return false;
    }

    ++glfw_window_count_;

    title_ = options.title;
    fullscreen_ = options.fullscreen;

    UpdateWindowMetrics();
    SetupCallbacks();

#ifdef __EMSCRIPTEN__
    SetupEmscriptenResize();
#endif

    return true;
}

void Window::ShutdownGlfwIfNeeded() noexcept {
    if (!glfw_initialized_ || glfw_window_count_ != 0) {
        return;
    }

    glfwTerminate();
    glfw_initialized_ = false;
}

void Window::SetupCallbacks() noexcept {
    glfwSetWindowUserPointer(impl_->window, this);

    glfwSetFramebufferSizeCallback(impl_->window, [](GLFWwindow* window, int width, int height) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self == nullptr) {
            return;
        }

        self->HandleResize(static_cast<u32>(width), static_cast<u32>(height));
    });

    glfwSetWindowContentScaleCallback(
        impl_->window, [](GLFWwindow* window, float xscale, float yscale) {
            auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (self == nullptr) {
                return;
            }

            self->content_scale_x_ = xscale;
            self->content_scale_y_ = yscale;
        });
}

void Window::UpdateWindowMetrics() noexcept {
    if (impl_ == nullptr || impl_->window == nullptr) {
        return;
    }

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(impl_->window, &framebuffer_width, &framebuffer_height);

    width_ = framebuffer_width > 0 ? static_cast<u32>(framebuffer_width) : 0;
    height_ = framebuffer_height > 0 ? static_cast<u32>(framebuffer_height) : 0;
    aspect_ratio_ = height_ > 0 ? static_cast<f32>(width_) / static_cast<f32>(height_) : 1.0f;

    float xscale = 1.0f;
    float yscale = 1.0f;
    glfwGetWindowContentScale(impl_->window, &xscale, &yscale);
    content_scale_x_ = xscale;
    content_scale_y_ = yscale;
}

void Window::HandleResize(u32 width, u32 height) noexcept {
    if (width == 0 || height == 0) {
        return;
    }

    if (width == width_ && height == height_) {
        return;
    }

    width_ = width;
    height_ = height;
    aspect_ratio_ = static_cast<f32>(width_) / static_cast<f32>(height_);

    std::vector<ResizeCallback> callbacks;
    callbacks.reserve(resize_callbacks_.size());
    for (const auto& [id, callback] : resize_callbacks_) {
        (void)id;
        if (callback) {
            callbacks.push_back(callback);
        }
    }

    for (const auto& callback : callbacks) {
        callback(width_, height_);
    }
}

#ifdef __EMSCRIPTEN__
void Window::SetupEmscriptenResize() noexcept {
    auto callback = [](int, const EmscriptenUiEvent*, void* user_data) -> EM_BOOL {
        auto* self = static_cast<Window*>(user_data);
        if (self == nullptr) {
            return EM_FALSE;
        }

        double css_width = 0.0;
        double css_height = 0.0;
        emscripten_get_element_css_size("canvas", &css_width, &css_height);

        const double device_pixel_ratio = emscripten_get_device_pixel_ratio();
        const u32 pixel_width = static_cast<u32>(css_width * device_pixel_ratio);
        const u32 pixel_height = static_cast<u32>(css_height * device_pixel_ratio);

        if (pixel_width == 0 || pixel_height == 0) {
            return EM_FALSE;
        }

        emscripten_set_canvas_element_size(
            "canvas", static_cast<int>(pixel_width), static_cast<int>(pixel_height));
        self->HandleResize(pixel_width, pixel_height);
        return EM_TRUE;
    };

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, callback);
    UpdateWindowMetrics();
}
#endif

void* Window::GetNativeHandle() const noexcept {
    if (impl_ == nullptr || impl_->window == nullptr) {
        return nullptr;
    }

    return static_cast<void*>(impl_->window);
}

bool Window::ShouldClose() const noexcept {
    if (impl_ == nullptr || impl_->window == nullptr) {
        return true;
    }

    return glfwWindowShouldClose(impl_->window) != 0;
}

void Window::PollEvents() const noexcept {
    if (!glfw_initialized_) {
        return;
    }

    glfwPollEvents();
}

void Window::WaitEvents() const noexcept {
    if (!glfw_initialized_) {
        return;
    }

    glfwWaitEvents();
}

void Window::Close() noexcept {
    if (impl_ == nullptr || impl_->window == nullptr) {
        return;
    }

    if (impl_->cursor != nullptr) {
        glfwDestroyCursor(impl_->cursor);
        impl_->cursor = nullptr;
    }

    glfwSetWindowUserPointer(impl_->window, nullptr);
    glfwDestroyWindow(impl_->window);
    impl_->window = nullptr;

    if (glfw_window_count_ > 0) {
        --glfw_window_count_;
    }

    ShutdownGlfwIfNeeded();
}

void Window::SetCursorMode(CursorMode mode) noexcept {
    if (impl_ == nullptr || impl_->window == nullptr) {
        return;
    }

    int glfw_cursor_mode = GLFW_CURSOR_NORMAL;
    switch (mode) {
    case CursorMode::kNormal:
        glfw_cursor_mode = GLFW_CURSOR_NORMAL;
        break;
    case CursorMode::kHidden:
        glfw_cursor_mode = GLFW_CURSOR_HIDDEN;
        break;
    case CursorMode::kDisabled:
        glfw_cursor_mode = GLFW_CURSOR_DISABLED;
        break;
    }

    glfwSetInputMode(impl_->window, GLFW_CURSOR, glfw_cursor_mode);
    cursor_mode_ = mode;
}

void Window::SetCursorType(CursorType type) noexcept {
    if (impl_ == nullptr || impl_->window == nullptr) {
        return;
    }

    GLFWcursor* cursor = CreateStandardCursor(type);
    if (cursor == nullptr) {
        slog::Warn("Failed to create GLFW cursor for type {}", static_cast<int>(type));
        return;
    }

    glfwSetCursor(impl_->window, cursor);
    if (impl_->cursor != nullptr) {
        glfwDestroyCursor(impl_->cursor);
    }

    impl_->cursor = cursor;
    cursor_type_ = type;
}

CallbackId Window::AddResizeCallback(ResizeCallback callback) {
    const CallbackId callback_id = next_callback_id_++;
    resize_callbacks_.emplace(callback_id, std::move(callback));
    return callback_id;
}

void Window::RemoveResizeCallback(CallbackId id) { resize_callbacks_.erase(id); }

const std::string& Window::GetTitle() const noexcept { return title_; }

u32 Window::GetWidth() const noexcept { return width_; }

u32 Window::GetHeight() const noexcept { return height_; }

f32 Window::GetAspectRatio() const noexcept { return aspect_ratio_; }

bool Window::IsFullscreen() const noexcept { return fullscreen_; }

f32 Window::GetContentScaleX() const noexcept { return content_scale_x_; }

f32 Window::GetContentScaleY() const noexcept { return content_scale_y_; }

CursorMode Window::GetCursorMode() const noexcept { return cursor_mode_; }

CursorType Window::GetCursorType() const noexcept { return cursor_type_; }

} // namespace woki
