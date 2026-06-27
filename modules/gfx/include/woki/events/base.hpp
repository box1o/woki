#pragma once

#include <concepts>
#include <string_view>

#include "woki/events/category.hpp"
#include "woki/events/type.hpp"

namespace woki::events {

template <typename T>
concept EventLike = requires(const T event) {
    { T::GetStaticType() } -> std::same_as<EventType>;
    { event.GetEventType() } -> std::same_as<EventType>;
    { event.GetCategoryFlags() } -> std::same_as<u16>;
    { event.GetName() } -> std::convertible_to<std::string_view>;
};

class Event {
public:
    virtual ~Event() = default;

    [[nodiscard]] virtual EventType GetEventType() const noexcept = 0;
    [[nodiscard]] virtual u16 GetCategoryFlags() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;

    [[nodiscard]] bool IsInCategory(EventCategory category) const noexcept {
        return (GetCategoryFlags() & category) != 0;
    }

    bool handled{false};
    f64 timestamp{0.0};
};

} // namespace woki::events
