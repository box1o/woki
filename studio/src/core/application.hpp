#pragma once

#include <functional>
#include <unordered_map>

#include <woki/core.hpp>
#ifndef __EMSCRIPTEN__
#include <woki/ext/ext.hpp>
#endif
#include <woki/gfx.hpp>

namespace woki {

struct ApplicationSettings {
    std::string title{"studio"};
    u32 width{1280};
    u32 height{720};
    bool floating{false};
    bool fullscreen{false};
    bool resizable{true};
    bool decorated{true};
};

class Application {
public:
    using EventCallback = std::function<void(events::Event&)>;

    explicit Application(ApplicationSettings settings = {});
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void Run();
    [[nodiscard]] bool Tick();

    CallbackId AddEventCallback(EventCallback callback);
    void RemoveEventCallback(CallbackId id);

private:
    void Initialize();
    void Shutdown() noexcept;
    [[nodiscard]] bool IsReady() const noexcept;
    [[nodiscard]] bool IsGraphicsReady() const noexcept;
    void BeginGraphicsInitialization();
    void FinalizeGraphicsInitialization();
    void Start();
    void Stop() noexcept;
    void EmitEvent(events::Event& event);
#ifndef __EMSCRIPTEN__
    void LoadSourceExtensions();
    void DispatchEventToExtensions(const events::Event& event);
#endif

    template <typename T, typename... Args> void EmitEvent(Args&&... args) {
        T event(std::forward<Args>(args)...);
        EmitEvent(event);
    }

    ApplicationSettings settings_{};
    scope<Window> window_;
    scope<api::Instance> instance_;
    scope<api::Surface> surface_;
    scope<api::Adapter> adapter_;
    scope<api::Device> device_;
    scope<api::Swapchain> swapchain_;
#ifndef __EMSCRIPTEN__
    scope<ext::Manager> extensions_;
#endif
    std::unordered_map<CallbackId, EventCallback> event_callbacks_;
    CallbackId next_callback_id_{1};
    CallbackId window_event_callback_id_{0};
    CallbackId window_resize_callback_id_{0};
    FrameTimer frame_timer_{};
    bool graphics_initialization_started_{false};
    bool graphics_initialization_failed_{false};
    bool is_running_{false};
};

} // namespace woki
