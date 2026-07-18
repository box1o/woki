#include "include/object.wgsl"
#include "include/lighting.wgsl"
#include "include/pbr_brdf.wgsl"

struct PbrMaterial {
    base_color: vec4<f32>,
    emissive: vec3<f32>,
    metallic: f32,
    roughness: f32,
};

@group(1) @binding(0) var<uniform> material: PbrMaterial;

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
        radiance += (diffuse + specular) * energy * n_dot_l;
    }
    return vec4<f32>(radiance, material.base_color.a);
}
