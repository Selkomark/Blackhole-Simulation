#include <metal_stdlib>
using namespace metal;

// Constants
constant float RS = 2.0; // Schwarzschild radius (mass = 1.0)
constant float PI = 3.14159265359;
constant float STEP_SIZE = 0.1;
constant float MAX_DIST = 100.0;
constant float MIN_STEP = 0.02;
constant float MAX_STEP = 0.5;

// Structures matching C++ layout
// Note: Metal float3 is 16-byte aligned, but C++ uses float[3] which is 12 bytes
// So we use packed_float3 or match the exact C++ layout
struct Camera {
    packed_float3 position;
    float _pad0;
    packed_float3 forward;
    float _pad1;
    packed_float3 right;
    float _pad2;
    packed_float3 up;
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
float disk_density(float3 pos, float time) {
    float r = length(pos);
    
    // Disk bounds
    if (r < RS * 2.5 || r > RS * 12.0) return 0.0;
    if (abs(pos.y) > 0.2) return 0.0;
    
    // Procedural pattern with time-based rotation
    // Rotate the disk pattern over time to create animation
    float angle = atan2(pos.z, pos.x);
    float rotationSpeed = 1.0; // Rotation speed (increased for more visible animation)
    float rotatedAngle = angle + time * rotationSpeed;
    float spiral = sin(rotatedAngle * 3.0 + r * 0.5);
    float rings = sin(r * 2.0);
    float noise = (spiral + rings) * 0.5 + 0.5;
    
    // Fade edges
    float fade = 1.0;
    if (r < RS * 3.0) fade = (r - RS * 2.5) / (RS * 0.5);
    if (r > RS * 10.0) fade = (RS * 12.0 - r) / (RS * 2.0);
    
    return noise * fade * exp(-abs(pos.y) * 10.0);
}

// Disk color based on temperature - White hot palette
float3 disk_color(float density, float r) {
    // Hot inner: Pure white
    float3 hot = float3(1.0, 1.0, 1.0);      // Pure white
    // Mid: Slightly warm white
    float3 mid = float3(1.0, 0.95, 0.9);     // Warm white
    // Cold outer: Dim white/gray
    float3 cold = float3(0.7, 0.65, 0.6);    // Dim warm gray
    
    float t = (r - RS * 2.5) / (RS * 9.5);
    t = clamp(t, 0.0, 1.0);
    
    // Blend between hot, mid, and cold
    float3 base_color;
    if (t < 0.5) {
        base_color = mix(hot, mid, t * 2.0);
    } else {
        base_color = mix(mid, cold, (t - 0.5) * 2.0);
    }
    
    return base_color * density * 3.5;
}

// Background starfield with time-based rotation
float3 sample_background(float3 dir, float time) {
    // Rotate background over time for continuous animation
    float rotationAngle = time * 0.1; // Slow rotation speed
    float cosRot = cos(rotationAngle);
    float sinRot = sin(rotationAngle);
    
    // Rotate direction around Y axis
    float3 rotatedDir;
    rotatedDir.x = dir.x * cosRot - dir.z * sinRot;
    rotatedDir.y = dir.y;
    rotatedDir.z = dir.x * sinRot + dir.z * cosRot;
    
    float u = 0.5 + atan2(rotatedDir.z, rotatedDir.x) / (2.0 * PI);
    float v = 0.5 - asin(rotatedDir.y) / PI;
    
    float3 color = float3(0.0);  // Pure black background
    
    // Stars only (no galaxy band)
    uint hash = uint(u * 4000.0) * 19349663u + uint(v * 4000.0) * 83492791u;
    if ((hash % 1000u) < 2u) {
        color += float3(1.0) * (0.5 + float(hash % 100u) / 200.0);
    }
    
    return color;
}

// Full volumetric ray tracing
float3 trace_ray(float3 origin, float3 direction, float time) {
    float3 pos = origin;
    float3 vel = direction;
    
    float3 accumulatedColor = float3(0.0);
    float transmittance = 1.0;
    float totalDist = 0.0;
    
    while (totalDist < MAX_DIST && transmittance > 0.01) {
        float r2 = dot(pos, pos);
        
        // Event Horizon
        if (r2 < RS * RS) {
            return accumulatedColor; // Black (absorbed)
        }
        
        // Volumetric Accretion Disk Integration
        float density = disk_density(pos, time);
        if (density > 0.001) {
            float r = sqrt(r2);
            float3 emission = disk_color(density, r);
            float absorption = density * 0.5;
            
            // Beer's Law integration for this step
            float dt = STEP_SIZE; // Approximation
            float stepTransmittance = exp(-absorption * dt);
            
            accumulatedColor += emission * transmittance * (1.0 - stepTransmittance);
            transmittance *= stepTransmittance;
        }
        
        // Adaptive Step
        float r = sqrt(r2);
        float dt = STEP_SIZE * (r / (RS * 2.0 + 0.1));
        if (dt < MIN_STEP) dt = MIN_STEP;
        if (dt > MAX_STEP) dt = MAX_STEP;
        
        // RK4 integration
        rk4_step(pos, vel, dt);
        
        totalDist += dt;
    }
    
        // Add background if ray escapes - pass time for rotation
        accumulatedColor += sample_background(vel, time) * transmittance;
    
    return accumulatedColor;
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
    
    // Debug: Check if camera data is valid
    // packed_float3 can be implicitly converted to float3
    float3 camPos = float3(uniforms.camera.position);
    float3 camForward = float3(uniforms.camera.forward);
    
    // Check for invalid camera data (all zeros)
    if (length(camPos) < 0.001 && length(camForward) < 0.001) {
        // Invalid camera - output red to indicate error
        output_texture.write(float4(1.0, 0.0, 0.0, 1.0), tid);
        return;
    }
    
    // Calculate aspect ratio and FOV
    float aspectRatio = float(uniforms.resolution.x) / float(uniforms.resolution.y);
    float fovRad = uniforms.camera.fov * PI / 180.0;
    float scale = tan(fovRad * 0.5);
    
    // Calculate pixel coordinates in normalized space
    float px = (2.0 * (float(tid.x) + 0.5) / float(uniforms.resolution.x) - 1.0) * aspectRatio * scale;
    float py = (1.0 - 2.0 * (float(tid.y) + 0.5) / float(uniforms.resolution.y)) * scale;
    
    // Calculate ray direction
    float3 forward = float3(uniforms.camera.forward);
    float3 right = float3(uniforms.camera.right);
    float3 up = float3(uniforms.camera.up);
    float3 dir = normalize(forward + right * px + up * py);
    
    // Trace ray - pass time for animation
    float3 origin = float3(uniforms.camera.position);
    float3 color = trace_ray(origin, dir, uniforms.time);
    
    // Check if color is valid (not NaN or Inf)
    if (isnan(color.x) || isnan(color.y) || isnan(color.z) ||
        isinf(color.x) || isinf(color.y) || isinf(color.z)) {
        color = float3(0.0, 1.0, 0.0); // Green for NaN/Inf
    }
    
    // Tone mapping (simple reinhard)
    color = color / (color + float3(1.0));
    
    // Gamma correction
    color.x = pow(max(color.x, 0.0), 1.0 / 2.2);
    color.y = pow(max(color.y, 0.0), 1.0 / 2.2);
    color.z = pow(max(color.z, 0.0), 1.0 / 2.2);
    
    // Clamp
    color = clamp(color, float3(0.0), float3(1.0));
    output_texture.write(float4(color, 1.0), tid);
}
