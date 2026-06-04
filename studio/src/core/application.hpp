#pragma once

#include <woki/core.hpp>
#include <woki/gui.hpp>

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
    explicit Application(ApplicationSettings settings = {});
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void Run();
    [[nodiscard]] bool Tick();

private:
    void Initialize();
    void Shutdown() noexcept;
    [[nodiscard]] bool IsReady() const noexcept;
    void Start();
    void Stop() noexcept;

    ApplicationSettings settings_{};
    scope<Window> window_;
    bool is_running_{false};
};

} // namespace woki
