struct EnvironmentData {
    parameters: vec4<f32>,
};

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
