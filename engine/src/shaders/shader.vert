#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWorld;
layout(location = 2) out vec3 fragNormalWorld;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 normalMatrix;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
  vec4 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

void main() {
  vec4 worldPosition = push.modelMatrix * vec4(inPosition, 1.0);
  fragNormalWorld = normalize(mat3(push.normalMatrix) * inNormal);
  fragPositionWorld = worldPosition.xyz;

  gl_Position = ubo.projectionMatrix * worldPosition;
  fragColor = inColor;
}
