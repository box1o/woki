#include "include/object.wgsl"
#include "include/lighting.wgsl"
#include "include/pbr_brdf.wgsl"

struct PbrMaterial {
    base_color: vec4<f32>,
    emissive: vec3<f32>,
    metallic: f32,
    roughness: f32,
};

struct ShadowData {
    light_view_projection: mat4x4<f32>,
    depth_bias: f32,
    normal_bias: f32,
    strength: f32,
    light_index: u32,
};

@group(1) @binding(0) var<uniform> material: PbrMaterial;
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

@vertex
fn vertex_main(input: MeshVertex) -> SurfaceVertex {
    return transform_vertex(input);
}

@fragment
fn fragment_main(input: SurfaceVertex) -> @location(0) vec4<f32> {
    let normal = normalize(input.world_normal);
    let view = normalize(object.view_position.xyz - input.world_position);
    let roughness = clamp(material.roughness, 0.045, 1.0);
    let metallic = clamp(material.metallic, 0.0, 1.0);
    let base = max(material.base_color.rgb, vec3<f32>(0.0));
    let reflectance = mix(vec3<f32>(0.04), base, metallic);
    var radiance = lighting.ambient.rgb * base + material.emissive;
    let count = min(lighting.light_count, MAX_LIGHTS);
    for (var index = 0u; index < count; index++) {
        let light = lighting.lights[index];
        let direction = light_direction(light, input.world_position);
        let halfway = normalize(view + direction);
        let n_dot_l = max(dot(normal, direction), 0.0);
        let n_dot_v = max(dot(normal, view), 0.0);
        let distribution = distribution_ggx(normal, halfway, roughness);
        let geometry = geometry_schlick_ggx(n_dot_l, roughness) *
            geometry_schlick_ggx(n_dot_v, roughness);
        let fresnel = fresnel_schlick(max(dot(halfway, view), 0.0), reflectance);
        let specular = distribution * geometry * fresnel / max(4.0 * n_dot_v * n_dot_l, 0.0001);
        let diffuse = (vec3<f32>(1.0) - fresnel) * (1.0 - metallic) * base / PI;
        let energy = light.color_intensity.rgb * light.color_intensity.w *
            light_attenuation(light, input.world_position);
        let casts_shadow = index == shadow.light_index && light.spot_shadow.z > 0.5;
        let light_visibility = select(1.0,
            shadow_visibility(input.world_position, normal, direction), casts_shadow);
        radiance += (diffuse + specular) * energy * n_dot_l * light_visibility;
    }
    return vec4<f32>(radiance, material.base_color.a);
}
