#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <woki/core.hpp>

namespace woki {

enum class CursorMode : u8 { kNormal, kHidden, kDisabled };
enum class CursorType : u8 { kArrow, kIBeam, kCrosshair, kHand, kHResize, kVResize };

using CallbackId = u32;

struct WindowOptions {
    std::string title{"woki"};
    u32 width{1280};
    u32 height{720};
    bool floating{false};
    bool fullscreen{false};
    bool resizable{true};
    bool decorated{true};
};

class Window final {
public:
    using ResizeCallback = std::function<void(u32 width, u32 height)>;

    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    [[nodiscard]] static scope<Window> Create(const WindowOptions& options = {});

    [[nodiscard]] const std::string& GetTitle() const noexcept;
    [[nodiscard]] u32 GetWidth() const noexcept;
    [[nodiscard]] u32 GetHeight() const noexcept;
    [[nodiscard]] f32 GetAspectRatio() const noexcept;
    [[nodiscard]] bool IsFullscreen() const noexcept;

    [[nodiscard]] f32 GetContentScaleX() const noexcept;
    [[nodiscard]] f32 GetContentScaleY() const noexcept;

    [[nodiscard]] CursorMode GetCursorMode() const noexcept;
    void SetCursorMode(CursorMode mode) noexcept;

    [[nodiscard]] CursorType GetCursorType() const noexcept;
    void SetCursorType(CursorType type) noexcept;

    [[nodiscard]] void* GetNativeHandle() const noexcept;

    [[nodiscard]] bool ShouldClose() const noexcept;
    void PollEvents() const noexcept;
    void WaitEvents() const noexcept;
    void Close() noexcept;

    CallbackId AddResizeCallback(ResizeCallback callback);
    void RemoveResizeCallback(CallbackId id);

private:
    Window() = default;

    bool Initialize(const WindowOptions& options) noexcept;
    bool InitializeGlfw() noexcept;
    bool CreateGlfwWindow(const WindowOptions& options) noexcept;
    static void ShutdownGlfwIfNeeded() noexcept;

    void SetupCallbacks() noexcept;
    void UpdateWindowMetrics() noexcept;
    void HandleResize(u32 width, u32 height) noexcept;

#ifdef __EMSCRIPTEN__
    void SetupEmscriptenResize() noexcept;
#endif

    struct Impl;
    scope<Impl> impl_;

    std::unordered_map<CallbackId, ResizeCallback> resize_callbacks_;
    CallbackId next_callback_id_{1};

    std::string title_{"woki"};
    u32 width_{0};
    u32 height_{0};
    f32 aspect_ratio_{1.0f};
    bool fullscreen_{false};

    CursorMode cursor_mode_{CursorMode::kNormal};
    CursorType cursor_type_{CursorType::kArrow};

    f32 content_scale_x_{1.0f};
    f32 content_scale_y_{1.0f};

    static inline u32 glfw_window_count_ = 0;
    static inline bool glfw_initialized_ = false;
};

} // namespace woki
