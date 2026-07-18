struct ObjectData {
    model: mat4x4<f32>,
    normal_matrix: mat4x4<f32>,
    view_projection: mat4x4<f32>,
    view_position: vec4<f32>,
};

@group(0) @binding(0) var<uniform> object: ObjectData;

struct MeshVertex {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
};

struct SurfaceVertex {
    @builtin(position) position: vec4<f32>,
    @location(0) world_position: vec3<f32>,
    @location(1) world_normal: vec3<f32>,
};

fn transform_vertex(input: MeshVertex) -> SurfaceVertex {
    let world_position = object.model * vec4<f32>(input.position, 1.0);
    var output: SurfaceVertex;
    output.position = object.view_projection * world_position;
    output.world_position = world_position.xyz;
    output.world_normal = normalize((object.normal_matrix * vec4<f32>(input.normal, 0.0)).xyz);
    return output;
}
