#include <metal_stdlib>
using namespace metal;

// Constants
constant float RS = 2.0; // Schwarzschild radius (mass = 1.0)
constant float PI = 3.14159265359;

// Structures matching C++ layout
struct Camera {
    float3 position;
    float _pad0;
    float3 forward;
    float _pad1;
    float3 right;
    float _pad2;
    float3 up;
    float fov;
};

struct Uniforms {
    Camera camera;
    uint2 resolution;
    float time;
};

// Vector math utilities
float3 cross_product(float3 a, float3 b) {
    return float3(a.y * b.z - a.z * b.y,
                  a.z * b.x - a.x * b.z,
                  a.x * b.y - a.y * b.x);
}

// Schwarzschild geodesic acceleration
float3 geodesic_acceleration(float3 pos, float3 vel) {
    float r2 = dot(pos, pos);
    float r = sqrt(r2);
    float3 h = cross_product(pos, vel);
    float h2 = dot(h, h);
    float factor = -1.5 * RS * h2 / (r2 * r2 * r);
    return pos * factor;
}

// RK4 Integration step
void rk4_step(thread float3& pos, thread float3& vel, float dt) {
    // k1
    float3 k1_v = geodesic_acceleration(pos, vel);
    float3 k1_p = vel;
    
    // k2
    float3 k2_v = geodesic_acceleration(pos + k1_p * (dt * 0.5), vel + k1_v * (dt * 0.5));
    float3 k2_p = vel + k1_v * (dt * 0.5);
    
    // k3
    float3 k3_v = geodesic_acceleration(pos + k2_p * (dt * 0.5), vel + k2_v * (dt * 0.5));
    float3 k3_p = vel + k2_v * (dt * 0.5);
    
    // k4
    float3 k4_v = geodesic_acceleration(pos + k3_p * dt, vel + k3_v * dt);
    float3 k4_p = vel + k3_v * dt;
    
    // Update
    vel = vel + (k1_v + k2_v * 2.0 + k3_v * 2.0 + k4_v) * (dt / 6.0);
    pos = pos + (k1_p + k2_p * 2.0 + k3_p * 2.0 + k4_p) * (dt / 6.0);
    
    // Renormalize velocity (null geodesic)
    vel = normalize(vel);
}

// Accretion disk density
float disk_density(float3 pos) {
    float r = length(pos);
    
    // Disk bounds
    if (r < RS * 2.5 || r > RS * 12.0) return 0.0;
    if (abs(pos.y) > 0.2) return 0.0;
    
    // Procedural pattern
    float angle = atan2(pos.z, pos.x);
    float spiral = sin(angle * 3.0 + r * 0.5);
    float rings = sin(r * 2.0);
    float noise = (spiral + rings) * 0.5 + 0.5;
    
    // Fade edges
    float fade = 1.0;
    if (r < RS * 3.0) fade = (r - RS * 2.5) / (RS * 0.5);
    if (r > RS * 10.0) fade = (RS * 12.0 - r) / (RS * 2.0);
    
    return noise * fade * exp(-abs(pos.y) * 10.0);
}

// Disk color based on temperature
float3 disk_color(float density, float r) {
    float3 hot = float3(0.8, 0.9, 1.0);
    float3 cold = float3(1.0, 0.3, 0.0);
    
    float t = (r - RS * 2.5) / (RS * 9.5);
    t = clamp(t, 0.0, 1.0);
    
    float3 base_color = mix(hot, cold, t);
    return base_color * density * 2.0;
}

// Background starfield
float3 sample_background(float3 dir) {
    float u = 0.5 + atan2(dir.z, dir.x) / (2.0 * PI);
    float v = 0.5 - asin(dir.y) / PI;
    
    float3 color = float3(0.0);
    
    // Galaxy band
    float band = exp(-abs(dir.y) * 5.0);
    color += float3(0.5, 0.6, 0.8) * band * 0.5;
    
    // Stars (hash)
    uint hash = uint(u * 4000) * 19349663u + uint(v * 4000) * 83492791u;
    if ((hash % 1000u) < 2u) {
        color += float3(1.0) * (0.5 + float(hash % 100u) / 200.0);
    }
    
    return color;
}

// Ray generation kernel
kernel void ray_generation(
    texture2d<float, access::write> output_texture [[texture(0)]],
    constant Uniforms& uniforms [[buffer(0)]],
    uint2 tid [[thread_position_in_grid]])
{
    if (tid.x >= uniforms.resolution.x || tid.y >= uniforms.resolution.y) {
        return;
    }
    
    // Simple test pattern - checkerboard
    float checker = float((tid.x / 32 + tid.y / 32) % 2);
    float3 color = float3(checker, 0.5, 1.0 - checker);
    
    // Write output
    output_texture.write(float4(color, 1.0), tid);
}
