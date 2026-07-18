#include "include/lighting.wgsl"
#include "include/pbr_brdf.wgsl"

struct ObjectData {
    model: mat4x4<f32>,
    normal_matrix: mat4x4<f32>,
    view_projection: mat4x4<f32>,
    view_position: vec4<f32>,
};

struct PbrMaterial {
    base_color: vec4<f32>,
    emissive: vec3<f32>,
    metallic: f32,
    roughness: f32,
    normal_scale: f32,
    occlusion_strength: f32,
    alpha_cutoff: f32,
};

struct MeshVertex {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

struct SurfaceVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

@group(0) @binding(0) var<uniform> object: ObjectData;
@group(1) @binding(0) var<uniform> material: PbrMaterial;
@group(1) @binding(1) var base_color_map: texture_2d<f32>;
@group(1) @binding(2) var metallic_roughness_map: texture_2d<f32>;
@group(1) @binding(3) var normal_map: texture_2d<f32>;
@group(1) @binding(4) var occlusion_map: texture_2d<f32>;
@group(1) @binding(5) var emissive_map: texture_2d<f32>;
@group(1) @binding(6) var material_sampler: sampler;

fn mapped_normal(input: SurfaceVertex) -> vec3<f32> {
    let sampled = textureSample(normal_map, material_sampler, input.uv).xyz * 2.0 - 1.0;
    let tangent_normal = normalize(vec3<f32>(sampled.xy * material.normal_scale, sampled.z));
    let position_dx = dpdx(input.world_position);
    let position_dy = dpdy(input.world_position);
    let uv_dx = dpdx(input.uv);
    let uv_dy = dpdy(input.uv);
    let determinant = uv_dx.x * uv_dy.y - uv_dx.y * uv_dy.x;
    if (abs(determinant) < 0.000001) {
        return normalize(input.world_normal);
    }
    let tangent = normalize((position_dx * uv_dy.y - position_dy * uv_dx.y) / determinant);
    let normal = normalize(input.world_normal);
    let bitangent = normalize(cross(normal, tangent));
    return normalize(mat3x3<f32>(tangent, bitangent, normal) * tangent_normal);
}

@vertex
fn vertex_main(input: MeshVertex) -> SurfaceVertex {
    let world_position = object.model * vec4<f32>(input.position, 1.0);
    var output: SurfaceVertex;
    output.position = object.view_projection * world_position;
    output.world_position = world_position.xyz;
    output.world_normal = normalize((object.normal_matrix * vec4<f32>(input.normal, 0.0)).xyz);
    output.uv = input.uv;
    return output;
}

@fragment
fn fragment_main(input: SurfaceVertex) -> @location(0) vec4<f32> {
    let color_sample = textureSample(base_color_map, material_sampler, input.uv);
    let base_color = material.base_color * color_sample;
    if (base_color.a < material.alpha_cutoff) {
        discard;
    }
    let properties = textureSample(metallic_roughness_map, material_sampler, input.uv);
    let metallic = clamp(material.metallic * properties.b, 0.0, 1.0);
    let roughness = clamp(material.roughness * properties.g, 0.045, 1.0);
    let occlusion_sample = textureSample(occlusion_map, material_sampler, input.uv).r;
    let occlusion = mix(1.0, occlusion_sample, material.occlusion_strength);
    let emissive = material.emissive *
        textureSample(emissive_map, material_sampler, input.uv).rgb;
    let normal = mapped_normal(input);
    let view = normalize(object.view_position.xyz - input.world_position);
    let base = max(base_color.rgb, vec3<f32>(0.0));
    let reflectance = mix(vec3<f32>(0.04), base, metallic);
    var radiance = lighting.ambient.rgb * base * occlusion + emissive;
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
        radiance += (diffuse + specular) * energy * n_dot_l;
    }
    return vec4<f32>(radiance, base_color.a);
}
