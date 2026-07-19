struct Particle {
  position: vec3f,
  // NOTE: vec3 has 16-byte alignment in uniform buffers — pad
  // explicitly or interleave with a scalar to avoid layout surprises
  life: f32,
  velocity: vec3f,
  mass: f32,
};

struct Uniforms {
  viewProjection: mat4x4f,
  time: f32,
  deltaTime: f32,
};

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) normal: vec3f,
};
