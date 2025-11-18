#pragma once

#include "Camera.hpp"

/**
 * Cinematic camera modes
 */
enum class CinematicMode {
  Manual = 0,
  SmoothOrbit = 1,
  WaveMotion = 2,
  RisingSpiral = 3,
  CloseFlyby = 4
};

/**
 * Cinematic camera system with multiple automated camera movements
 */
class CinematicCamera {
public:
  CinematicCamera(Camera &camera, const Vector3 &initialPosition);
  
  // Update camera position based on current mode and delta time
  void update(double deltaTime, const uint8_t *keyStates);
  
  // Cycle to next cinematic mode
  void cycleMode();
  
  // Get current mode
  CinematicMode getMode() const { return mode; }
  
  // Get mode name as string
  const char* getModeName() const;
  
  // Reset camera to initial position
  void reset();

private:
  Camera &cam;
  Vector3 initialPos;
  CinematicMode mode;
  
  double orbitAngle;
  double orbitRadius;
  double cinematicTime;
  
  // Rotation state - we apply rotations incrementally each frame
  // These don't accumulate, they're just flags for current rotation direction
  double rotationSpeed;  // Rotation speed multiplier
  
  // Update functions for each mode
  void updateManualMode(double deltaTime, const uint8_t *keyStates);
  void updateSmoothOrbit(double deltaTime);
  void updateWaveMotion(double deltaTime);
  void updateRisingSpiral(double deltaTime);
  void updateCloseFlyby(double deltaTime);
  
  // Update camera look direction based on rotation
  // Rotations are applied incrementally each frame only when keys are pressed
  void updateCameraLookDirection(double deltaTime);
};

// Helper function to get mode name from enum
const char* getCinematicModeName(CinematicMode mode);

