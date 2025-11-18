#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../camera/Camera.hpp"
#include "../camera/CinematicCamera.hpp"
#include "../ui/HUD.hpp"
#include "../rendering/MetalRTRenderer.h"
#include "../physics/BlackHole.hpp"

/**
 * Main application class managing the simulation lifecycle
 */
class Application {
public:
  Application();
  ~Application();
  
  // Initialize SDL, window, and renderers
  bool initialize();
  
  // Main application loop
  void run();

private:
  // SDL components
  SDL_Window *window;
  SDL_Renderer *sdlRenderer;
  TTF_Font *font;
  
  // Rendering
  MetalRTRenderer *gpuRenderer;
  SDL_Texture *gpuTexture;
  
  // Simulation components
  BlackHole *blackHole;
  Camera *camera;
  CinematicCamera *cinematicCamera;
  HUD *hud;
  
  // Window properties (dynamic)
  int windowWidth;
  int windowHeight;
  bool isFullscreen;
  
  // State
  bool running;
  int currentFPS;
  
  // Private methods
  void handleEvents();
  void update(double deltaTime);
  void render(double elapsedTime);
  void updateWindowTitle();
  void cleanup();
  void toggleFullscreen();
  void handleWindowResize(int width, int height);
  void recreateRenderTargets();
  
  // Helper to convert camera data for GPU
  void prepareCameraData(CameraData &data);
};

