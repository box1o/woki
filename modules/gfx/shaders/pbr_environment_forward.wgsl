#include "include/object.wgsl"
#include "include/lighting.wgsl"
#include "include/pbr_brdf.wgsl"

struct PbrMaterial {
    base_color: vec4<f32>,
    emissive: vec3<f32>,
    metallic: f32,
    roughness: f32,
};

struct EnvironmentData {
    parameters: vec4<f32>,
};

@group(1) @binding(0) var<uniform> material: PbrMaterial;
@group(2) @binding(4) var radiance_map: texture_cube<f32>;
@group(2) @binding(5) var irradiance_map: texture_cube<f32>;
@group(2) @binding(6) var brdf_lut: texture_2d<f32>;
@group(2) @binding(7) var environment_sampler: sampler;
@group(2) @binding(8) var<uniform> environment: EnvironmentData;

fn rotate_environment(direction: vec3<f32>) -> vec3<f32> {
    let sine = environment.parameters.z;
    let cosine = environment.parameters.w;
    return vec3<f32>(cosine * direction.x + sine * direction.z, direction.y,
        -sine * direction.x + cosine * direction.z);
}

fn fresnel_schlick_roughness(
    cosine: f32, reflectance: vec3<f32>, roughness: f32) -> vec3<f32> {
    return reflectance + (max(vec3<f32>(1.0 - roughness), reflectance) - reflectance) *
        pow(1.0 - cosine, 5.0);
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
    let n_dot_v = max(dot(normal, view), 0.0);
    let environment_fresnel = fresnel_schlick_roughness(n_dot_v, reflectance, roughness);
    let environment_diffuse = textureSample(irradiance_map, environment_sampler,
        rotate_environment(normal)).rgb * base;
    let reflection = rotate_environment(reflect(-view, normal));
    let environment_specular = textureSampleLevel(radiance_map, environment_sampler, reflection,
        roughness * environment.parameters.y).rgb;
    let brdf = textureSample(brdf_lut, environment_sampler, vec2<f32>(n_dot_v, roughness)).rg;
    let specular_ibl = environment_specular *
        (environment_fresnel * brdf.x + vec3<f32>(brdf.y));
    let diffuse_weight = (vec3<f32>(1.0) - environment_fresnel) * (1.0 - metallic);
    var radiance = (diffuse_weight * environment_diffuse + specular_ibl) *
        environment.parameters.x + lighting.ambient.rgb * base + material.emissive;
    let count = min(lighting.light_count, MAX_LIGHTS);
    for (var index = 0u; index < count; index++) {
        let light = lighting.lights[index];
        let direction = light_direction(light, input.world_position);
        let halfway = normalize(view + direction);
        let n_dot_l = max(dot(normal, direction), 0.0);
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
    return vec4<f32>(radiance, material.base_color.a);
}
