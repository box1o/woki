#pragma once

#include <string_view>

#include "woki/events/base.hpp"

namespace woki::events {

struct WindowCloseEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowClosed;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowClose"; }
};

struct WindowFocusEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowFocused;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowFocus"; }
};

struct WindowLostFocusEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowLostFocus;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowLostFocus"; }
};

struct WindowResizeEvent final : Event {
    u32 width{0};
    u32 height{0};

    WindowResizeEvent(u32 width_value, u32 height_value)
        : width(width_value)
        , height(height_value) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowResized;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowResize"; }
};

struct WindowMovedEvent final : Event {
    i32 x{0};
    i32 y{0};

    WindowMovedEvent(i32 x_value, i32 y_value)
        : x(x_value)
        , y(y_value) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowMoved;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowMoved"; }
};

struct WindowScaleChangedEvent final : Event {
    f32 x{1.0f};
    f32 y{1.0f};

    WindowScaleChangedEvent(f32 x_value, f32 y_value)
        : x(x_value)
        , y(y_value) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowScaleChanged;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override {
        return "WindowScaleChanged";
    }
};

struct WindowMinimizedEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowMinimized;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowMinimized"; }
};

struct WindowMaximizedEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowMaximized;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowMaximized"; }
};

struct WindowRestoredEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kWindowRestored;
    }

    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kWindow);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "WindowRestored"; }
};

} // namespace woki::events
