#include "application.hpp"
#include "woki/api/instance.hpp"
#include "woki/core.hpp"

namespace woki {

Application::Application(ApplicationSettings settings) : settings_(std::move(settings)) {
    Initialize();
}

Application::~Application() { Shutdown(); }

void Application::Initialize() {
    WindowOptions options;
    options.title = settings_.title;
    options.width = settings_.width;
    options.height = settings_.height;
    options.floating = settings_.floating;
    options.fullscreen = settings_.fullscreen;
    options.resizable = settings_.resizable;
    options.decorated = settings_.decorated;

    window_ = Window::Create(options);
    if (window_ == nullptr) {
        slog::Critical("Failed to create application window");
        return;
    }

    slog::Info("Created window '{}' ({}x{})", window_->GetTitle(), window_->GetWidth(),
        window_->GetHeight());

    // NOTE: Instance
    instance_ = api::Instance::Create({
        .backend = api::Backend::kAuto,
        .enable_validation = true,
        .enable_debug_labels = true,
    });

    if (instance_ == nullptr) {
        WOKI_PANIC("Failed to create graphics instance");
        return;
    }



}

void Application::Shutdown() noexcept {
    if (window_ != nullptr) {
        window_->Close();
        window_.reset();
    }

    is_running_ = false;
}

bool Application::IsReady() const noexcept { return window_ != nullptr; }

void Application::Start() {
    if (!is_running_) {
        slog::Info("Starting application loop");
        is_running_ = true;
    }
}

void Application::Stop() noexcept {
    if (is_running_) {
        is_running_ = false;
        slog::Info("Application loop stopped");
    }
}

bool Application::Tick() {
    if (!IsReady()) {
        slog::Critical("Application cannot run without a window");
        return false;
    }

    Start();

    if (!is_running_) {
        return false;
    }

    window_->PollEvents();

    // NOTE: Drawing and update logic would go here

    if (window_->ShouldClose()) {
        Stop();
        return false;
    }

    return true;
}

void Application::Run() { while (Tick()); }

} // namespace woki
