#include "fullscreen_triangle.wgsl"

@group(0) @binding(0) var hdr_color: texture_2d<f32>;
@group(0) @binding(1) var linear_sampler: sampler;

fn aces_fitted(color: vec3<f32>) -> vec3<f32> {
    let a = 2.51;
    let b = 0.03;
    let c = 2.43;
    let d = 0.59;
    let e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e),
        vec3<f32>(0.0), vec3<f32>(1.0));
}

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> FullscreenVertex {
    return fullscreen_vertex(vertex_index);
}

@fragment
fn fragment_main(input: FullscreenVertex) -> @location(0) vec4<f32> {
    let hdr = max(textureSample(hdr_color, linear_sampler, input.uv).rgb, vec3<f32>(0.0));
    return vec4<f32>(aces_fitted(hdr), 1.0);
}
