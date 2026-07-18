#include <woki/gfx/animation/animation.hpp>

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace woki::gfx {
namespace {

template <typename T>
[[nodiscard]] bool ValidKeys(const std::span<const AnimationKey<T>> keys, const f32 duration) {
    f32 previous = -1.0F;
    for (const auto& key : keys) {
        if (!std::isfinite(key.time) || key.time < 0.0F || key.time > duration ||
            key.time <= previous) {
            return false;
        }
        previous = key.time;
    }
    return true;
}

[[nodiscard]] math::vec3f Interpolate(const std::span<const AnimationKey<math::vec3f>> keys,
    const f32 time, const math::vec3f fallback) {
    if (keys.empty()) {
        return fallback;
    }
    if (keys.size() == 1 || time <= keys.front().time) {
        return keys.front().value;
    }
    if (time >= keys.back().time) {
        return keys.back().value;
    }
    const auto upper = std::ranges::upper_bound(keys, time, {}, &AnimationKey<math::vec3f>::time);
    const auto& right = *upper;
    const auto& left = *(upper - 1);
    const f32 factor = (time - left.time) / (right.time - left.time);
    return left.value * (1.0F - factor) + right.value * factor;
}

[[nodiscard]] math::quatf Interpolate(const std::span<const AnimationKey<math::quatf>> keys,
    const f32 time, const math::quatf fallback) {
    if (keys.empty()) {
        return fallback;
    }
    if (keys.size() == 1 || time <= keys.front().time) {
        return keys.front().value;
    }
    if (time >= keys.back().time) {
        return keys.back().value;
    }
    const auto upper = std::ranges::upper_bound(keys, time, {}, &AnimationKey<math::quatf>::time);
    const auto& right = *upper;
    const auto& left = *(upper - 1);
    const f32 factor = (time - left.time) / (right.time - left.time);
    return math::slerp(left.value, right.value, factor);
}

} // namespace

math::mat4f ToMatrix(const Transform& transform) noexcept {
    return math::translate(transform.translation) * transform.rotation.toMat4() *
           math::scale(transform.scale);
}

Result<void> Validate(const Skeleton& skeleton) {
    std::unordered_set<std::string> names{};
    for (u32 index = 0; index < skeleton.joints.size(); ++index) {
        const auto& joint = skeleton.joints[index];
        if (joint.name.empty() || !names.insert(joint.name).second) {
            return Err(
                ErrorCode::ValidationInvalidState, "Skeleton joints require unique nonempty names");
        }
        if (joint.parent >= static_cast<i32>(index) || joint.parent < -1) {
            return Err(
                ErrorCode::ValidationOutOfRange, "Skeleton parents must precede their children");
        }
        if (joint.bind_transform.rotation.length_squared() <= 0.0F) {
            return Err(ErrorCode::ValidationNullValue, "Skeleton bind rotations must be nonzero");
        }
    }
    return Ok();
}

Result<void> Validate(const AnimationClip& clip, const u32 joint_count) {
    if (!std::isfinite(clip.duration) || clip.duration < 0.0F) {
        return Err(
            ErrorCode::ValidationOutOfRange, "Animation duration must be finite and nonnegative");
    }
    std::unordered_set<u32> joints{};
    for (const auto& track : clip.tracks) {
        if (track.joint >= joint_count || !joints.insert(track.joint).second) {
            return Err(
                ErrorCode::ValidationOutOfRange, "Animation tracks require unique valid joints");
        }
        if (!ValidKeys(std::span{track.translations}, clip.duration) ||
            !ValidKeys(std::span{track.rotations}, clip.duration) ||
            !ValidKeys(std::span{track.scales}, clip.duration)) {
            return Err(ErrorCode::ValidationInvalidState,
                "Animation keys must have strictly increasing in-range times");
        }
    }
    return Ok();
}

Result<AnimationPose> Evaluate(
    const Skeleton& skeleton, const AnimationClip& clip, f32 time, const AnimationWrapMode wrap) {
    if (auto result = Validate(skeleton); !result) {
        return Err(result.error());
    }
    if (auto result = Validate(clip, static_cast<u32>(skeleton.joints.size())); !result) {
        return Err(result.error());
    }
    if (!std::isfinite(time)) {
        return Err(ErrorCode::ValidationOutOfRange, "Animation time must be finite");
    }
    if (clip.duration > 0.0F) {
        if (wrap == AnimationWrapMode::Loop) {
            time = std::fmod(time, clip.duration);
            if (time < 0.0F) {
                time += clip.duration;
            }
        } else {
            time = std::clamp(time, 0.0F, clip.duration);
        }
    } else {
        time = 0.0F;
    }

    AnimationPose pose{};
    pose.local.reserve(skeleton.joints.size());
    for (const auto& joint : skeleton.joints) {
        pose.local.push_back(joint.bind_transform);
    }
    for (const auto& track : clip.tracks) {
        auto& transform = pose.local[track.joint];
        transform.translation = Interpolate(track.translations, time, transform.translation);
        transform.rotation = Interpolate(track.rotations, time, transform.rotation).normalized();
        transform.scale = Interpolate(track.scales, time, transform.scale);
    }

    pose.global.resize(skeleton.joints.size());
    pose.skin_matrices.resize(skeleton.joints.size());
    for (u32 index = 0; index < skeleton.joints.size(); ++index) {
        const auto& joint = skeleton.joints[index];
        const math::mat4f local = ToMatrix(pose.local[index]);
        pose.global[index] =
            joint.parent < 0 ? local : pose.global[static_cast<u32>(joint.parent)] * local;
        pose.skin_matrices[index] = pose.global[index] * joint.inverse_bind;
    }
    return Ok(std::move(pose));
}

} // namespace woki::gfx
