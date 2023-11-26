#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

Vol::Scene::Camera::Camera()
    : center(0.0f, 0.0f, 0.0f),
      orientation(
          glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f))),
      radius(3.0f)
{
}

void Vol::Scene::Camera::rotate(const glm::vec2 &delta)
{
    // Scale delta by sensitivity
    glm::vec2 angle = delta * 0.25f;

    // Apply yaw rotation
    glm::quat yaw =
        glm::angleAxis(glm::radians(-angle.x), glm::vec3(0.0f, 0.0f, 1.0f));
    orientation = yaw * orientation;

    // Apply pitch rotation
    glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::quat pitch = glm::angleAxis(glm::radians(angle.y), right);
    orientation = pitch * orientation;
}

void Vol::Scene::Camera::zoom(float delta)
{
    radius = std::clamp(radius - delta, 0.1f, 10.0f);
}

glm::vec3 Vol::Scene::Camera::get_position() const
{
    glm::vec3 forward = orientation * glm::vec3(0.0f, -1.0f, 0.0f);
    return center + radius * -forward;
}

glm::mat4 Vol::Scene::Camera::get_view() const
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -get_position());
    glm::mat4 rotation = glm::transpose(glm::mat4_cast(orientation));

    return rotation * translation;
}