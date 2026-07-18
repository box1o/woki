#include <woki/gfx/resource/frame_uniform_buffer.hpp>

namespace woki::gfx {

Result<std::unique_ptr<FrameUniformBuffer>> FrameUniformBuffer::Create(
    GpuResourceManager& resources, rhi::Queue& queue, const FrameUniformBufferDesc& desc) {
    auto arena = FrameUploadArena::Create({
        .capacity_per_frame = desc.capacity_per_frame,
        .frames_in_flight = desc.frames_in_flight,
        .default_alignment = desc.alignment,
    });
    if (!arena) {
        return Err(arena.error());
    }

    BufferResourceDesc buffer_desc{};
    buffer_desc.gpu.size = arena->Capacity();
    buffer_desc.gpu.usage = rhi::BufferUsage::Uniform | rhi::BufferUsage::CopyDst;
    buffer_desc.gpu.label = desc.label;
    buffer_desc.lifetime = ResourceLifetime::Dynamic;
    auto buffer = resources.CreateBuffer(buffer_desc);
    if (!buffer) {
        return Err(buffer.error());
    }
    return Ok(std::unique_ptr<FrameUniformBuffer>(
        new FrameUniformBuffer(resources, queue, std::move(*arena), *buffer)));
}

FrameUniformBuffer::FrameUniformBuffer(GpuResourceManager& resources, rhi::Queue& queue,
    FrameUploadArena arena, const BufferHandle buffer)
    : resources_(&resources), queue_(&queue), arena_(std::move(arena)), buffer_(buffer) {}

FrameUniformBuffer::~FrameUniformBuffer() {
    if (resources_ != nullptr && buffer_) {
        static_cast<void>(resources_->Destroy(buffer_));
    }
}

Result<void> FrameUniformBuffer::BeginFrame(
    const u64 frame_number, const u64 completed_submission) {
    if (auto result = arena_.BeginFrame(frame_number, completed_submission); !result) {
        return result;
    }
    flushed_ = false;
    return Ok();
}

Result<UniformBufferSlice> FrameUniformBuffer::Write(
    const std::span<const std::byte> data, const u64 alignment) {
    if (flushed_) {
        return Err(
            ErrorCode::InvalidState, "Frame uniform buffer cannot be written after it is flushed");
    }
    auto allocation = arena_.Write(data, alignment);
    if (!allocation) {
        return Err(allocation.error());
    }
    return Ok(UniformBufferSlice{
        .buffer = buffer_,
        .offset = allocation->offset,
        .size = allocation->size,
    });
}

Result<void> FrameUniformBuffer::Flush() {
    if (!arena_.FrameActive()) {
        return Err(ErrorCode::InvalidState, "Frame uniform buffer has no active frame");
    }
    if (flushed_) {
        return Ok();
    }
    const auto data = arena_.PendingData();
    if (!data.empty()) {
        const rhi::Buffer* buffer = resources_->Resolve(buffer_);
        if (buffer == nullptr) {
            return Err(
                ErrorCode::FailedToAcquireResource, "Frame uniform GPU buffer is no longer active");
        }
        if (auto result =
                queue_->WriteBuffer(*buffer, arena_.PendingOffset(), data.data(), data.size());
            !result) {
            return Err(result.error());
        }
    }
    flushed_ = true;
    return Ok();
}

Result<void> FrameUniformBuffer::MarkSubmitted(const u64 submission) {
    if (!flushed_) {
        return Err(
            ErrorCode::InvalidState, "Frame uniform buffer must be flushed before submission");
    }
    return arena_.MarkSubmitted(submission);
}

BufferHandle FrameUniformBuffer::Buffer() const noexcept { return buffer_; }
u64 FrameUniformBuffer::Used() const noexcept { return arena_.Used(); }
bool FrameUniformBuffer::FrameActive() const noexcept { return arena_.FrameActive(); }

} // namespace woki::gfx
