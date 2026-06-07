#pragma once

#include <string_view>

#include "woki/events/base.hpp"

namespace woki::events {

struct AppTickEvent final : Event {
    f32 delta_time{0.0f};

    explicit AppTickEvent(f32 delta_time)
        : delta_time(delta_time) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kAppTick; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kApplication);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "AppTick"; }
};

struct AppUpdateEvent final : Event {
    f32 delta_time{0.0f};

    explicit AppUpdateEvent(f32 delta_time)
        : delta_time(delta_time) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kAppUpdate;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kApplication);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "AppUpdate"; }
};

struct AppRenderEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kAppRender; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kApplication);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "AppRender"; }
};

struct AppShutdownEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kAppShutdown;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kApplication);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "AppShutdown"; }
};

struct AppSuspendEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kAppSuspend;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kApplication);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "AppSuspend"; }
};

struct AppResumeEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kAppResume; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kApplication);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "AppResume"; }
};

} // namespace woki::events
