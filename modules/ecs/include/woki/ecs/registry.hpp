#pragma once

#include <woki/core.hpp>

#include "entity.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace woki {

template <bool IsConst, typename... Components>
class BasicView;

template <typename T>
concept Component =
    !std::is_const_v<T> && !std::is_volatile_v<T> && !std::is_reference_v<T> &&
    std::is_object_v<T> && std::destructible<T>;

namespace detail {

[[nodiscard]] constexpr u32 NextEntityGeneration(u32 generation) noexcept {
    const u32 next = generation + 1;
    return next == Entity::kInvalidGeneration ? 0u : next;
}

class IComponentStorage {
public:
    virtual ~IComponentStorage() = default;

    [[nodiscard]] virtual bool Remove(Entity entity) noexcept = 0;
    virtual void Clear() noexcept = 0;

    [[nodiscard]] virtual std::size_t Size() const noexcept = 0;
    [[nodiscard]] virtual std::span<const Entity> DenseEntities() const noexcept = 0;
};

template <Component T>
class ComponentStorage final : public IComponentStorage {
public:
    static constexpr u32 kInvalidPosition = std::numeric_limits<u32>::max();

    template <typename... Args>
    T& Emplace(Entity entity, Args&&... args) {
        WOKI_ASSERT_MSG(!Contains(entity), "Entity already owns component");

        const u32 index = entity.Index();
        EnsureSparse(index);

        const u32 position = static_cast<u32>(entities_.size());
        sparse_[index] = position;
        entities_.push_back(entity);
        data_.emplace_back(std::forward<Args>(args)...);
        return data_.back();
    }

    [[nodiscard]] bool Contains(Entity entity) const noexcept {
        const u32 index = entity.Index();
        if (index >= sparse_.size()) {
            return false;
        }

        const u32 position = sparse_[index];
        return position != kInvalidPosition && position < entities_.size() && entities_[position] == entity;
    }

    [[nodiscard]] T* TryGet(Entity entity) noexcept {
        if (!Contains(entity)) {
            return nullptr;
        }

        return &data_[sparse_[entity.Index()]];
    }

    [[nodiscard]] const T* TryGet(Entity entity) const noexcept {
        if (!Contains(entity)) {
            return nullptr;
        }

        return &data_[sparse_[entity.Index()]];
    }

    T& Get(Entity entity) {
        T* value = TryGet(entity);
        WOKI_ASSERT_MSG(value != nullptr, "Entity does not own component");
        return *value;
    }

    const T& Get(Entity entity) const {
        const T* value = TryGet(entity);
        WOKI_ASSERT_MSG(value != nullptr, "Entity does not own component");
        return *value;
    }

    [[nodiscard]] bool Remove(Entity entity) noexcept override {
        const u32 index = entity.Index();
        if (index >= sparse_.size()) {
            return false;
        }

        const u32 position = sparse_[index];
        if (position == kInvalidPosition || position >= entities_.size() || entities_[position] != entity) {
            return false;
        }

        const u32 last_position = static_cast<u32>(entities_.size() - 1);
        if (position != last_position) {
            entities_[position] = entities_[last_position];
            data_[position] = std::move(data_[last_position]);
            sparse_[entities_[position].Index()] = position;
        }

        entities_.pop_back();
        data_.pop_back();
        sparse_[index] = kInvalidPosition;
        return true;
    }

    void Clear() noexcept override {
        sparse_.clear();
        entities_.clear();
        data_.clear();
    }

    [[nodiscard]] std::size_t Size() const noexcept override {
        return entities_.size();
    }

    [[nodiscard]] std::span<const Entity> DenseEntities() const noexcept override {
        return entities_;
    }

private:
    void EnsureSparse(u32 index) {
        if (index >= sparse_.size()) {
            sparse_.resize(static_cast<std::size_t>(index) + 1, kInvalidPosition);
        }
    }

    std::vector<u32> sparse_;
    std::vector<Entity> entities_;
    std::vector<T> data_;
};

} // namespace detail

class Registry final {
public:
    static constexpr u32 kInvalidPosition = std::numeric_limits<u32>::max();

    Registry() = default;
    ~Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

    [[nodiscard]] Entity Create() {
        u32 index = Entity::kInvalidIndex;

        if (!free_list_.empty()) {
            index = free_list_.back();
            free_list_.pop_back();
            active_[index] = true;
        } else {
            index = static_cast<u32>(generations_.size());
            generations_.push_back(0);
            active_.push_back(true);
            entity_positions_.push_back(kInvalidPosition);
        }

        const Entity entity(index, generations_[index]);
        entity_positions_[index] = static_cast<u32>(entities_.size());
        entities_.push_back(entity);
        ++alive_count_;
        return entity;
    }

    [[nodiscard]] Entity Spawn() {
        return Create();
    }

    [[nodiscard]] bool Destroy(Entity entity) {
        if (!Valid(entity)) {
            return false;
        }

        for (auto& [_, storage] : storages_) {
            (void)storage->Remove(entity);
        }

        const u32 index = entity.Index();
        const u32 position = entity_positions_[index];
        const u32 last_position = static_cast<u32>(entities_.size() - 1);
        if (position != last_position) {
            entities_[position] = entities_[last_position];
            entity_positions_[entities_[position].Index()] = position;
        }

        entities_.pop_back();
        entity_positions_[index] = kInvalidPosition;
        active_[index] = false;
        generations_[index] = detail::NextEntityGeneration(generations_[index]);
        free_list_.push_back(index);
        --alive_count_;
        return true;
    }

    [[nodiscard]] bool Valid(Entity entity) const noexcept {
        if (!entity.Valid()) {
            return false;
        }

        const u32 index = entity.Index();
        return index < generations_.size() && active_[index] && generations_[index] == entity.Generation();
    }

    [[nodiscard]] bool Alive(Entity entity) const noexcept {
        return Valid(entity);
    }

    void Clear() noexcept {
        generations_.clear();
        active_.clear();
        entity_positions_.clear();
        entities_.clear();
        free_list_.clear();
        alive_count_ = 0;

        for (auto& [_, storage] : storages_) {
            storage->Clear();
        }
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        return alive_count_;
    }

    [[nodiscard]] bool Empty() const noexcept {
        return alive_count_ == 0;
    }

    [[nodiscard]] std::span<const Entity> Entities() const noexcept {
        return entities_;
    }

    [[nodiscard]] std::size_t Capacity() const noexcept {
        return generations_.size();
    }

    template <typename Func>
    void EachEntity(Func&& func) const {
        for (Entity entity : entities_) {
            std::invoke(std::forward<Func>(func), entity);
        }
    }

    template <Component T, typename... Args>
    T& Emplace(Entity entity, Args&&... args) {
        WOKI_ASSERT_MSG(Valid(entity), "Cannot add component to dead entity");
        return EnsureStorage<T>().Emplace(entity, std::forward<Args>(args)...);
    }

    template <Component T, typename... Args>
    T& GetOrEmplace(Entity entity, Args&&... args) {
        WOKI_ASSERT_MSG(Valid(entity), "Cannot access component on dead entity");

        if (T* value = TryGet<T>(entity)) {
            return *value;
        }

        return EnsureStorage<T>().Emplace(entity, std::forward<Args>(args)...);
    }

    template <Component T>
    [[nodiscard]] bool Has(Entity entity) const noexcept {
        if (!Valid(entity)) {
            return false;
        }

        const auto* storage = FindStorage<T>();
        return storage != nullptr && storage->Contains(entity);
    }

    template <Component... Components>
    [[nodiscard]] bool AllOf(Entity entity) const noexcept {
        return (Has<Components>(entity) && ...);
    }

    template <Component... Components>
    [[nodiscard]] bool AnyOf(Entity entity) const noexcept {
        return (Has<Components>(entity) || ...);
    }

    template <Component T>
    [[nodiscard]] T* TryGet(Entity entity) noexcept {
        if (!Valid(entity)) {
            return nullptr;
        }

        auto* storage = FindStorage<T>();
        return storage != nullptr ? storage->TryGet(entity) : nullptr;
    }

    template <Component T>
    [[nodiscard]] const T* TryGet(Entity entity) const noexcept {
        if (!Valid(entity)) {
            return nullptr;
        }

        const auto* storage = FindStorage<T>();
        return storage != nullptr ? storage->TryGet(entity) : nullptr;
    }

    template <Component T>
    T& Get(Entity entity) {
        T* value = TryGet<T>(entity);
        WOKI_ASSERT_MSG(value != nullptr, "Entity does not own component");
        return *value;
    }

    template <Component T>
    const T& Get(Entity entity) const {
        const T* value = TryGet<T>(entity);
        WOKI_ASSERT_MSG(value != nullptr, "Entity does not own component");
        return *value;
    }

    template <Component T>
    [[nodiscard]] bool Remove(Entity entity) noexcept {
        auto* storage = FindStorage<T>();
        return storage != nullptr && storage->Remove(entity);
    }

    template <Component T>
    [[nodiscard]] std::size_t Count() const noexcept {
        const auto* storage = FindStorage<T>();
        return storage != nullptr ? storage->Size() : 0u;
    }

    template <Component T>
    void Clear() noexcept {
        auto* storage = FindStorage<T>();
        if (storage == nullptr) {
            return;
        }

        storage->Clear();
    }

    template <Component... Components>
    [[nodiscard]] auto View() & -> BasicView<false, Components...>;

    template <Component... Components>
    [[nodiscard]] auto View() const & -> BasicView<true, Components...>;

private:
    template <bool IsConst, typename... Components>
    friend class BasicView;

    template <Component T>
    [[nodiscard]] detail::ComponentStorage<T>* FindStorage() noexcept {
        const auto it = storages_.find(std::type_index(typeid(T)));
        if (it == storages_.end()) {
            return nullptr;
        }

        return static_cast<detail::ComponentStorage<T>*>(it->second.get());
    }

    template <Component T>
    [[nodiscard]] const detail::ComponentStorage<T>* FindStorage() const noexcept {
        const auto it = storages_.find(std::type_index(typeid(T)));
        if (it == storages_.end()) {
            return nullptr;
        }

        return static_cast<const detail::ComponentStorage<T>*>(it->second.get());
    }

    template <Component T>
    detail::ComponentStorage<T>& EnsureStorage() {
        const auto key = std::type_index(typeid(T));
        const auto it = storages_.find(key);
        if (it != storages_.end()) {
            return *static_cast<detail::ComponentStorage<T>*>(it->second.get());
        }

        auto storage = std::make_unique<detail::ComponentStorage<T>>();
        auto* storage_ptr = storage.get();
        storages_.emplace(key, std::move(storage));
        return *storage_ptr;
    }

    std::vector<u32> generations_;
    std::vector<bool> active_;
    std::vector<u32> entity_positions_;
    std::vector<Entity> entities_;
    std::vector<u32> free_list_;
    std::unordered_map<std::type_index, std::unique_ptr<detail::IComponentStorage>> storages_;
    std::size_t alive_count_ = 0;
};

template <bool IsConst, typename... Components>
class BasicView final {
public:
    static_assert((Component<Components> && ...), "Views only support component types");
    static_assert(sizeof...(Components) > 0, "Views require at least one component type");

    using RegistryType = std::conditional_t<IsConst, const Registry, Registry>;

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Entity;
        using difference_type = std::ptrdiff_t;

        Iterator() = default;

        explicit Iterator(const BasicView* view, std::size_t index) noexcept
            : view_(view), index_(index) {
            Advance();
        }

        [[nodiscard]] Entity operator*() const noexcept {
            return view_->entities_[index_];
        }

        Iterator& operator++() noexcept {
            ++index_;
            Advance();
            return *this;
        }

        [[nodiscard]] bool operator==(const Iterator& other) const noexcept = default;

    private:
        void Advance() noexcept {
            while (view_ != nullptr && index_ < view_->entities_.size() && !view_->ContainsAll(view_->entities_[index_])) {
                ++index_;
            }
        }

        const BasicView* view_ = nullptr;
        std::size_t index_ = 0;
    };

    explicit BasicView(RegistryType& registry) noexcept
        : registry_(&registry) {
        SelectLeadStorage();
    }

    [[nodiscard]] Iterator begin() const noexcept {
        return Iterator(this, 0);
    }

    [[nodiscard]] Iterator end() const noexcept {
        return Iterator(this, entities_.size());
    }

    [[nodiscard]] bool Empty() const noexcept {
        return begin() == end();
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        std::size_t count = 0;
        for ([[maybe_unused]] Entity entity : *this) {
            ++count;
        }
        return count;
    }

    [[nodiscard]] auto Get(Entity entity) const {
        WOKI_ASSERT_MSG(ContainsAll(entity), "Entity is not part of this view");

        if constexpr (IsConst) {
            return std::forward_as_tuple(registry_->template Get<Components>(entity)...);
        } else {
            return std::forward_as_tuple(registry_->template Get<Components>(entity)...);
        }
    }

    template <typename Func>
    void Each(Func&& func) const {
        for (Entity entity : *this) {
            if constexpr (std::is_invocable_v<Func, Entity, decltype(registry_->template Get<Components>(entity))...>) {
                std::invoke(std::forward<Func>(func), entity, registry_->template Get<Components>(entity)...);
            } else {
                std::invoke(std::forward<Func>(func), registry_->template Get<Components>(entity)...);
            }
        }
    }

private:
    template <Component T>
    [[nodiscard]] const detail::IComponentStorage* CandidateStorage() const noexcept {
        if constexpr (IsConst) {
            return registry_->template FindStorage<T>();
        } else {
            return registry_->template FindStorage<T>();
        }
    }

    void SelectLeadStorage() noexcept {
        const detail::IComponentStorage* lead = nullptr;

        auto select_candidate = [&](const detail::IComponentStorage* candidate) {
            if (candidate == nullptr) {
                lead = nullptr;
                return false;
            }

            if (lead == nullptr || candidate->Size() < lead->Size()) {
                lead = candidate;
            }

            return true;
        };

        if (!(select_candidate(CandidateStorage<Components>()) && ...)) {
            entities_ = {};
            return;
        }

        entities_ = lead != nullptr ? lead->DenseEntities() : std::span<const Entity>{};
    }

    [[nodiscard]] bool ContainsAll(Entity entity) const noexcept {
        return registry_->template AllOf<Components...>(entity);
    }

    RegistryType* registry_ = nullptr;
    std::span<const Entity> entities_{};
};

template <Component... Components>
[[nodiscard]] inline auto Registry::View() & -> BasicView<false, Components...> {
    return BasicView<false, Components...>(*this);
}

template <Component... Components>
[[nodiscard]] inline auto Registry::View() const & -> BasicView<true, Components...> {
    return BasicView<true, Components...>(*this);
}

} // namespace woki
