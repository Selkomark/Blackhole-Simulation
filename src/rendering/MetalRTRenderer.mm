#include "../../include/rendering/MetalRTRenderer.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <vector>

struct MetalRTRenderer {
  id<MTLDevice> device;
  id<MTLCommandQueue> commandQueue;
  id<MTLComputePipelineState> pipelineState;
  id<MTLBuffer> uniformBuffer;
  id<MTLTexture> outputTexture;
  std::vector<uint8_t> pixelData;
  int width;
  int height;
};

// Uniforms structure matching Metal shader
struct Uniforms {
  struct {
    float position[3];
    float _pad0;
    float forward[3];
    float _pad1;
    float right[3];
    float _pad2;
    float up[3];
    float fov;
  } camera;
  uint32_t resolution[2];
  float time;
};

MetalRTRenderer *metal_rt_renderer_create(int width, int height) {
  @autoreleasepool {
    MetalRTRenderer *renderer = new MetalRTRenderer();
    renderer->width = width;
    renderer->height = height;
    renderer->pixelData.resize(width * height * 4);

    // Get Metal device
    renderer->device = MTLCreateSystemDefaultDevice();
    if (!renderer->device) {
      NSLog(@"Metal is not supported on this device");
      delete renderer;
      return nullptr;
    }

    // Create command queue
    renderer->commandQueue = [renderer->device newCommandQueue];

    // Load Metal library
    NSError *error = nil;

    // Try to load from file first
    NSString *libraryPath = @"build/default.metallib";
    id<MTLLibrary> library = nil;

    if ([[NSFileManager defaultManager] fileExistsAtPath:libraryPath]) {
      NSURL *libraryURL = [NSURL fileURLWithPath:libraryPath];
      library = [renderer->device newLibraryWithURL:libraryURL error:&error];
      if (library) {
        NSLog(@"Loaded Metal library from: %@", libraryPath);
      }
    }

    // Fallback to default library
    if (!library) {
      NSLog(@"Trying default library...");
      library = [renderer->device newDefaultLibrary];
    }

    if (!library) {
      NSLog(@"Failed to load Metal library: %@", error);
      delete renderer;
      return nullptr;
    }

    // Get kernel function
    id<MTLFunction> kernelFunction =
        [library newFunctionWithName:@"ray_generation"];
    if (!kernelFunction) {
      NSLog(@"Failed to find kernel function");
      delete renderer;
      return nullptr;
    }

    // Create compute pipeline
    renderer->pipelineState =
        [renderer->device newComputePipelineStateWithFunction:kernelFunction
                                                        error:&error];
    if (!renderer->pipelineState) {
      NSLog(@"Failed to create pipeline state: %@", error);
      delete renderer;
      return nullptr;
    }

    // Create uniform buffer
    renderer->uniformBuffer =
        [renderer->device newBufferWithLength:sizeof(Uniforms)
                                      options:MTLResourceStorageModeShared];

    // Create output texture
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:width
                                    height:height
                                 mipmapped:NO];
    textureDesc.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
    textureDesc.storageMode = MTLStorageModeShared;
    renderer->outputTexture =
        [renderer->device newTextureWithDescriptor:textureDesc];

    if (!renderer->outputTexture) {
      NSUInteger maxSize = [renderer->device supportsFamily:MTLGPUFamilyApple1] ? 16384 : 8192;
      NSLog(@"Failed to create output texture at resolution %dx%d", width, height);
      NSLog(@"Maximum texture size supported: %lu x %lu", (unsigned long)maxSize, (unsigned long)maxSize);
      delete renderer;
      return nullptr;
    }

    NSLog(@"Metal renderer initialized successfully");
    NSLog(@"Resolution: %dx%d", width, height);
    NSLog(@"Texture format: RGBA8Unorm");
    NSLog(@"Texture size: %lu bytes", (unsigned long)(width * height * 4));

    return renderer;
  }
}

void metal_rt_renderer_destroy(MetalRTRenderer *renderer) {
  if (renderer) {
    delete renderer;
  }
}

void metal_rt_renderer_render(MetalRTRenderer *renderer,
                              const CameraData *camera, float time) {
  @autoreleasepool {
    // Update uniforms
    Uniforms *uniforms = (Uniforms *)[renderer->uniformBuffer contents];
    memcpy(uniforms->camera.position, camera->position, sizeof(float) * 3);
    memcpy(uniforms->camera.forward, camera->forward, sizeof(float) * 3);
    memcpy(uniforms->camera.right, camera->right, sizeof(float) * 3);
    memcpy(uniforms->camera.up, camera->up, sizeof(float) * 3);
    uniforms->camera.fov = camera->fov;
    uniforms->resolution[0] = renderer->width;
    uniforms->resolution[1] = renderer->height;
    uniforms->time = time;

    // Debug: Log camera data (only first time)
    static bool cameraLogged = false;
    if (!cameraLogged) {
      NSLog(@"Camera position: [%f, %f, %f]", 
            camera->position[0], camera->position[1], camera->position[2]);
      NSLog(@"Camera forward: [%f, %f, %f]", 
            camera->forward[0], camera->forward[1], camera->forward[2]);
      NSLog(@"Camera right: [%f, %f, %f]", 
            camera->right[0], camera->right[1], camera->right[2]);
      NSLog(@"Camera up: [%f, %f, %f]", 
            camera->up[0], camera->up[1], camera->up[2]);
      NSLog(@"Camera FOV: %f", camera->fov);
      NSLog(@"Resolution: %d x %d", renderer->width, renderer->height);
      cameraLogged = true;
    }

    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [renderer->commandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder =
        [commandBuffer computeCommandEncoder];

    [encoder setComputePipelineState:renderer->pipelineState];
    [encoder setTexture:renderer->outputTexture atIndex:0];
    [encoder setBuffer:renderer->uniformBuffer offset:0 atIndex:0];

    // Dispatch threads
    MTLSize threadgroupSize = MTLSizeMake(8, 8, 1);
    MTLSize threadgroupCount = MTLSizeMake(
        (renderer->width + threadgroupSize.width - 1) / threadgroupSize.width,
        (renderer->height + threadgroupSize.height - 1) /
            threadgroupSize.height,
        1);

    [encoder dispatchThreadgroups:threadgroupCount
            threadsPerThreadgroup:threadgroupSize];
    [encoder endEncoding];

    // Commit and wait
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    // Check for errors
    if (commandBuffer.error) {
      NSLog(@"Command buffer error: %@", commandBuffer.error);
    }

    NSLog(@"Threadgroups: %lu x %lu, Threads per group: %lu x %lu",
          (unsigned long)threadgroupCount.width,
          (unsigned long)threadgroupCount.height,
          (unsigned long)threadgroupSize.width,
          (unsigned long)threadgroupSize.height);

    // Read back pixels
    // Metal RGBA8Unorm stores as RGBA (R=byte0, G=byte1, B=byte2, A=byte3)
    // SDL ARGB8888 expects ARGB (A=byte0, R=byte1, G=byte2, B=byte3) on little-endian
    // So we need to convert: RGBA -> ARGB
    size_t bytesPerRow = renderer->width * 4;
    size_t totalBytes = bytesPerRow * renderer->height;
    
    // Temporary buffer for RGBA data
    std::vector<uint8_t> tempBuffer(totalBytes);
    [renderer->outputTexture
           getBytes:tempBuffer.data()
        bytesPerRow:bytesPerRow
         fromRegion:MTLRegionMake2D(0, 0, renderer->width, renderer->height)
        mipmapLevel:0];
    
    // Convert RGBA to BGRA (SDL_PIXELFORMAT_ARGB8888 on little-endian is BGRA in memory)
    uint8_t *rgba = tempBuffer.data();
    uint8_t *bgra = renderer->pixelData.data();
    size_t pixelCount = static_cast<size_t>(renderer->width) * static_cast<size_t>(renderer->height);
    for (size_t i = 0; i < pixelCount; i++) {
      size_t idx = i * 4;
      bgra[idx + 0] = rgba[idx + 2]; // B (from RGBA B)
      bgra[idx + 1] = rgba[idx + 1]; // G (from RGBA G)
      bgra[idx + 2] = rgba[idx + 0]; // R (from RGBA R)
      bgra[idx + 3] = rgba[idx + 3]; // A (from RGBA A)
    }

    // Debug: Check first pixel
    static bool logged = false;
    if (!logged) {
      uint8_t *pixels = renderer->pixelData.data();
      NSLog(@"First pixel ARGB: %d %d %d %d", pixels[0], pixels[1], pixels[2],
            pixels[3]);
      logged = true;
    }
  }
}

const void *metal_rt_renderer_get_pixels(MetalRTRenderer *renderer) {
  return renderer->pixelData.data();
}
