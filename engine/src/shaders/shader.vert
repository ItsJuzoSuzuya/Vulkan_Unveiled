#version 460

layout(location = 0) in vec3 position;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
} ubo;

void main() {
  gl_Position = ubo.projectionMatrix * push.modelMatrix * vec4(position, 1.0);
}
