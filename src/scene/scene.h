#pragma once

#include "camera.h"

namespace Vol::Scene
{
class Scene {
  public:
    inline Camera &get_camera() { return camera; }

  private:
    Camera camera;
};
}  // namespace Vol::Scene