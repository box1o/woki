struct PbrMaterial {
    base_color: vec4<f32>,
    emissive: vec3<f32>,
    metallic: f32,
    roughness: f32,
    normal_scale: f32,
    occlusion_strength: f32,
    alpha_cutoff: f32,
};

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

@fragment
fn fragment_main(input: SurfaceVertex) -> @location(0) vec4<f32> {
    let base_color = material.base_color *
        textureSample(base_color_map, material_sampler, input.uv);
    if (base_color.a < material.alpha_cutoff) {
        discard;
    }
    let properties = textureSample(metallic_roughness_map, material_sampler, input.uv);
    let metallic = clamp(material.metallic * properties.b, 0.0, 1.0);
    let roughness = clamp(material.roughness * properties.g, 0.045, 1.0);
    let sampled_occlusion = textureSample(occlusion_map, material_sampler, input.uv).r;
    let occlusion = mix(1.0, sampled_occlusion, material.occlusion_strength);
    let emissive = material.emissive *
        textureSample(emissive_map, material_sampler, input.uv).rgb;
    let normal = mapped_normal(input);
    let view = normalize(object.view_position.xyz - input.world_position);
    let base = max(base_color.rgb, vec3<f32>(0.0));
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
    let image_lighting = (diffuse_weight * environment_diffuse + specular_ibl) *
        environment.parameters.x * occlusion;
    var radiance = image_lighting + lighting.ambient.rgb * base * occlusion + emissive;

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
        let specular = distribution * geometry * fresnel /
            max(4.0 * n_dot_v * n_dot_l, 0.0001);
        let diffuse = (vec3<f32>(1.0) - fresnel) * (1.0 - metallic) * base / PI;
        let energy = light.color_intensity.rgb * light.color_intensity.w *
            light_attenuation(light, input.world_position);
        let casts_shadow = index == shadow.light_index && light.spot_shadow.z > 0.5;
        let visibility = select(1.0,
            shadow_visibility(input.world_position, normal, direction), casts_shadow);
        radiance += (diffuse + specular) * energy * n_dot_l * visibility;
    }
    return vec4<f32>(radiance, base_color.a);
}
