#include "include/lighting.wgsl"
#include "include/pbr_brdf.wgsl"
#include "include/shadow_data.wgsl"
#include "include/environment_data.wgsl"

struct ObjectData {
    model: mat4x4<f32>,
    normal_matrix: mat4x4<f32>,
    view_projection: mat4x4<f32>,
    view_position: vec4<f32>,
};

struct SkinData {
    joints: array<mat4x4<f32>>,
};

struct SkinnedMeshVertex {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) joints: vec4<u32>,
    @location(4) weights: vec4<f32>,
};

struct SurfaceVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
};

@group(0) @binding(0) var<uniform> object: ObjectData;
@group(3) @binding(0) var<storage, read> skin: SkinData;

#include "include/pbr_full_surface.wgsl"

fn skin_matrix(input: SkinnedMeshVertex) -> mat4x4<f32> {
    return skin.joints[input.joints.x] * input.weights.x +
        skin.joints[input.joints.y] * input.weights.y +
        skin.joints[input.joints.z] * input.weights.z +
        skin.joints[input.joints.w] * input.weights.w;
}

@vertex
fn vertex_main(input: SkinnedMeshVertex) -> SurfaceVertex {
    let deformation = skin_matrix(input);
    let world_position = object.model * deformation * vec4<f32>(input.position, 1.0);
    let deformed_normal = deformation * vec4<f32>(input.normal, 0.0);
    var output: SurfaceVertex;
    output.position = object.view_projection * world_position;
    output.world_position = world_position.xyz;
    output.world_normal = normalize((object.normal_matrix * deformed_normal).xyz);
    output.uv = input.uv;
    return output;
}
