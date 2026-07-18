#pragma once

#include <span>
#include <string>
#include <vector>

#include <woki/core.hpp>
#include <woki/math/math.hpp>

namespace woki::gfx {

enum class AnimationWrapMode : u8 {
    Clamp = 0,
    Loop,
};

struct Transform final {
    math::vec3f translation{};
    math::quatf rotation{0.0F, 0.0F, 0.0F, 1.0F};
    math::vec3f scale{1.0F};
};

template <typename T> struct AnimationKey final {
    f32 time{0.0F};
    T value{};
};

struct AnimationTrack final {
    u32 joint{0};
    std::vector<AnimationKey<math::vec3f>> translations{};
    std::vector<AnimationKey<math::quatf>> rotations{};
    std::vector<AnimationKey<math::vec3f>> scales{};
};

struct AnimationClip final {
    std::string name{};
    f32 duration{0.0F};
    std::vector<AnimationTrack> tracks{};
};

struct SkeletonJoint final {
    std::string name{};
    i32 parent{-1};
    Transform bind_transform{};
    math::mat4f inverse_bind{math::mat4f::identity()};
};

struct Skeleton final {
    std::vector<SkeletonJoint> joints{};
};

struct AnimationPose final {
    std::vector<Transform> local{};
    std::vector<math::mat4f> global{};
    std::vector<math::mat4f> skin_matrices{};
};

[[nodiscard]] math::mat4f ToMatrix(const Transform& transform) noexcept;
[[nodiscard]] Result<void> Validate(const Skeleton& skeleton);
[[nodiscard]] Result<void> Validate(const AnimationClip& clip, u32 joint_count);
[[nodiscard]] Result<AnimationPose> Evaluate(const Skeleton& skeleton, const AnimationClip& clip,
    f32 time, AnimationWrapMode wrap = AnimationWrapMode::Loop);

} // namespace woki::gfx
