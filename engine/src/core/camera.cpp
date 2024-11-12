#include "camera.hpp"
#include <glm/ext/matrix_float4x4.hpp>

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
} // namespace engine
