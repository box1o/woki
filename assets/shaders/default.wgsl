#include "include/common/particle.wgsl"
#include "include/common/common.wgsl"


@vertex
fn vertex_main(input: VertexInput) -> VertexOutput {
    return make_vertex_output(input);
}

@fragment
fn fragment_main(input: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(input.color, 1.0);
}
