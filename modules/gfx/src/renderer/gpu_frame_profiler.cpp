#include "gpu_frame_profiler.hpp"

namespace woki::gfx::detail {

Result<u64> ResolveGpuDuration(const u64 beginning, const u64 end) {
    if (end < beginning) {
        return Err(ErrorCode::ValidationInvalidState, "GPU timestamp range ends before it begins");
    }
    return Ok(end - beginning);
}

GpuFrameProfiler::GpuFrameProfiler(rhi::Device& device) : device_(&device) { Initialize(); }

GpuFrameProfiler::~GpuFrameProfiler() = default;

void GpuFrameProfiler::Initialize() {
    supported_ = device_->HasFeature(rhi::FeatureName::TimestampQuery);
    if (!supported_) {
        return;
    }

    auto query_set = device_->CreateQuerySet({
        .type = rhi::QueryType::Timestamp,
        .count = 2,
        .label = "Renderer frame timestamps",
    });
    if (!query_set) {
        initialization_error_ = query_set.error();
        return;
    }
    query_set_ = std::move(*query_set);

    for (auto& slot : slots_) {
        auto resolve = device_->CreateBuffer({
            .size = kTimestampBytes,
            .usage = rhi::BufferUsage::QueryResolve | rhi::BufferUsage::CopySrc,
            .label = "Renderer timestamp resolve",
        });
        if (!resolve) {
            initialization_error_ = resolve.error();
            query_set_.reset();
            return;
        }
        auto readback = device_->CreateBuffer({
            .size = kTimestampBytes,
            .usage = rhi::BufferUsage::CopyDst | rhi::BufferUsage::MapRead,
            .label = "Renderer timestamp readback",
        });
        if (!readback) {
            initialization_error_ = readback.error();
            query_set_.reset();
            return;
        }
        slot.resolve = std::move(*resolve);
        slot.readback = std::move(*readback);
    }
}

bool GpuFrameProfiler::Supported() const noexcept { return supported_; }

bool GpuFrameProfiler::Active() const noexcept {
    return query_set_ != nullptr && !initialization_error_;
}

const std::optional<Error>& GpuFrameProfiler::InitializationError() const noexcept {
    return initialization_error_;
}

Result<bool> GpuFrameProfiler::Capture(RhiRenderGraphFrame& frame) {
    active_slot_.reset();
    if (!Active()) {
        return Ok(false);
    }
    const auto slot = std::ranges::find_if(
        slots_, [](const Slot& candidate) { return !candidate.submitted && !candidate.mapping; });
    if (slot == slots_.end()) {
        return Ok(false);
    }
    const auto index = static_cast<std::size_t>(std::distance(slots_.begin(), slot));
    if (auto capture = frame.CaptureTimestamps(*query_set_, *slot->resolve, *slot->readback);
        !capture) {
        return Err(capture.error());
    }
    active_slot_ = index;
    return Ok(true);
}

void GpuFrameProfiler::MarkSubmitted(const u64 submission) noexcept {
    if (!active_slot_) {
        return;
    }
    Slot& slot = slots_[*active_slot_];
    slot.submission = submission;
    slot.submitted = true;
    active_slot_.reset();
}

void GpuFrameProfiler::CancelCapture() noexcept { active_slot_.reset(); }

void GpuFrameProfiler::StartReadback(Slot& slot) {
    auto state = std::make_shared<MapState>();
    const auto future = slot.readback->MapAsync(rhi::MapMode::Read, 0, kTimestampBytes,
        rhi::CallbackMode::AllowProcessEvents,
        [state](const rhi::MapAsyncStatus status, std::string_view) {
            state->status.store(
                status == rhi::MapAsyncStatus::Success ? 1 : -1, std::memory_order_release);
        });
    if (future.id == 0) {
        state->status.store(-1, std::memory_order_release);
    }
    slot.mapping = std::move(state);
}

std::optional<GpuFrameTimingSample> GpuFrameProfiler::Poll(const u64 completed_submission) {
    if (!Active()) {
        return std::nullopt;
    }
    device_->Tick();
    std::optional<GpuFrameTimingSample> newest{};
    for (auto& slot : slots_) {
        if (slot.mapping) {
            const i32 status = slot.mapping->status.load(std::memory_order_acquire);
            if (status == 0) {
                continue;
            }
            if (status > 0) {
                std::array<u64, 2> timestamps{};
                if (auto read =
                        slot.readback->ReadMappedRange(0, timestamps.data(), kTimestampBytes);
                    read) {
                    auto duration = ResolveGpuDuration(timestamps[0], timestamps[1]);
                    if (duration && (!newest || slot.submission > newest->submission)) {
                        newest = GpuFrameTimingSample{
                            .submission = slot.submission,
                            .duration_ns = *duration,
                        };
                    }
                }
                slot.readback->Unmap();
            }
            slot.mapping.reset();
            slot.submitted = false;
            slot.submission = 0;
            continue;
        }
        if (slot.submitted && slot.submission <= completed_submission) {
            StartReadback(slot);
        }
    }
    return newest;
}

} // namespace woki::gfx::detail
