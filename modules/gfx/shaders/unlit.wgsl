#include "include/object.wgsl"

struct UnlitMaterial {
    color: vec4<f32>,
};

@group(1) @binding(0) var<uniform> material: UnlitMaterial;

@vertex
fn vertex_main(input: MeshVertex) -> SurfaceVertex {
    return transform_vertex(input);
}

@fragment
fn fragment_main(input: SurfaceVertex) -> @location(0) vec4<f32> {
    return material.color;
}
