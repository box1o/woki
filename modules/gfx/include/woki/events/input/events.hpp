#pragma once

#include <string_view>

#include "woki/events/base.hpp"

namespace woki::events {

enum class KeyCode : u16 {
    kSpace = 32,
    kApostrophe = 39,
    kComma = 44,
    kMinus = 45,
    kPeriod = 46,
    kSlash = 47,

    kD0 = 48,
    kD1,
    kD2,
    kD3,
    kD4,
    kD5,
    kD6,
    kD7,
    kD8,
    kD9,

    kSemicolon = 59,
    kEqual = 61,

    kA = 65,
    kB,
    kC,
    kD,
    kE,
    kF,
    kG,
    kH,
    kI,
    kJ,
    kK,
    kL,
    kM,
    kN,
    kO,
    kP,
    kQ,
    kR,
    kS,
    kT,
    kU,
    kV,
    kW,
    kX,
    kY,
    kZ,

    kLeftBracket = 91,
    kBackslash = 92,
    kRightBracket = 93,
    kGraveAccent = 96,

    kEscape = 256,
    kEnter,
    kTab,
    kBackspace,
    kInsert,
    kDelete,
    kRight,
    kLeft,
    kDown,
    kUp,
    kPageUp,
    kPageDown,
    kHome,
    kEnd,

    kCapsLock = 280,
    kScrollLock,
    kNumLock,
    kPrintScreen,
    kPause,

    kF1 = 290,
    kF2,
    kF3,
    kF4,
    kF5,
    kF6,
    kF7,
    kF8,
    kF9,
    kF10,
    kF11,
    kF12,

    kLeftShift = 340,
    kLeftControl,
    kLeftAlt,
    kLeftSuper,
    kRightShift,
    kRightControl,
    kRightAlt,
    kRightSuper,

    kMenu = 348,
};

enum class MouseButton : u8 {
    kLeft = 0,
    kRight = 1,
    kMiddle = 2,
    kButton4 = 3,
    kButton5 = 4,
    kButton6 = 5,
    kButton7 = 6,
    kButton8 = 7,
};

struct KeyPressedEvent final : Event {
    KeyCode key{KeyCode::kSpace};
    u32 repeat_count{0};

    KeyPressedEvent(KeyCode key, u32 repeat_count = 0)
        : key(key)
        , repeat_count(repeat_count) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kKeyPressed; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kKeyboard | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "KeyPressed"; }
};

struct KeyReleasedEvent final : Event {
    KeyCode key{KeyCode::kSpace};

    explicit KeyReleasedEvent(KeyCode key)
        : key(key) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kKeyReleased;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kKeyboard | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "KeyReleased"; }
};

struct KeyTypedEvent final : Event {
    u32 character{0};

    explicit KeyTypedEvent(u32 character)
        : character(character) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kKeyTyped; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kKeyboard | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "KeyTyped"; }
};

struct MouseMovedEvent final : Event {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 delta_x{0.0f};
    f32 delta_y{0.0f};

    MouseMovedEvent(f32 x, f32 y, f32 delta_x, f32 delta_y)
        : x(x)
        , y(y)
        , delta_x(delta_x)
        , delta_y(delta_y) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kMouseMoved;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kMouse | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "MouseMoved"; }
};

struct MouseScrolledEvent final : Event {
    f32 offset_x{0.0f};
    f32 offset_y{0.0f};

    MouseScrolledEvent(f32 offset_x, f32 offset_y)
        : offset_x(offset_x)
        , offset_y(offset_y) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kMouseScrolled;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kMouse | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "MouseScrolled"; }
};

struct MouseButtonPressedEvent final : Event {
    MouseButton button{MouseButton::kLeft};
    f32 x{0.0f};
    f32 y{0.0f};

    MouseButtonPressedEvent(MouseButton button, f32 x, f32 y)
        : button(button)
        , x(x)
        , y(y) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kMouseButtonPressed;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(
            EventCategory::kMouse | EventCategory::kMouseButton | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override {
        return "MouseButtonPressed";
    }
};

struct MouseButtonReleasedEvent final : Event {
    MouseButton button{MouseButton::kLeft};
    f32 x{0.0f};
    f32 y{0.0f};

    MouseButtonReleasedEvent(MouseButton button, f32 x, f32 y)
        : button(button)
        , x(x)
        , y(y) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kMouseButtonReleased;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(
            EventCategory::kMouse | EventCategory::kMouseButton | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override {
        return "MouseButtonReleased";
    }
};

struct MouseButtonClickedEvent final : Event {
    MouseButton button{MouseButton::kLeft};
    f32 x{0.0f};
    f32 y{0.0f};

    MouseButtonClickedEvent(MouseButton button, f32 x, f32 y)
        : button(button)
        , x(x)
        , y(y) {}

    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kMouseButtonClicked;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(
            EventCategory::kMouse | EventCategory::kMouseButton | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override {
        return "MouseButtonClicked";
    }
};

struct MouseEnteredEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept {
        return EventType::kMouseEntered;
    }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kMouse | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "MouseEntered"; }
};

struct MouseLeftEvent final : Event {
    [[nodiscard]] static constexpr EventType GetStaticType() noexcept { return EventType::kMouseLeft; }
    [[nodiscard]] EventType GetEventType() const noexcept override { return GetStaticType(); }
    [[nodiscard]] u16 GetCategoryFlags() const noexcept override {
        return static_cast<u16>(EventCategory::kMouse | EventCategory::kInput);
    }
    [[nodiscard]] std::string_view GetName() const noexcept override { return "MouseLeft"; }
};

} // namespace woki::events
