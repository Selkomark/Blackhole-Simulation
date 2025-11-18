#include "../../include/camera/Camera.hpp"

Camera::Camera(const Vector3 &pos, const Vector3 &lookAt, double fieldOfView)
    : position(pos), fov(fieldOfView) {
  this->lookAt(lookAt);
}

void Camera::lookAt(const Vector3 &target) {
  forward = (target - position).normalized();
  right = forward.cross(Vector3(0, 1, 0)).normalized();
  up = right.cross(forward).normalized();
}

