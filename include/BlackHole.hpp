#pragma once
#include "Vector3.hpp"
#include <vector>

class BlackHole {
public:
  double mass;
  double rs; // Schwarzschild radius

  BlackHole(double mass = 1.0);

  // Integrate a ray and return the final accumulated color
  Vector3 trace(const Ray &ray, double stepSize = 0.1,
                double maxDist = 100.0) const;

private:
  Vector3 acceleration(const Vector3 &pos, const Vector3 &vel) const;

  // Helper for accretion disk texture/noise
  double diskDensity(const Vector3 &pos) const;
  Vector3 diskColor(double density, double r) const;
  Vector3 sampleBackground(const Vector3 &dir) const;
};
