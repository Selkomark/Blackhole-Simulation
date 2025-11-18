#pragma once

#include "../utils/Vector3.hpp"

/**
 * Camera structure for view transformation
 */
struct Camera {
  Vector3 position;
  Vector3 forward;
  Vector3 right;
  Vector3 up;
  double fov; // Field of view in degrees

  Camera(const Vector3 &pos, const Vector3 &lookAt, double fieldOfView);
  
  // Update camera basis vectors to look at a target
  void lookAt(const Vector3 &target);
};

