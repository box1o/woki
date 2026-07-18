struct ShadowData {
    light_view_projection: mat4x4<f32>,
    depth_bias: f32,
    normal_bias: f32,
    strength: f32,
    light_index: u32,
};

@group(2) @binding(1) var<uniform> shadow: ShadowData;
@group(2) @binding(2) var shadow_map: texture_depth_2d;
@group(2) @binding(3) var shadow_sampler: sampler_comparison;

fn shadow_visibility(
    world_position: vec3<f32>, normal: vec3<f32>, direction: vec3<f32>) -> f32 {
    let clip = shadow.light_view_projection * vec4<f32>(world_position, 1.0);
    if (clip.w <= 0.0) {
        return 1.0;
    }
    let projected = clip.xyz / clip.w;
    let uv = projected.xy * vec2<f32>(0.5, -0.5) + vec2<f32>(0.5);
    if (any(uv < vec2<f32>(0.0)) || any(uv > vec2<f32>(1.0)) ||
        projected.z < 0.0 || projected.z > 1.0) {
        return 1.0;
    }
    let facing = max(dot(normal, direction), 0.0);
    let bias = shadow.depth_bias + shadow.normal_bias * (1.0 - facing);
    let sampled = textureSampleCompareLevel(shadow_map, shadow_sampler, uv, projected.z - bias);
    return mix(1.0, sampled, shadow.strength);
}
