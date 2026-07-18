#pragma once

#include <woki/gfx/graph/rhi_render_graph.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <optional>

namespace woki::gfx::detail {

struct GpuFrameTimingSample final {
    u64 submission{0};
    u64 duration_ns{0};
};

[[nodiscard]] Result<u64> ResolveGpuDuration(u64 beginning, u64 end);

class GpuFrameProfiler final {
public:
    explicit GpuFrameProfiler(rhi::Device& device);
    ~GpuFrameProfiler();

    GpuFrameProfiler(const GpuFrameProfiler&) = delete;
    GpuFrameProfiler& operator=(const GpuFrameProfiler&) = delete;
    GpuFrameProfiler(GpuFrameProfiler&&) = delete;
    GpuFrameProfiler& operator=(GpuFrameProfiler&&) = delete;

    [[nodiscard]] bool Supported() const noexcept;
    [[nodiscard]] bool Active() const noexcept;
    [[nodiscard]] const std::optional<Error>& InitializationError() const noexcept;

    [[nodiscard]] Result<bool> Capture(RhiRenderGraphFrame& frame);
    void MarkSubmitted(u64 submission) noexcept;
    void CancelCapture() noexcept;
    [[nodiscard]] std::optional<GpuFrameTimingSample> Poll(u64 completed_submission);

private:
    struct MapState final {
        std::atomic<i32> status{0};
    };

    struct Slot final {
        scope<rhi::Buffer> resolve{};
        scope<rhi::Buffer> readback{};
        std::shared_ptr<MapState> mapping{};
        u64 submission{0};
        bool submitted{false};
    };

    void Initialize();
    void StartReadback(Slot& slot);

    static constexpr std::size_t kBufferedFrames = 3;
    static constexpr u64 kTimestampBytes = sizeof(u64) * 2;

    rhi::Device* device_{nullptr};
    scope<rhi::QuerySet> query_set_{};
    std::array<Slot, kBufferedFrames> slots_{};
    std::optional<std::size_t> active_slot_{};
    std::optional<Error> initialization_error_{};
    bool supported_{false};
};

} // namespace woki::gfx::detail
