#pragma once

#include "context.hpp"
#include "internal.hpp"
#include "resources.hpp"

#include <woki/rhi/forward.hpp>

#include <concepts>
#include <functional>
#include <string_view>
#include <type_traits>

#include <woki/assert/assert.hpp>
#include <woki/core.hpp>

namespace woki::rhi {

class RenderGraph;

class CopyPassContext;
class RenderPassContext;

class PassBuilder final {
public:
    PassBuilder& Target(Framebuffer framebuffer, FramebufferTargetConfig config = {});
    PassBuilder& Color(u32 slot, Resource resource, ColorAttachmentConfig config = {});
    PassBuilder& Color(u32 slot, PerFrameSlot resource, ColorAttachmentConfig config = {});
    PassBuilder& Depth(Resource resource, DepthAttachmentConfig config = {});
    PassBuilder& Depth(PerFrameSlot resource, DepthAttachmentConfig config = {});
    PassBuilder& Sample(Resource resource, SampleMode mode = SampleMode::ColorTexture);
    PassBuilder& Buffer(Resource resource);
    PassBuilder& Buffer(PerFrameSlot resource);
    PassBuilder& Copy(Resource src, Resource dst);

    template <typename Fn> PassBuilder& Execute(Fn&& callback);

private:
    friend class RenderGraphBuilder;
    PassBuilder(RenderGraphBuilder& owner, u32 pass_index);

    RenderGraphBuilder* owner_{nullptr};
    u32 pass_index_{kInvalidGraphResource};
};

class FramebufferBuilder final {
public:
    FramebufferBuilder& Color(u32 slot, Resource resource);
    FramebufferBuilder& Depth(Resource resource);
    [[nodiscard]] Framebuffer Build();

private:
    friend class RenderGraphBuilder;
    explicit FramebufferBuilder(RenderGraphBuilder& owner, u32 framebuffer_index);

    RenderGraphBuilder* owner_{nullptr};
    u32 framebuffer_index_{kInvalidGraphResource};
};

class RenderGraphBuilder final {
public:
    explicit RenderGraphBuilder(Device& device);

    [[nodiscard]] PerFrameSlot PerFrame();
    [[nodiscard]] PerFrameSlot PerFrameBuffer();
    [[nodiscard]] Resource Transient(TransientDesc desc);
    [[nodiscard]] Resource TransientBuffer(TransientBufferDesc desc);
    [[nodiscard]] Resource Use(Texture& texture);
    [[nodiscard]] Resource Use(Buffer& buffer);

    [[nodiscard]] FramebufferBuilder Framebuffer();
    [[nodiscard]] PassBuilder AddPass(std::string_view debug_name);

    void SetPassData(std::string_view pass_name, void* user_data);

    template <typename T> void SetPassData(const std::string_view pass_name, T& user_data) {
        SetPassData(pass_name, static_cast<void*>(&user_data));
    }

    [[nodiscard]] Result<scope<RenderGraph>> Compile(u32 width, u32 height);

private:
    friend class PassBuilder;
    friend class FramebufferBuilder;
    friend class RenderGraph;

    [[nodiscard]] u32 AllocateResource(render_graph::detail::ResourceRecord record);
    [[nodiscard]] u32 AllocateFramebuffer();
    [[nodiscard]] u32 AllocatePass(std::string_view debug_name);

    Device* device_{nullptr};
    render_graph::detail::GraphBlueprint blueprint_{};
};

template <typename Fn> PassBuilder& PassBuilder::Execute(Fn&& callback) {
    WOKI_ASSERT(owner_ != nullptr);
    render_graph::detail::PassRecord& pass = owner_->blueprint_.passes[pass_index_];

    if constexpr (std::is_invocable_v<Fn, CopyPassContext&> &&
                  !std::is_invocable_v<Fn, RenderPassContext&>) {
        pass.copy_execute = [fn = std::forward<Fn>(callback)](CopyPassContext& ctx) mutable {
            if constexpr (std::same_as<std::invoke_result_t<Fn&, CopyPassContext&>, Result<void>>) {
                return fn(ctx);
            } else {
                fn(ctx);
                return Ok();
            }
        };
    } else if constexpr (std::is_invocable_v<Fn, RenderPassContext&>) {
        pass.render_execute = [fn = std::forward<Fn>(callback)](RenderPassContext& ctx) mutable {
            if constexpr (std::same_as<std::invoke_result_t<Fn&, RenderPassContext&>,
                              Result<void>>) {
                return fn(ctx);
            } else {
                fn(ctx);
                return Ok();
            }
        };
    } else {
        static_assert(
            sizeof(Fn) == 0, "Pass callback must accept RenderPassContext& or CopyPassContext&");
    }
    return *this;
}

} // namespace woki::rhi
