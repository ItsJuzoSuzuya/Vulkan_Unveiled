#include "game_object.hpp"
namespace engine {

glm::mat4 Transform::mat4() {
  const float c1 = glm::cos(position.y);
  const float s1 = glm::sin(position.y);
  const float c2 = glm::cos(position.x);
  const float s2 = glm::sin(position.x);
  const float c3 = glm::cos(position.z);
  const float s3 = glm::sin(position.z);
  return glm::mat4{{
                       scale.x * (c1 * c3 + s1 * s2 * s3),
                       scale.x * c2 * s3,
                       scale.x * (c1 * s2 * s3 - s1 * c3),
                       0.f,
                   },
                   {
                       scale.y * (s1 * s2 * c3 - s3 * c1),
                       scale.y * c2 * c3,
                       scale.y * (s1 * s3 + c1 * s2 * c3),
                       0.f,
                   },
                   {
                       scale.z * s1 * c2,
                       scale.z * -s2,
                       scale.z * c1 * c2,
                       0.f,
                   },
                   {position.x, position.y, position.z, 1.f}};
}

} // namespace engine
