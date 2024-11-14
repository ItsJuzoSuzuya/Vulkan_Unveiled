#include "camera.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

namespace engine {
void Camera::setPerspectiveProjection(float fov, float aspect, float near,
                                      float far) {
  float tanHalfFov = tan(fov / 2.f);
  projectionMatrix = glm::mat4(0.f);
  projectionMatrix[0][0] = 1.f / (aspect * tanHalfFov);
  projectionMatrix[1][1] = 1.f / tanHalfFov;
  projectionMatrix[2][2] = far / (far - near);
  projectionMatrix[2][3] = 1.f;
  projectionMatrix[3][2] = -(far * near) / (far - near);
}

void Camera::setView(glm::vec3 position, glm::vec3 rotation) {
  const float c1 = glm::cos(rotation.y);
  const float s1 = glm::sin(rotation.y);
  const float c2 = glm::cos(rotation.x);
  const float s2 = glm::sin(rotation.x);
  const float c3 = glm::cos(rotation.z);
  const float s3 = glm::sin(rotation.z);
  const glm::vec3 u{c1 * c3 + s1 * s2 * s3, c2 * s3, c1 * s2 * s3 - s1 * c3};
  const glm::vec3 v{s1 * s2 * c3 - s3 * c1, c2 * c3, s1 * s3 + c1 * s2 * c3};
  const glm::vec3 w{s1 * c2, -s2, c1 * c2};

  viewMatrix = glm::mat4{1.f};
  viewMatrix[0][0] = u.x;
  viewMatrix[1][0] = u.y;
  viewMatrix[2][0] = u.z;
  viewMatrix[0][1] = v.x;
  viewMatrix[1][1] = v.y;
  viewMatrix[2][1] = v.z;
  viewMatrix[0][2] = w.x;
  viewMatrix[1][2] = w.y;
  viewMatrix[2][2] = w.z;
  viewMatrix[3][0] = -position.x;
  viewMatrix[3][1] = -position.y;
  viewMatrix[3][2] = -position.z;
};

} // namespace engine
