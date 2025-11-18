#include "../include/MetalRTRenderer.h"
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

    NSLog(@"Metal renderer initialized successfully");
    NSLog(@"Resolution: %dx%d", width, height);
    NSLog(@"Texture format: RGBA8Unorm");

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
    [renderer->outputTexture
           getBytes:renderer->pixelData.data()
        bytesPerRow:renderer->width * 4
         fromRegion:MTLRegionMake2D(0, 0, renderer->width, renderer->height)
        mipmapLevel:0];

    // Debug: Check first pixel
    static bool logged = false;
    if (!logged) {
      uint8_t *pixels = renderer->pixelData.data();
      NSLog(@"First pixel RGBA: %d %d %d %d", pixels[0], pixels[1], pixels[2],
            pixels[3]);
      logged = true;
    }
  }
}

const void *metal_rt_renderer_get_pixels(MetalRTRenderer *renderer) {
  return renderer->pixelData.data();
}
