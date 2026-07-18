#pragma once

#include "../resource/resource_types.hpp"

#include <limits>
#include <string>
#include <variant>
#include <vector>

namespace woki::gfx {

template <typename Tag> class GraphHandle final {
public:
    static constexpr u32 kInvalidIndex = std::numeric_limits<u32>::max();

    constexpr GraphHandle() noexcept = default;
    [[nodiscard]] static constexpr GraphHandle FromIndex(const u32 index) noexcept {
        return GraphHandle(index);
    }
    [[nodiscard]] constexpr u32 Index() const noexcept { return index_; }
    [[nodiscard]] constexpr bool Valid() const noexcept { return index_ != kInvalidIndex; }
    [[nodiscard]] explicit constexpr operator bool() const noexcept { return Valid(); }
    [[nodiscard]] friend constexpr bool operator==(GraphHandle, GraphHandle) noexcept = default;

private:
    explicit constexpr GraphHandle(const u32 index) noexcept : index_(index) {}
    u32 index_{kInvalidIndex};
};

struct GraphResourceTag;
struct GraphPassTag;
using GraphResource = GraphHandle<GraphResourceTag>;
using GraphPass = GraphHandle<GraphPassTag>;

enum class GraphResourceKind : u8 {
    Buffer = 0,
    Texture,
};

enum class GraphAccess : u8 {
    Read = 0,
    Write,
    ReadWrite,
};

using ImportedGraphResource = std::variant<std::monostate, BufferHandle, TextureHandle>;

struct GraphResourceDesc final {
    std::string label{};
    GraphResourceKind kind{GraphResourceKind::Texture};
    ImportedGraphResource imported{};
};

struct GraphResourceUse final {
    GraphResource resource{};
    GraphAccess access{GraphAccess::Read};
};

struct GraphPassDesc final {
    std::string label{};
    std::vector<GraphResourceUse> resources{};
    std::vector<GraphPass> depends_on{};
};

struct GraphResourceLifetime final {
    GraphResource resource{};
    u32 first_pass{0};
    u32 last_pass{0};
};

struct CompiledGraphPass final {
    GraphPass pass{};
    std::vector<GraphPass> dependencies{};
};

struct CompiledRenderGraph final {
    std::vector<CompiledGraphPass> passes{};
    std::vector<GraphResourceLifetime> lifetimes{};
};

class RenderGraph final {
public:
    [[nodiscard]] Result<GraphResource> AddResource(const GraphResourceDesc& desc);
    [[nodiscard]] Result<GraphResource> Import(BufferHandle buffer, std::string label = {});
    [[nodiscard]] Result<GraphResource> Import(TextureHandle texture, std::string label = {});
    [[nodiscard]] Result<GraphPass> AddPass(const GraphPassDesc& desc);

    [[nodiscard]] const GraphResourceDesc* Resource(GraphResource resource) const noexcept;
    [[nodiscard]] const GraphPassDesc* Pass(GraphPass pass) const noexcept;
    [[nodiscard]] Result<CompiledRenderGraph> Compile() const;

    [[nodiscard]] std::size_t ResourceCount() const noexcept;
    [[nodiscard]] std::size_t PassCount() const noexcept;
    void Clear() noexcept;

private:
    std::vector<GraphResourceDesc> resources_{};
    std::vector<GraphPassDesc> passes_{};
};

} // namespace woki::gfx
