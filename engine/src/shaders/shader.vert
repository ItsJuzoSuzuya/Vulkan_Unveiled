#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat3 normalMatrix;
} push;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
  vec4 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

void main() {
  vec4 worldPosition = push.modelMatrix * vec4(inPosition, 1.0);
  vec3 normalWorldSpace = normalize(push.normalMatrix * inNormal);
  vec3 directionToLight = ubo.lightPosition - worldPosition.xyz;
  float distanceToLight = length(directionToLight);

  float lightIntensity = max(dot(normalWorldSpace, normalize(directionToLight)), 0);
  lightIntensity = lightIntensity / (1.0 + 0.2 * distanceToLight + 0.2 * distanceToLight * distanceToLight);

  vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w;
  vec3 diffuseLight = lightColor * lightIntensity;

  vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  fragColor = inColor * (ambientLight + diffuseLight);

  gl_Position = ubo.projectionMatrix * worldPosition;
}
