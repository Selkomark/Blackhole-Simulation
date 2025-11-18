#include "../../include/camera/CinematicCamera.hpp"
#include <SDL2/SDL.h>
#include <cmath>

CinematicCamera::CinematicCamera(Camera &camera, const Vector3 &initialPosition)
    : cam(camera), initialPos(initialPosition), mode(CinematicMode::SmoothOrbit),
      orbitAngle(0.0), orbitRadius(15.0), cinematicTime(0.0) {}

void CinematicCamera::update(double deltaTime, const uint8_t *keyStates) {
  cinematicTime += deltaTime;
  
  switch (mode) {
    case CinematicMode::Manual:
      updateManualMode(deltaTime, keyStates);
      break;
    case CinematicMode::SmoothOrbit:
      updateSmoothOrbit(deltaTime);
      break;
    case CinematicMode::WaveMotion:
      updateWaveMotion(deltaTime);
      break;
    case CinematicMode::RisingSpiral:
      updateRisingSpiral(deltaTime);
      break;
    case CinematicMode::CloseFlyby:
      updateCloseFlyby(deltaTime);
      break;
  }
  
  // Always look at the black hole center
  cam.lookAt(Vector3(0, 0, 0));
}

void CinematicCamera::updateManualMode(double deltaTime, const uint8_t *keyStates) {
  // Gentle automatic orbit
  orbitAngle += 0.15 * deltaTime;
  orbitRadius = 15.0;
  double baseX = std::cos(orbitAngle) * orbitRadius;
  double baseZ = std::sin(orbitAngle) * orbitRadius;
  double baseY = 3.0 + std::sin(orbitAngle * 0.5) * 1.5;
  
  // User can offset from base position with manual controls
  Vector3 basePos(baseX, baseY, baseZ);
  Vector3 manualOffset(0, 0, 0);
  
  double moveSpeed = 5.0 * deltaTime;
  
  if (keyStates[SDL_SCANCODE_W]) {
    manualOffset += cam.forward * moveSpeed;
  }
  if (keyStates[SDL_SCANCODE_S]) {
    manualOffset -= cam.forward * moveSpeed;
  }
  if (keyStates[SDL_SCANCODE_A]) {
    manualOffset -= cam.right * moveSpeed;
  }
  if (keyStates[SDL_SCANCODE_D]) {
    manualOffset += cam.right * moveSpeed;
  }
  if (keyStates[SDL_SCANCODE_SPACE]) {
    manualOffset.y += moveSpeed;
  }
  if (keyStates[SDL_SCANCODE_LSHIFT]) {
    manualOffset.y -= moveSpeed;
  }
  
  // Blend between auto position and manual control
  cam.position = basePos + manualOffset;
}

void CinematicCamera::updateSmoothOrbit(double deltaTime) {
  orbitAngle += 0.25 * deltaTime;
  orbitRadius = 15.0;
  cam.position.x = std::cos(orbitAngle) * orbitRadius;
  cam.position.z = std::sin(orbitAngle) * orbitRadius;
  cam.position.y = 3.0 + std::sin(orbitAngle * 0.5) * 1.5;
}

void CinematicCamera::updateWaveMotion(double deltaTime) {
  orbitAngle += 0.3 * deltaTime;
  cam.position.x = std::cos(orbitAngle) * 12.0;
  cam.position.z = std::sin(orbitAngle * 2.0) * 8.0; // Figure-8 motion
  cam.position.y = 2.0 + std::sin(orbitAngle * 1.5) * 3.0;
}

void CinematicCamera::updateRisingSpiral(double deltaTime) {
  orbitAngle += 0.35 * deltaTime;
  orbitRadius = 10.0 + std::sin(cinematicTime * 0.3) * 3.0;
  cam.position.x = std::cos(orbitAngle) * orbitRadius;
  cam.position.z = std::sin(orbitAngle) * orbitRadius;
  cam.position.y = 1.0 + cinematicTime * 0.4;
  
  // Reset height periodically
  if (cam.position.y > 8.0) {
    cam.position.y = 1.0;
    cinematicTime = 0.0;
  }
}

void CinematicCamera::updateCloseFlyby(double deltaTime) {
  orbitAngle += 0.5 * deltaTime; // Faster rotation
  orbitRadius = 6.0 + std::sin(orbitAngle * 0.7) * 2.0;
  cam.position.x = std::cos(orbitAngle) * orbitRadius;
  cam.position.z = std::sin(orbitAngle) * orbitRadius;
  cam.position.y = 1.5 + std::cos(orbitAngle * 1.3) * 2.0;
}

void CinematicCamera::cycleMode() {
  int nextMode = (static_cast<int>(mode) + 1) % 5;
  mode = static_cast<CinematicMode>(nextMode);
  cinematicTime = 0.0;
  orbitAngle = 0.0;
}

const char* CinematicCamera::getModeName() const {
  return getCinematicModeName(mode);
}

void CinematicCamera::reset() {
  cam.position = initialPos;
  orbitAngle = 0.0;
  cinematicTime = 0.0;
}

const char* getCinematicModeName(CinematicMode mode) {
  switch (mode) {
    case CinematicMode::Manual:
      return "Manual Control";
    case CinematicMode::SmoothOrbit:
      return "Smooth Orbit";
    case CinematicMode::WaveMotion:
      return "Wave Motion";
    case CinematicMode::RisingSpiral:
      return "Rising Spiral";
    case CinematicMode::CloseFlyby:
      return "Close Fly-by";
    default:
      return "Unknown";
  }
}

