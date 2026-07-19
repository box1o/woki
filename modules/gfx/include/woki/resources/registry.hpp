#pragma once

#include "metadata.hpp"
#include "pool.hpp"

#include <string>
#include <unordered_map>
#include <utility>

namespace woki::gfx::detail {

template <typename T, typename Tag> class ResourceRegistry final {
public:
    struct Entry final {
        ResourceMetadata metadata{};
        T value;

        template <typename... Args>
        Entry(ResourceMetadata metadata, Args&&... args)
            : metadata(std::move(metadata)), value(std::forward<Args>(args)...) {}
    };

    ResourceRegistry() = default;
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;
    ResourceRegistry(ResourceRegistry&&) noexcept = default;
    ResourceRegistry& operator=(ResourceRegistry&&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] Handle<Tag> Create(
        const AssetId asset_id, std::string label, const State state, Args&&... args) {
        if (asset_id) {
            const auto existing = assets_.find(asset_id);
            if (existing != assets_.end()) {
                return existing->second;
            }
        }

        ResourceMetadata metadata{
            .asset_id = asset_id,
            .version = Version::Initial(),
            .state = state,
            .label = std::move(label),
        };
        const Handle handle = pool_.Emplace(std::move(metadata), std::forward<Args>(args)...);

        if (asset_id) {
            assets_.emplace(asset_id, handle);
        }
        return handle;
    }

    [[nodiscard]] Handle<Tag> Find(const AssetId asset_id) const noexcept {
        const auto iterator = assets_.find(asset_id);
        return iterator == assets_.end() ? Handle<Tag>{} : iterator->second;
    }

    [[nodiscard]] Entry* TryGet(const Handle<Tag> handle) noexcept { return pool_.TryGet(handle); }

    [[nodiscard]] const Entry* TryGet(const Handle<Tag> handle) const noexcept {
        return pool_.TryGet(handle);
    }

    [[nodiscard]] T* TryGetValue(const Handle<Tag> handle) noexcept {
        Entry* entry = TryGet(handle);
        return entry == nullptr ? nullptr : &entry->value;
    }

    [[nodiscard]] const T* TryGetValue(const Handle<Tag> handle) const noexcept {
        const Entry* entry = TryGet(handle);
        return entry == nullptr ? nullptr : &entry->value;
    }

    [[nodiscard]] bool SetState(const Handle<Tag> handle, const State state) noexcept {
        Entry* entry = TryGet(handle);
        if (entry == nullptr) {
            return false;
        }

        entry->metadata.state = state;
        return true;
    }

    template <typename U> [[nodiscard]] bool Replace(const Handle<Tag> handle, U&& replacement) {
        Entry* entry = TryGet(handle);
        if (entry == nullptr) {
            return false;
        }

        entry->value = std::forward<U>(replacement);
        entry->metadata.version = entry->metadata.version.Next();
        entry->metadata.state = State::Resident;
        return true;
    }

    [[nodiscard]] bool Remove(const Handle<Tag> handle) {
        Entry* entry = TryGet(handle);
        if (entry == nullptr) {
            return false;
        }

        if (entry->metadata.asset_id) {
            assets_.erase(entry->metadata.asset_id);
        }
        return pool_.Remove(handle);
    }

    void Clear() {
        assets_.clear();
        pool_.Clear();
    }

    [[nodiscard]] std::size_t Size() const noexcept { return pool_.Size(); }

private:
    Pool<Entry, Tag> pool_{};
    std::unordered_map<AssetId, Handle<Tag>> assets_{};
};

} // namespace woki::gfx::detail
