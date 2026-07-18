struct ShadowData {
    light_view_projections: array<mat4x4<f32>, 4>,
    atlas_transforms: array<vec4<f32>, 4>,
    split_distances: vec4<f32>,
    parameters: vec4<f32>,
    light_index: u32,
    cascade_count: u32,
    padding: vec2<u32>,
};

@group(2) @binding(1) var<uniform> shadow: ShadowData;
@group(2) @binding(2) var shadow_map: texture_depth_2d;
@group(2) @binding(3) var shadow_sampler: sampler_comparison;

fn shadow_visibility(
    world_position: vec3<f32>, normal: vec3<f32>, direction: vec3<f32>) -> f32 {
    let cascade_count = clamp(shadow.cascade_count, 1u, 4u);
    let view_distance = distance(object.view_position.xyz, world_position);
    var cascade = cascade_count - 1u;
    for (var index = 0u; index < cascade_count; index++) {
        if (view_distance <= shadow.split_distances[index]) {
            cascade = index;
            break;
        }
    }
    let clip = shadow.light_view_projections[cascade] * vec4<f32>(world_position, 1.0);
    if (clip.w <= 0.0) {
        return 1.0;
    }
    let projected = clip.xyz / clip.w;
    let cascade_uv = projected.xy * vec2<f32>(0.5, -0.5) + vec2<f32>(0.5);
    if (any(cascade_uv < vec2<f32>(0.0)) || any(cascade_uv > vec2<f32>(1.0)) ||
        projected.z < 0.0 || projected.z > 1.0) {
        return 1.0;
    }
    let atlas = shadow.atlas_transforms[cascade];
    let texel = vec2<f32>(1.0) / vec2<f32>(textureDimensions(shadow_map));
    let uv = clamp(cascade_uv * atlas.xy + atlas.zw,
        atlas.zw + texel * 0.5, atlas.zw + atlas.xy - texel * 0.5);
    let facing = max(dot(normal, direction), 0.0);
    let bias = shadow.parameters.x + shadow.parameters.y * (1.0 - facing);
    let sampled = textureSampleCompareLevel(shadow_map, shadow_sampler, uv, projected.z - bias);
    return mix(1.0, sampled, shadow.parameters.z);
}
