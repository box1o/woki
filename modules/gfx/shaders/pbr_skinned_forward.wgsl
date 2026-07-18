#include "include/lighting.wgsl"

struct ObjectData {
    model: mat4x4<f32>,
};

struct SkinData {
    joints: array<mat4x4<f32>>,
};

struct PbrMaterial {
    base_color: vec4<f32>,
    emissive: vec3<f32>,
    metallic: f32,
    roughness: f32,
};

struct SkinnedVertex {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) joints: vec4<u32>,
    @location(3) weights: vec4<f32>,
};

struct SurfaceVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
};

@group(0) @binding(0) var<uniform> object: ObjectData;
@group(1) @binding(0) var<uniform> material: PbrMaterial;
@group(3) @binding(0) var<storage, read> skin: SkinData;

fn skin_matrix(input: SkinnedVertex) -> mat4x4<f32> {
    return skin.joints[input.joints.x] * input.weights.x +
        skin.joints[input.joints.y] * input.weights.y +
        skin.joints[input.joints.z] * input.weights.z +
        skin.joints[input.joints.w] * input.weights.w;
}

@vertex
fn vertex_main(input: SkinnedVertex) -> SurfaceVertex {
    let deformation = skin_matrix(input);
    let world = object.model * deformation;
    let world_position = world * vec4<f32>(input.position, 1.0);
    var output: SurfaceVertex;
    output.position = world_position;
    output.world_position = world_position.xyz;
    output.world_normal = normalize((world * vec4<f32>(input.normal, 0.0)).xyz);
    return output;
}

@fragment
fn fragment_main(input: SurfaceVertex) -> @location(0) vec4<f32> {
    let normal = normalize(input.world_normal);
    let view = normalize(-input.world_position);
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
        let fresnel = reflectance + (vec3<f32>(1.0) - reflectance) *
            pow(1.0 - max(dot(halfway, view), 0.0), 5.0);
        let alpha = roughness * roughness;
        let alpha2 = alpha * alpha;
        let n_dot_h = max(dot(normal, halfway), 0.0);
        let divisor = n_dot_h * n_dot_h * (alpha2 - 1.0) + 1.0;
        let distribution = alpha2 / max(3.14159265359 * divisor * divisor, 0.0001);
        let k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
        let geometry_l = n_dot_l / max(n_dot_l * (1.0 - k) + k, 0.0001);
        let geometry_v = n_dot_v / max(n_dot_v * (1.0 - k) + k, 0.0001);
        let specular = distribution * geometry_l * geometry_v * fresnel /
            max(4.0 * n_dot_v * n_dot_l, 0.0001);
        let diffuse = (vec3<f32>(1.0) - fresnel) * (1.0 - metallic) * base / 3.14159265359;
        let energy = light.color_intensity.rgb * light.color_intensity.w *
            light_attenuation(light, input.world_position);
        radiance += (diffuse + specular) * energy * n_dot_l;
    }
    return vec4<f32>(radiance, material.base_color.a);
}
