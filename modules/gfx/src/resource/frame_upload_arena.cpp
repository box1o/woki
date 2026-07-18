#include <woki/gfx/resource/frame_upload_arena.hpp>

#include <algorithm>
#include <cstring>
#include <limits>

namespace woki::gfx {
namespace {

[[nodiscard]] constexpr bool IsPowerOfTwo(const u64 value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

[[nodiscard]] constexpr u64 AlignUp(const u64 value, const u64 alignment) noexcept {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

Result<FrameUploadArena> FrameUploadArena::Create(const FrameUploadArenaDesc& desc) {
    if (desc.capacity_per_frame == 0 || desc.frames_in_flight == 0) {
        return Err(ErrorCode::ValidationOutOfRange,
            "Frame upload arena requires nonzero capacity and frame count");
    }
    if (!IsPowerOfTwo(desc.default_alignment)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Frame upload arena alignment must be a power of two");
    }
    if (desc.capacity_per_frame > std::numeric_limits<std::size_t>::max() / desc.frames_in_flight) {
        return Err(ErrorCode::ValidationOutOfRange, "Frame upload arena capacity overflows");
    }
    return Ok(FrameUploadArena(desc));
}

FrameUploadArena::FrameUploadArena(const FrameUploadArenaDesc& desc)
    : desc_(desc),
      storage_(static_cast<std::size_t>(desc.capacity_per_frame * desc.frames_in_flight)),
      segments_(desc.frames_in_flight) {}

Result<void> FrameUploadArena::BeginFrame(const u64 frame_number, const u64 completed_submission) {
    if (active_) {
        return Err(
            ErrorCode::ValidationInvalidState, "Frame upload arena already has an active frame");
    }
    active_segment_ = static_cast<u32>(frame_number % desc_.frames_in_flight);
    if (segments_[active_segment_].last_submission > completed_submission) {
        return Err(
            ErrorCode::InvalidState, "Frame upload arena segment is still in use by the GPU");
    }
    cursor_ = 0;
    active_ = true;
    return Ok();
}

Result<FrameUploadAllocation> FrameUploadArena::Write(
    const std::span<const std::byte> data, u64 alignment) {
    if (!active_) {
        return Err(ErrorCode::InvalidState, "Frame upload arena has no active frame");
    }
    if (data.empty()) {
        return Err(ErrorCode::ValidationNullValue, "Frame upload allocation cannot be empty");
    }
    alignment = alignment == 0 ? desc_.default_alignment : alignment;
    if (!IsPowerOfTwo(alignment)) {
        return Err(ErrorCode::ValidationInvalidState,
            "Frame upload allocation alignment must be a power of two");
    }
    const u64 local_offset = AlignUp(cursor_, alignment);
    if (local_offset > desc_.capacity_per_frame ||
        data.size() > desc_.capacity_per_frame - local_offset) {
        return Err(ErrorCode::ValidationOutOfRange, "Frame upload arena segment is exhausted");
    }
    const u64 absolute_offset =
        static_cast<u64>(active_segment_) * desc_.capacity_per_frame + local_offset;
    std::memcpy(storage_.data() + absolute_offset, data.data(), data.size());
    cursor_ = local_offset + data.size();
    return Ok(FrameUploadAllocation{.offset = absolute_offset, .size = data.size()});
}

Result<void> FrameUploadArena::MarkSubmitted(const u64 submission) {
    if (!active_) {
        return Err(ErrorCode::InvalidState, "Frame upload arena has no active frame");
    }
    if (submission == 0) {
        return Err(ErrorCode::ValidationOutOfRange, "GPU submission identity must be nonzero");
    }
    segments_[active_segment_].last_submission = submission;
    active_ = false;
    return Ok();
}

void FrameUploadArena::AbortFrame() noexcept {
    cursor_ = 0;
    active_ = false;
}

std::span<const std::byte> FrameUploadArena::PendingData() const noexcept {
    if (!active_ || cursor_ == 0) {
        return {};
    }
    return std::span(storage_).subspan(PendingOffset(), cursor_);
}

u64 FrameUploadArena::PendingOffset() const noexcept {
    return active_ ? static_cast<u64>(active_segment_) * desc_.capacity_per_frame : 0;
}

u64 FrameUploadArena::Capacity() const noexcept { return storage_.size(); }
u64 FrameUploadArena::CapacityPerFrame() const noexcept { return desc_.capacity_per_frame; }
u64 FrameUploadArena::Used() const noexcept { return active_ ? cursor_ : 0; }
bool FrameUploadArena::FrameActive() const noexcept { return active_; }

} // namespace woki::gfx
