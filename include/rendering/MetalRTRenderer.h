#ifndef METAL_RT_RENDERER_H
#define METAL_RT_RENDERER_H

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

// Render a frame
void metal_rt_renderer_render(MetalRTRenderer *renderer,
                              const CameraData *camera, float time);

// Get output texture data (RGBA8)
const void *metal_rt_renderer_get_pixels(MetalRTRenderer *renderer);

#ifdef __cplusplus
}
#endif

#endif // METAL_RT_RENDERER_H
