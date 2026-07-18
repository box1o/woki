#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <woki/core.hpp>

namespace woki::gfx {

struct FrameUploadArenaDesc final {
    u64 capacity_per_frame{0};
    u32 frames_in_flight{0};
    u64 default_alignment{1};
};

struct FrameUploadAllocation final {
    u64 offset{0};
    u64 size{0};
};

class FrameUploadArena final {
public:
    [[nodiscard]] static Result<FrameUploadArena> Create(const FrameUploadArenaDesc& desc);

    [[nodiscard]] Result<void> BeginFrame(u64 frame_number, u64 completed_submission);
    [[nodiscard]] Result<FrameUploadAllocation> Write(
        std::span<const std::byte> data, u64 alignment = 0);
    [[nodiscard]] Result<void> MarkSubmitted(u64 submission);

    [[nodiscard]] std::span<const std::byte> PendingData() const noexcept;
    [[nodiscard]] u64 PendingOffset() const noexcept;
    [[nodiscard]] u64 Capacity() const noexcept;
    [[nodiscard]] u64 CapacityPerFrame() const noexcept;
    [[nodiscard]] u64 Used() const noexcept;
    [[nodiscard]] bool FrameActive() const noexcept;

private:
    explicit FrameUploadArena(const FrameUploadArenaDesc& desc);

    struct Segment final {
        u64 last_submission{0};
    };

    FrameUploadArenaDesc desc_{};
    std::vector<std::byte> storage_{};
    std::vector<Segment> segments_{};
    u32 active_segment_{0};
    u64 cursor_{0};
    bool active_{false};
};

} // namespace woki::gfx
