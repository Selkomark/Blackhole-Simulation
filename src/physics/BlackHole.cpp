#include "../../include/physics/BlackHole.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

BlackHole::BlackHole(double mass) : mass(mass), rs(2.0 * mass) {}

Vector3 BlackHole::acceleration(const Vector3 &pos, const Vector3 &vel) const
{
  double r2 = pos.lengthSquared();
  double r = std::sqrt(r2);
  Vector3 h = pos.cross(vel);
  double h2 = h.lengthSquared();
  double factor = -1.5 * rs * h2 / (r2 * r2 * r);
  return pos * factor;
}

// Simple procedural noise for disk
double BlackHole::diskDensity(const Vector3 &pos) const
{
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

// Calculate Doppler beaming factor
double BlackHole::dopplerFactor(const Vector3 &pos, const Vector3 &rayDir) const
{
  double r = pos.length();

  // Keplerian orbital velocity: v = sqrt(GM/r) = sqrt(RS/(2r)) in geometrized units
  double v_orbital = std::sqrt(rs / (2.0 * r));
  v_orbital = std::min(v_orbital, 0.5); // Cap at 0.5c for numerical stability

  // Disk rotates clockwise when viewed from above (positive y)
  // This makes the left side approach the observer and the right side recede
  // Velocity direction is tangent to orbit: perpendicular to radial direction in x-z plane
  Vector3 radial_xz = Vector3(pos.x, 0.0, pos.z).normalized();
  Vector3 velocity = Vector3(radial_xz.z, 0.0, -radial_xz.x) * v_orbital;

  // Doppler factor: δ = 1 / (γ(1 - β·n))
  // where β = v/c, n is direction toward observer (opposite of ray direction)
  double beta = v_orbital; // v/c (c = 1 in our units)
  double gamma = 1.0 / std::sqrt(1.0 - beta * beta);

  // Component of velocity toward observer (negative of ray direction)
  double beta_parallel = -velocity.dot(rayDir);

  // Doppler factor
  double delta = 1.0 / (gamma * (1.0 - beta_parallel));

  return delta;
}

Vector3 BlackHole::diskColor(double density, double r, const Vector3 &pos, const Vector3 &rayDir, int colorMode) const
{
  double t = (r - rs * 2.5) / (rs * 9.5);
  t = std::min(std::max(t, 0.0), 1.0);

  // Define color palettes for different modes
  Vector3 hot, mid, cold;
  Vector3 doppler_bright, doppler_dim;

  if (colorMode == 0)
  {
    // Blue mode - Interstellar style with extra blue in inner region
    hot = Vector3(0.7, 0.85, 1.0);  // Inner: More blue
    mid = Vector3(0.75, 0.85, 1.0); // Mid: Blue-white
    cold = Vector3(0.5, 0.6, 0.8);  // Outer: Cooler blue
    doppler_bright = Vector3(0.85, 0.92, 1.0);
    doppler_dim = Vector3(0.5, 0.6, 0.8);
  }
  else if (colorMode == 1)
  {
    // Orange mode - Warm glowing plasma
    hot = Vector3(1.0, 0.9, 0.7);  // Inner: Bright orange-white
    mid = Vector3(1.0, 0.75, 0.5); // Mid: Orange
    cold = Vector3(0.9, 0.6, 0.4); // Outer: Dim orange
    doppler_bright = Vector3(1.0, 0.95, 0.85);
    doppler_dim = Vector3(0.8, 0.5, 0.3);
  }
  else
  {
    // Red mode - Hot red plasma
    hot = Vector3(1.0, 0.85, 0.75); // Inner: Bright red-white
    mid = Vector3(1.0, 0.6, 0.5);   // Mid: Red-orange
    cold = Vector3(0.85, 0.4, 0.3); // Outer: Deep red
    doppler_bright = Vector3(1.0, 0.9, 0.85);
    doppler_dim = Vector3(0.7, 0.3, 0.2);
  }

  // Blend between hot, mid, and cold
  Vector3 baseColor;
  if (t < 0.5)
  {
    baseColor = hot * (1.0 - t * 2.0) + mid * (t * 2.0);
  }
  else
  {
    baseColor = mid * (1.0 - (t - 0.5) * 2.0) + cold * ((t - 0.5) * 2.0);
  }

  // Apply Doppler beaming
  double delta = dopplerFactor(pos, rayDir);

  // Intensity boost: I_observed = I_emitted * δ^3 (for emission)
  double intensity_boost = std::pow(delta, 3.0);

  // Frequency shift affects color
  Vector3 doppler_color = baseColor;
  if (delta > 1.0)
  {
    // Approaching: shift toward brighter color
    double shift = std::min((delta - 1.0) * 2.0, 0.4);
    doppler_color = baseColor * (1.0 - shift) + doppler_bright * shift;
  }
  else
  {
    // Receding: shift toward dimmer color
    double shift = std::min((1.0 - delta) * 2.0, 0.3);
    doppler_color = baseColor * (1.0 - shift) + doppler_dim * shift;
  }

  return doppler_color * density * 4.0 * intensity_boost;
}

Vector3 BlackHole::sampleBackground(const Vector3 &dir) const
{
  constexpr double pi = 3.14159265358979323846;
  double u = 0.5 + std::atan2(dir.z, dir.x) / (2 * pi);
  double v = 0.5 - std::asin(dir.y) / pi;

  Vector3 color(0, 0, 0); // Pure black background

  // Stars only (no galaxy band)
  uint32_t hash =
      (uint32_t)(u * 4000) * 19349663 + (uint32_t)(v * 4000) * 83492791;
  if ((hash % 1000) < 2)
  {
    color += Vector3(1, 1, 1) * (0.5 + (hash % 100) / 200.0);
  }

  return color;
}

Vector3 BlackHole::trace(const Ray &ray, double stepSize,
                         double maxDist) const
{
  Vector3 pos = ray.origin;
  Vector3 vel = ray.direction;

  Vector3 accumulatedColor(0, 0, 0);
  double transmittance = 1.0;
  double totalDist = 0;

  while (totalDist < maxDist && transmittance > 0.01)
  {
    double r2 = pos.lengthSquared();

    // Event Horizon
    if (r2 < rs * rs)
    {
      return accumulatedColor; // Black (absorbed)
    }

    // Volumetric Accretion Disk Integration
    double density = diskDensity(pos);
    if (density > 0.001)
    {
      double r = std::sqrt(r2);
      Vector3 emission = diskColor(density, r, pos, vel, 0); // Default to blue mode
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
