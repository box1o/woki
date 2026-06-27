#pragma once

#include <string_view>

#include "woki/events/base.hpp"

namespace woki::events {

struct FrameBeginEvent final : Event {
    f32 delta_time{0.0f};

    explicit FrameBeginEvent(f32 delta_time_value)
        : delta_time(delta_time_value) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kFrameBegin;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kRender);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "FrameBegin"; }
};

struct FrameEndEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kFrameEnd; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kRender);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "FrameEnd"; }
};

struct RenderBeginEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kRenderBegin;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kRender);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "RenderBegin"; }
};

struct RenderEndEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kRenderEnd; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kRender);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "RenderEnd"; }
};

struct ViewportResizeEvent final : Event {
    u32 width{0};
    u32 height{0};

    ViewportResizeEvent(u32 width_value, u32 height_value)
        : width(width_value)
        , height(height_value) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kViewportResized;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kRender);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "ViewportResize"; }
};

struct SwapBuffersEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kSwapBuffers;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kRender);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "SwapBuffers"; }
};

} // namespace woki::events
