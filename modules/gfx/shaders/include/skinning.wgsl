struct SkinData {
    joints: array<mat4x4<f32>>,
};

@group(3) @binding(0) var<storage, read> skin: SkinData;

fn skin_matrix(joints: vec4<u32>, weights: vec4<f32>) -> mat4x4<f32> {
    return skin.joints[joints.x] * weights.x +
        skin.joints[joints.y] * weights.y +
        skin.joints[joints.z] * weights.z +
        skin.joints[joints.w] * weights.w;
}
