#pragma once

#include "frame_upload_arena.hpp"
#include "gpu_resource_manager.hpp"

#include <memory>
#include <span>
#include <string>

namespace woki::gfx {

struct FrameUniformBufferDesc final {
    u64 capacity_per_frame{0};
    u32 frames_in_flight{0};
    u64 alignment{256};
    std::string label{"Frame uniforms"};
};

struct UniformBufferSlice final {
    BufferHandle buffer{};
    u64 offset{0};
    u64 size{0};
};

class FrameUniformBuffer final {
public:
    [[nodiscard]] static Result<std::unique_ptr<FrameUniformBuffer>> Create(
        GpuResourceManager& resources, rhi::Queue& queue, const FrameUniformBufferDesc& desc);
    ~FrameUniformBuffer();

    FrameUniformBuffer(const FrameUniformBuffer&) = delete;
    FrameUniformBuffer& operator=(const FrameUniformBuffer&) = delete;
    FrameUniformBuffer(FrameUniformBuffer&&) = delete;
    FrameUniformBuffer& operator=(FrameUniformBuffer&&) = delete;

    [[nodiscard]] Result<void> BeginFrame(u64 frame_number, u64 completed_submission);
    [[nodiscard]] Result<UniformBufferSlice> Write(
        std::span<const std::byte> data, u64 alignment = 0);
    [[nodiscard]] Result<void> Flush();
    [[nodiscard]] Result<void> MarkSubmitted(u64 submission);
    void AbortFrame() noexcept;

    [[nodiscard]] BufferHandle Buffer() const noexcept;
    [[nodiscard]] u64 Used() const noexcept;
    [[nodiscard]] bool FrameActive() const noexcept;

private:
    FrameUniformBuffer(GpuResourceManager& resources, rhi::Queue& queue, FrameUploadArena arena,
        BufferHandle buffer);

    GpuResourceManager* resources_{nullptr};
    rhi::Queue* queue_{nullptr};
    FrameUploadArena arena_;
    BufferHandle buffer_{};
    bool flushed_{false};
};

} // namespace woki::gfx
