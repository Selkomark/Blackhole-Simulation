#ifndef METAL_RT_RENDERER_H
#define METAL_RT_RENDERER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer to Metal renderer
typedef struct MetalRTRenderer MetalRTRenderer;

// Camera structure (C-compatible)
typedef struct {
  float position[3];
  float forward[3];
  float up[3];
  float right[3];
  float fov;
} CameraData;

// Create Metal renderer
MetalRTRenderer *metal_rt_renderer_create(int width, int height);

// Destroy Metal renderer
void metal_rt_renderer_destroy(MetalRTRenderer *renderer);

// Resize renderer (recreates textures and buffers)
void metal_rt_renderer_resize(MetalRTRenderer *renderer, int width, int height);

// Render a frame
void metal_rt_renderer_render(MetalRTRenderer *renderer,
                              const CameraData *camera, float time, int colorMode, float colorIntensity);

// Get output texture data (RGBA8)
const void *metal_rt_renderer_get_pixels(MetalRTRenderer *renderer);

// Render and get pixels atomically (for screenshots - ensures fresh render)
// Returns pointer to pixel data, or nullptr on error
// The pixel data is valid until the next render call
const void *metal_rt_renderer_render_and_get_pixels(MetalRTRenderer *renderer,
                                                     const CameraData *camera, float time, int colorMode, float colorIntensity);

// Get pixel data size in bytes
size_t metal_rt_renderer_get_pixel_data_size(MetalRTRenderer *renderer);

#ifdef __cplusplus
}
#endif

#endif // METAL_RT_RENDERER_H
