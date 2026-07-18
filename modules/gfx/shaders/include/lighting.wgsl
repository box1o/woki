const MAX_LIGHTS: u32 = 256u;

struct GpuLight {
    position_range: vec4<f32>,
    direction_type: vec4<f32>,
    color_intensity: vec4<f32>,
    spot_shadow: vec4<f32>,
};

struct LightingData {
    ambient: vec4<f32>,
    light_count: u32,
    padding: vec3<u32>,
    lights: array<GpuLight>,
};

@group(2) @binding(0) var<storage, read> lighting: LightingData;

fn light_direction(light: GpuLight, world_position: vec3<f32>) -> vec3<f32> {
    if (light.direction_type.w < 0.5) {
        return normalize(-light.direction_type.xyz);
    }
    return normalize(light.position_range.xyz - world_position);
}

fn light_attenuation(light: GpuLight, world_position: vec3<f32>) -> f32 {
    if (light.direction_type.w < 0.5) {
        return 1.0;
    }
    let distance = length(light.position_range.xyz - world_position);
    let normalized_distance = clamp(distance / max(light.position_range.w, 0.0001), 0.0, 1.0);
    let range_falloff = 1.0 - normalized_distance * normalized_distance;
    var attenuation = range_falloff * range_falloff;
    if (light.direction_type.w > 1.5) {
        let cosine = dot(normalize(light.direction_type.xyz), -light_direction(light, world_position));
        let inner = cos(light.spot_shadow.x);
        let outer = cos(light.spot_shadow.y);
        attenuation *= smoothstep(outer, inner, cosine);
    }
    return attenuation;
}
