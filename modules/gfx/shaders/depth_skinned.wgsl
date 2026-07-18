#include "include/object_data.wgsl"
#include "include/skinning.wgsl"

struct SkinnedDepthVertex {
    @location(0) position: vec3<f32>,
    @location(3) joints: vec4<u32>,
    @location(4) weights: vec4<f32>,
};

@vertex
fn vertex_main(input: SkinnedDepthVertex) -> @builtin(position) vec4<f32> {
    let deformation = skin_matrix(input.joints, input.weights);
    return object.view_projection * object.model * deformation *
        vec4<f32>(input.position, 1.0);
}
