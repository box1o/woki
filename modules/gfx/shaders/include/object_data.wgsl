struct ObjectData {
    model: mat4x4<f32>,
    normal_matrix: mat4x4<f32>,
    view_projection: mat4x4<f32>,
    view_position: vec4<f32>,
};

@group(0) @binding(0) var<uniform> object: ObjectData;
