#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
namespace engine {
class Camera {
public:
  void setOrthographicProjection(float left, float right, float top,
                                 float bottom, float near, float far);
  void setPerspectiveProjection(float fov, float aspect, float near, float far);

  const glm::mat4 &getProjection() const { return projectionMatrix; }

private:
  glm::mat4 projectionMatrix{1.f};
};
} // namespace engine
