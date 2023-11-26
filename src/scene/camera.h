#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Vol::Scene
{
class Camera {
  public:
    explicit Camera();

    void rotate(const glm::vec2 &delta);
    void zoom(float delta);

    glm::vec3 get_position() const;
    glm::mat4 get_view() const;

  private:
    glm::vec3 center;
    glm::quat orientation;
    float radius;
};
}  // namespace Vol::Scene