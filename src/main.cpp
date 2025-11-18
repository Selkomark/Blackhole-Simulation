#include "../include/BlackHole.hpp"
#include "../include/MetalRTRenderer.h"
#include "../include/Renderer.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <chrono>
#include <cmath>
#include <iostream>

void renderTextHints(SDL_Renderer *renderer, TTF_Font *font, bool showHints,
                     bool useGPU, int fps) {
  if (!showHints || !font)
    return;

  // Semi-transparent background
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_Rect overlay = {10, 10, 500, 220};
  SDL_RenderFillRect(renderer, &overlay);

  // Border
  SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
  SDL_RenderDrawRect(renderer, &overlay);

  // Render text
  SDL_Color textColor = {255, 255, 255, 255};
  SDL_Color highlightColor = {100, 200, 255, 255};
  SDL_Color gpuColor = {255, 100, 100, 255};

  std::string hints[] = {"CONTROLS:",
                         "WASD - Move Camera",
                         "Space/Shift - Up/Down",
                         "P - Toggle Auto-Orbit",
                         "R - Reset Camera",
                         "G - Toggle CPU/GPU (" +
                             std::string(useGPU ? "GPU" : "CPU") + ")",
                         "Tab - Toggle This Help",
                         "ESC/Q - Quit",
                         "FPS: " + std::to_string(fps)};

  int y = 20;
  for (int i = 0; i < 9; i++) {
    SDL_Color color = (i == 0)   ? highlightColor
                      : (i == 5) ? (useGPU ? gpuColor : textColor)
                                 : textColor;
    SDL_Surface *surface =
        TTF_RenderText_Blended(font, hints[i].c_str(), color);
    if (surface) {
      SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_Rect destRect = {20, y, surface->w, surface->h};
      SDL_RenderCopy(renderer, texture, nullptr, &destRect);
      SDL_DestroyTexture(texture);
      SDL_FreeSurface(surface);
      y += 24;
    }
  }
}

int main(int, char **) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
              << std::endl;
    return 1;
  }

  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError()
              << std::endl;
    return 1;
  }

  const int WIDTH = 1920;
  const int HEIGHT = 1080;

  SDL_Window *window =
      SDL_CreateWindow("Black Hole Simulation - CPU/GPU Toggle",
                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH,
                       HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << std::endl;
    return 1;
  }

  SDL_Renderer *sdlRenderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  TTF_Font *font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);
  if (!font) {
    std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError()
              << std::endl;
  }

  std::cout << "\n=== BLACK HOLE SIMULATION - CPU/GPU TOGGLE ===\n";
  std::cout << "Resolution: 1920 × 1080\n";
  std::cout << "G          - Toggle CPU/GPU rendering\n";
  std::cout << "WASD       - Move camera\n";
  std::cout << "Space      - Move camera up\n";
  std::cout << "Shift      - Move camera down\n";
  std::cout << "P          - Toggle auto-orbit (default: ON)\n";
  std::cout << "R          - Reset camera position\n";
  std::cout << "Tab        - Toggle hints overlay\n";
  std::cout << "ESC/Q      - Quit\n";
  std::cout << "===========================================\n\n";

  // Initialize both renderers
  BlackHole bh(1.0);
  Vector3 initialPos(0, 3, -15);
  Camera cam(initialPos, Vector3(0, 0, 0), 60.0);
  Renderer cpuRenderer(WIDTH, HEIGHT, sdlRenderer);

  MetalRTRenderer *gpuRenderer = metal_rt_renderer_create(WIDTH, HEIGHT);
  SDL_Texture *gpuTexture = nullptr;
  if (gpuRenderer) {
    gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ABGR8888,
                                   SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    std::cout << "GPU renderer initialized\n";
  } else {
    std::cout << "GPU renderer failed to initialize, CPU only mode\n";
  }

  bool quit = false;
  bool autoOrbit = true;
  bool showHints = true;
  bool useGPU = false; // Start with CPU
  double orbitAngle = 0.0;
  double orbitRadius = 15.0;
  double orbitHeight = 3.0;

  SDL_Event e;
  auto startTime = std::chrono::high_resolution_clock::now();
  auto lastTime = std::chrono::high_resolution_clock::now();
  int frameCount = 0;
  double fpsUpdateTime = 0.0;
  int currentFPS = 0;

  while (!quit) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double deltaTime =
        std::chrono::duration<double>(currentTime - lastTime).count();
    lastTime = currentTime;
    double elapsedTime =
        std::chrono::duration<double>(currentTime - startTime).count();

    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      } else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
          quit = true;
          break;
        case SDLK_p:
          autoOrbit = !autoOrbit;
          std::cout << "Auto-orbit: " << (autoOrbit ? "ON" : "OFF")
                    << std::endl;
          break;
        case SDLK_r:
          cam.position = initialPos;
          orbitAngle = 0.0;
          std::cout << "Camera reset" << std::endl;
          break;
        case SDLK_TAB:
          showHints = !showHints;
          std::cout << "Hints: " << (showHints ? "ON" : "OFF") << std::endl;
          break;
        case SDLK_g:
          if (gpuRenderer) {
            useGPU = !useGPU;
            std::cout << "Switched to " << (useGPU ? "GPU" : "CPU")
                      << " rendering" << std::endl;
          } else {
            std::cout << "GPU renderer not available" << std::endl;
          }
          break;
        }
      }
    }

    const Uint8 *currentKeyStates = SDL_GetKeyboardState(nullptr);
    double moveSpeed = 5.0 * deltaTime;

    bool manualMovement = false;
    if (currentKeyStates[SDL_SCANCODE_W]) {
      cam.position += cam.forward * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_S]) {
      cam.position -= cam.forward * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_A]) {
      cam.position -= cam.right * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_D]) {
      cam.position += cam.right * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_SPACE]) {
      cam.position += cam.up * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_LSHIFT]) {
      cam.position -= cam.up * moveSpeed;
      manualMovement = true;
    }

    if (manualMovement && autoOrbit) {
      autoOrbit = false;
      std::cout << "Auto-orbit disabled (manual control)" << std::endl;
    }

    if (autoOrbit) {
      orbitAngle += 0.3 * deltaTime;
      cam.position.x = std::cos(orbitAngle) * orbitRadius;
      cam.position.z = std::sin(orbitAngle) * orbitRadius;
      cam.position.y = orbitHeight;
    }

    cam.forward = (Vector3(0, 0, 0) - cam.position).normalized();
    cam.right = cam.forward.cross(Vector3(0, 1, 0)).normalized();
    cam.up = cam.right.cross(cam.forward).normalized();

    // Render
    SDL_RenderClear(sdlRenderer);

    if (useGPU && gpuRenderer) {
      // GPU rendering
      CameraData gpuCam;
      gpuCam.position[0] = cam.position.x;
      gpuCam.position[1] = cam.position.y;
      gpuCam.position[2] = cam.position.z;
      gpuCam.forward[0] = cam.forward.x;
      gpuCam.forward[1] = cam.forward.y;
      gpuCam.forward[2] = cam.forward.z;
      gpuCam.right[0] = cam.right.x;
      gpuCam.right[1] = cam.right.y;
      gpuCam.right[2] = cam.right.z;
      gpuCam.up[0] = cam.up.x;
      gpuCam.up[1] = cam.up.y;
      gpuCam.up[2] = cam.up.z;
      gpuCam.fov = cam.fov;

      metal_rt_renderer_render(gpuRenderer, &gpuCam, elapsedTime);
      const void *pixels = metal_rt_renderer_get_pixels(gpuRenderer);
      SDL_UpdateTexture(gpuTexture, nullptr, pixels, WIDTH * 4);
      SDL_RenderCopy(sdlRenderer, gpuTexture, nullptr, nullptr);
    } else {
      // CPU rendering
      cpuRenderer.render(bh, cam);
      cpuRenderer.updateTexture();
    }

    // Render hints
    renderTextHints(sdlRenderer, font, showHints, useGPU, currentFPS);

    SDL_RenderPresent(sdlRenderer);

    // FPS calculation
    frameCount++;
    fpsUpdateTime += deltaTime;
    if (fpsUpdateTime >= 0.5) {
      currentFPS = (int)(frameCount / fpsUpdateTime);
      frameCount = 0;
      fpsUpdateTime = 0.0;

      std::string title = "Black Hole Simulation | " +
                          std::string(useGPU ? "GPU" : "CPU") +
                          " | FPS: " + std::to_string(currentFPS) +
                          " | Auto-orbit: " + (autoOrbit ? "ON" : "OFF");
      SDL_SetWindowTitle(window, title.c_str());
    }
  }

  // Cleanup
  if (gpuRenderer)
    metal_rt_renderer_destroy(gpuRenderer);
  if (gpuTexture)
    SDL_DestroyTexture(gpuTexture);
  if (font)
    TTF_CloseFont(font);
  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();

  return 0;
}
