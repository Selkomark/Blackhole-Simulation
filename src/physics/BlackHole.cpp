#include "../../include/physics/BlackHole.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

BlackHole::BlackHole(double mass) : mass(mass), rs(2.0 * mass) {}

Vector3 BlackHole::acceleration(const Vector3 &pos, const Vector3 &vel) const {
  double r2 = pos.lengthSquared();
  double r = std::sqrt(r2);
  Vector3 h = pos.cross(vel);
  double h2 = h.lengthSquared();
  double factor = -1.5 * rs * h2 / (r2 * r2 * r);
  return pos * factor;
}

// Simple procedural noise for disk
double BlackHole::diskDensity(const Vector3 &pos) const {
  double r = pos.length();

  // Disk bounds
  if (r < rs * 2.5 || r > rs * 12.0)
    return 0.0;
  if (std::abs(pos.y) > 0.2)
    return 0.0; // Thin disk

  // Noise-like pattern based on angle and radius
  double angle = std::atan2(pos.z, pos.x);
  double spiral = std::sin(angle * 3.0 + r * 0.5);
  double rings = std::sin(r * 2.0);

  double noise = (spiral + rings) * 0.5 + 0.5;

  // Fade edges
  double fade = 1.0;
  if (r < rs * 3.0)
    fade = (r - rs * 2.5) / (rs * 0.5);
  if (r > rs * 10.0)
    fade = (rs * 12.0 - r) / (rs * 2.0);

  return noise * fade * std::exp(-std::abs(pos.y) * 10.0);
}

Vector3 BlackHole::diskColor(double density, double r) const {
  // Temperature gradient based on radius - White hot palette
  // Hot inner: Pure white
  Vector3 hot(1.0, 1.0, 1.0);      // Pure white
  // Mid: Slightly warm white
  Vector3 mid(1.0, 0.95, 0.9);     // Warm white
  // Cold outer: Dim white/gray
  Vector3 cold(0.7, 0.65, 0.6);    // Dim warm gray

  double t = (r - rs * 2.5) / (rs * 9.5);
  t = std::clamp(t, 0.0, 1.0);

  // Blend between hot, mid, and cold
  Vector3 baseColor;
  if (t < 0.5) {
    baseColor = hot * (1.0 - t * 2.0) + mid * (t * 2.0);
  } else {
    baseColor = mid * (1.0 - (t - 0.5) * 2.0) + cold * ((t - 0.5) * 2.0);
  }

  return baseColor * density * 3.5;
}

Vector3 BlackHole::sampleBackground(const Vector3 &dir) const {
  double u = 0.5 + std::atan2(dir.z, dir.x) / (2 * std::numbers::pi);
  double v = 0.5 - std::asin(dir.y) / std::numbers::pi;

  Vector3 color(0, 0, 0);  // Pure black background

  // Stars only (no galaxy band)
  uint32_t hash =
      (uint32_t)(u * 4000) * 19349663 + (uint32_t)(v * 4000) * 83492791;
  if ((hash % 1000) < 2) {
    color += Vector3(1, 1, 1) * (0.5 + (hash % 100) / 200.0);
  }

  return color;
}

Vector3 BlackHole::trace(const Ray &ray, double stepSize,
                         double maxDist) const {
  Vector3 pos = ray.origin;
  Vector3 vel = ray.direction;

  Vector3 accumulatedColor(0, 0, 0);
  double transmittance = 1.0;
  double totalDist = 0;

  while (totalDist < maxDist && transmittance > 0.01) {
    double r2 = pos.lengthSquared();

    // Event Horizon
    if (r2 < rs * rs) {
      return accumulatedColor; // Black (absorbed)
    }

    // Volumetric Accretion Disk Integration
    double density = diskDensity(pos);
    if (density > 0.001) {
      double r = std::sqrt(r2);
      Vector3 emission = diskColor(density, r);
      double absorption = density * 0.5;

      // Beer's Law integration for this step
      double dt = stepSize; // Approximation
      double stepTransmittance = std::exp(-absorption * dt);

      accumulatedColor += emission * transmittance * (1.0 - stepTransmittance);
      transmittance *= stepTransmittance;
    }

    // Adaptive Step
    double r = std::sqrt(r2);
    double dt = stepSize * (r / (rs * 2 + 0.1));
    if (dt < 0.02)
      dt = 0.02; // Minimum step
    if (dt > 0.5)
      dt = 0.5; // Maximum step

    // RK4
    Vector3 k1_v = acceleration(pos, vel);
    Vector3 k1_p = vel;
    Vector3 k2_v =
        acceleration(pos + k1_p * (dt * 0.5), vel + k1_v * (dt * 0.5));
    Vector3 k2_p = vel + k1_v * (dt * 0.5);
    Vector3 k3_v =
        acceleration(pos + k2_p * (dt * 0.5), vel + k2_v * (dt * 0.5));
    Vector3 k3_p = vel + k2_v * (dt * 0.5);
    Vector3 k4_v = acceleration(pos + k3_p * dt, vel + k3_v * dt);
    Vector3 k4_p = vel + k3_v * dt;

    Vector3 next_vel =
        vel + (k1_v + k2_v * 2.0 + k3_v * 2.0 + k4_v) * (dt / 6.0);
    Vector3 next_pos =
        pos + (k1_p + k2_p * 2.0 + k3_p * 2.0 + k4_p) * (dt / 6.0);

    pos = next_pos;
    vel = next_vel; // Don't normalize here to conserve angular momentum better?
                    // Actually for null geodesics |v| should be constant c.
                    // Numerical error might drift it, so normalizing is safer
                    // for stability.
    vel = vel.normalized();

    totalDist += dt;
  }

  // Add background if ray escapes
  accumulatedColor += sampleBackground(vel) * transmittance;

  return accumulatedColor;
}
