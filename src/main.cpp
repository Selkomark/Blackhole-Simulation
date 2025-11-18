#include "../include/MetalRTRenderer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>

void renderTextHints(SDL_Renderer *renderer, TTF_Font *font, bool showHints,
                     int width) {
  if (!showHints)
    return;

  // Semi-transparent background
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_Rect overlay = {10, 10, 450, 180};
  SDL_RenderFillRect(renderer, &overlay);

  // Border
  SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
  SDL_RenderDrawRect(renderer, &overlay);

  // Render text
  SDL_Color textColor = {255, 255, 255, 255};
  SDL_Color highlightColor = {100, 200, 255, 255};

  const char *hints[] = {"CONTROLS:",
                         "WASD - Move Camera",
                         "Space/Shift - Up/Down",
                         "P - Toggle Auto-Orbit",
                         "R - Reset Camera",
                         "Tab - Toggle This Help",
                         "ESC/Q - Quit"};

  int y = 20;
  for (int i = 0; i < 7; i++) {
    SDL_Surface *surface = TTF_RenderText_Blended(
        font, hints[i], i == 0 ? highlightColor : textColor);
    if (surface) {
      SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_Rect destRect = {20, y, surface->w, surface->h};
      SDL_RenderCopy(renderer, texture, nullptr, &destRect);
      SDL_DestroyTexture(texture);
      SDL_FreeSurface(surface);
      y += 25;
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

  // 4K Resolution
  const int WIDTH = 3840;
  const int HEIGHT = 2160;

  SDL_Window *window =
      SDL_CreateWindow("Black Hole Simulation - Metal GPU (4K)",
                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH,
                       HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
  if (window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
              << std::endl;
    return 1;
  }

  SDL_Renderer *sdlRenderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  // Load font
  TTF_Font *font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);
  if (!font) {
    std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError()
              << std::endl;
    font = nullptr; // Continue without text
  }

  // Print controls to console
  std::cout << "\n=== BLACK HOLE SIMULATION - METAL GPU ===\n";
  std::cout << "Resolution: 4K (3840 × 2160)\n";
  std::cout << "WASD       - Move camera\n";
  std::cout << "Space      - Move camera up\n";
  std::cout << "Shift      - Move camera down\n";
  std::cout << "P          - Toggle auto-orbit (default: ON)\n";
  std::cout << "R          - Reset camera position\n";
  std::cout << "Tab        - Toggle hints overlay\n";
  std::cout << "ESC/Q      - Quit\n";
  std::cout << "=========================================\n\n";

  // Initialize Metal Renderer
  MetalRTRenderer *metalRenderer = metal_rt_renderer_create(WIDTH, HEIGHT);
  if (!metalRenderer) {
    std::cerr << "Failed to create Metal renderer!" << std::endl;
    return 1;
  }

  // Create SDL texture for display (ABGR8888 matches Metal's RGBA8Unorm byte
  // order on little-endian)
  SDL_Texture *displayTexture =
      SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ABGR8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

  // Initialize Camera
  CameraData camera;
  float initialPos[3] = {0.0f, 3.0f, -15.0f};
  memcpy(camera.position, initialPos, sizeof(float) * 3);
  camera.fov = 60.0f;

  bool quit = false;
  bool autoOrbit = true;
  bool showHints = true;
  double orbitAngle = 0.0;
  double orbitRadius = 15.0;
  double orbitHeight = 3.0;

  SDL_Event e;
  auto startTime = std::chrono::high_resolution_clock::now();
  auto lastTime = std::chrono::high_resolution_clock::now();
  int frameCount = 0;
  double fpsUpdateTime = 0.0;
  double currentFPS = 0.0;

  while (!quit) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double deltaTime =
        std::chrono::duration<double>(currentTime - lastTime).count();
    lastTime = currentTime;

    double elapsedTime =
        std::chrono::duration<double>(currentTime - startTime).count();

    // Handle Input
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
          memcpy(camera.position, initialPos, sizeof(float) * 3);
          orbitAngle = 0.0;
          std::cout << "Camera reset" << std::endl;
          break;
        case SDLK_TAB:
          showHints = !showHints;
          std::cout << "Hints: " << (showHints ? "ON" : "OFF") << std::endl;
          break;
        }
      }
    }

    const Uint8 *currentKeyStates = SDL_GetKeyboardState(nullptr);
    float moveSpeed = 5.0f * deltaTime;

    // Manual controls
    bool manualMovement = false;
    float forward[3] = {camera.forward[0], camera.forward[1],
                        camera.forward[2]};
    float right[3] = {camera.right[0], camera.right[1], camera.right[2]};
    float up[3] = {camera.up[0], camera.up[1], camera.up[2]};

    if (currentKeyStates[SDL_SCANCODE_W]) {
      for (int i = 0; i < 3; i++)
        camera.position[i] += forward[i] * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_S]) {
      for (int i = 0; i < 3; i++)
        camera.position[i] -= forward[i] * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_A]) {
      for (int i = 0; i < 3; i++)
        camera.position[i] -= right[i] * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_D]) {
      for (int i = 0; i < 3; i++)
        camera.position[i] += right[i] * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_SPACE]) {
      for (int i = 0; i < 3; i++)
        camera.position[i] += up[i] * moveSpeed;
      manualMovement = true;
    }
    if (currentKeyStates[SDL_SCANCODE_LSHIFT]) {
      for (int i = 0; i < 3; i++)
        camera.position[i] -= up[i] * moveSpeed;
      manualMovement = true;
    }

    if (manualMovement && autoOrbit) {
      autoOrbit = false;
      std::cout << "Auto-orbit disabled (manual control)" << std::endl;
    }

    // Auto-orbit
    if (autoOrbit) {
      orbitAngle += 0.3 * deltaTime;
      camera.position[0] = std::cos(orbitAngle) * orbitRadius;
      camera.position[2] = std::sin(orbitAngle) * orbitRadius;
      camera.position[1] = orbitHeight;
    }

    // Update camera vectors (look at origin)
    float len = std::sqrt(camera.position[0] * camera.position[0] +
                          camera.position[1] * camera.position[1] +
                          camera.position[2] * camera.position[2]);
    camera.forward[0] = -camera.position[0] / len;
    camera.forward[1] = -camera.position[1] / len;
    camera.forward[2] = -camera.position[2] / len;

    float worldUp[3] = {0.0f, 1.0f, 0.0f};
    camera.right[0] =
        camera.forward[1] * worldUp[2] - camera.forward[2] * worldUp[1];
    camera.right[1] =
        camera.forward[2] * worldUp[0] - camera.forward[0] * worldUp[2];
    camera.right[2] =
        camera.forward[0] * worldUp[1] - camera.forward[1] * worldUp[0];
    len = std::sqrt(camera.right[0] * camera.right[0] +
                    camera.right[1] * camera.right[1] +
                    camera.right[2] * camera.right[2]);
    camera.right[0] /= len;
    camera.right[1] /= len;
    camera.right[2] /= len;

    camera.up[0] = camera.right[1] * camera.forward[2] -
                   camera.right[2] * camera.forward[1];
    camera.up[1] = camera.right[2] * camera.forward[0] -
                   camera.right[0] * camera.forward[2];
    camera.up[2] = camera.right[0] * camera.forward[1] -
                   camera.right[1] * camera.forward[0];

    // Render with Metal
    metal_rt_renderer_render(metalRenderer, &camera, elapsedTime);

    // Update SDL texture
    const void *pixels = metal_rt_renderer_get_pixels(metalRenderer);
    SDL_UpdateTexture(displayTexture, nullptr, pixels, WIDTH * 4);

    // Display
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, displayTexture, nullptr, nullptr);

    // Render hints
    if (font) {
      renderTextHints(sdlRenderer, font, showHints, WIDTH);
    }

    SDL_RenderPresent(sdlRenderer);

    // FPS calculation
    frameCount++;
    fpsUpdateTime += deltaTime;
    if (fpsUpdateTime >= 0.5) {
      currentFPS = frameCount / fpsUpdateTime;
      frameCount = 0;
      fpsUpdateTime = 0.0;

      std::string title = "Black Hole Simulation - Metal GPU (4K) | FPS: " +
                          std::to_string((int)currentFPS) +
                          " | Auto-orbit: " + (autoOrbit ? "ON" : "OFF");
      SDL_SetWindowTitle(window, title.c_str());
    }
  }

  // Cleanup
  metal_rt_renderer_destroy(metalRenderer);
  SDL_DestroyTexture(displayTexture);
  if (font)
    TTF_CloseFont(font);
  SDL_DestroyRenderer(sdlRenderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();

  return 0;
}
