#version 460

struct ObjectData {
    mat4 modelMatrix;
    mat4 normalMatrix;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
  vec4 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

layout(set = 0, binding = 1) readonly buffer ObjectBuffer {
  ObjectData data[];
} objectBuffer;

void main() {
  vec4 worldPosition = objectBuffer.data[gl_InstanceIndex].modelMatrix * vec4(inPosition, 1.0);

  fragNormalWorld = normalize(mat3(objectBuffer.data[gl_InstanceIndex].normalMatrix) * inNormal);
  fragPositionWorld = worldPosition.xyz;

  gl_Position = ubo.projectionMatrix * worldPosition;
  fragColor = inColor;
  fragTexCoord = inTexCoord;
}
