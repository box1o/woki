#pragma once

#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <woki/gfx/resource/resource_handle.hpp>

namespace woki::gfx::detail {

template <typename T, typename Tag> class ResourcePool final {
public:
    using Handle = ResourceHandle<Tag>;

    ResourcePool() = default;

    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) noexcept = default;
    ResourcePool& operator=(ResourcePool&&) noexcept = default;

    template <typename... Args> [[nodiscard]] Handle Emplace(Args&&... args) {
        if (!free_indices_.empty()) {
            const u32 index = free_indices_.back();
            free_indices_.pop_back();

            Slot& slot = slots_[index];
            WOKI_ASSERT(!slot.value.has_value());
            slot.value.emplace(std::forward<Args>(args)...);
            ++size_;
            return Handle::FromParts(index, slot.generation);
        }

        WOKI_ASSERT(slots_.size() < static_cast<std::size_t>(Handle::kInvalidIndex));
        const u32 index = static_cast<u32>(slots_.size());
        slots_.emplace_back(std::forward<Args>(args)...);
        ++size_;
        return Handle::FromParts(index, slots_.back().generation);
    }

    [[nodiscard]] bool Contains(const Handle handle) const noexcept {
        if (!handle.Valid() || handle.Index() >= slots_.size()) {
            return false;
        }

        const Slot& slot = slots_[handle.Index()];
        return slot.value.has_value() && slot.generation == handle.Generation();
    }

    [[nodiscard]] T* TryGet(const Handle handle) noexcept {
        if (!Contains(handle)) {
            return nullptr;
        }

        return &*slots_[handle.Index()].value;
    }

    [[nodiscard]] const T* TryGet(const Handle handle) const noexcept {
        if (!Contains(handle)) {
            return nullptr;
        }

        return &*slots_[handle.Index()].value;
    }

    [[nodiscard]] T& Get(const Handle handle) {
        T* value = TryGet(handle);
        WOKI_ASSERT_MSG(value != nullptr, "Invalid resource handle");
        return *value;
    }

    [[nodiscard]] const T& Get(const Handle handle) const {
        const T* value = TryGet(handle);
        WOKI_ASSERT_MSG(value != nullptr, "Invalid resource handle");
        return *value;
    }

    [[nodiscard]] bool Remove(const Handle handle) {
        if (!Contains(handle)) {
            return false;
        }

        Slot& slot = slots_[handle.Index()];
        slot.value.reset();
        slot.generation = NextGeneration(slot.generation);
        free_indices_.push_back(handle.Index());
        --size_;
        return true;
    }

    void Clear() {
        free_indices_.clear();
        free_indices_.reserve(slots_.size());

        for (u32 index = 0; index < slots_.size(); ++index) {
            Slot& slot = slots_[index];
            slot.value.reset();
            slot.generation = NextGeneration(slot.generation);
            free_indices_.push_back(index);
        }

        size_ = 0;
    }

    template <typename Function> void Each(Function&& function) {
        for (u32 index = 0; index < slots_.size(); ++index) {
            Slot& slot = slots_[index];
            if (slot.value.has_value()) {
                std::invoke(function, Handle::FromParts(index, slot.generation), *slot.value);
            }
        }
    }

    template <typename Function> void Each(Function&& function) const {
        for (u32 index = 0; index < slots_.size(); ++index) {
            const Slot& slot = slots_[index];
            if (slot.value.has_value()) {
                std::invoke(function, Handle::FromParts(index, slot.generation), *slot.value);
            }
        }
    }

    [[nodiscard]] std::size_t Size() const noexcept { return size_; }

    [[nodiscard]] bool Empty() const noexcept { return size_ == 0; }

    [[nodiscard]] std::size_t Capacity() const noexcept { return slots_.size(); }

private:
    struct Slot final {
        template <typename... Args>
        explicit Slot(Args&&... args) : value(std::in_place, std::forward<Args>(args)...) {}

        std::optional<T> value{};
        u32 generation{1};
    };

    [[nodiscard]] static constexpr u32 NextGeneration(const u32 generation) noexcept {
        const u32 next = generation + 1u;
        return next == Handle::kInvalidGeneration ? 1u : next;
    }

    std::vector<Slot> slots_{};
    std::vector<u32> free_indices_{};
    std::size_t size_{0};
};

} // namespace woki::gfx::detail
