#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "../camera/Camera.hpp"
#include "../camera/CinematicCamera.hpp"
#include "../ui/HUD.hpp"
#include "../rendering/MetalRTRenderer.h"
#include "../physics/BlackHole.hpp"
#include "../utils/ResolutionManager.hpp"
#include "../utils/VideoRecorder.hpp"

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
  Mix_Music *backgroundMusic;
  
  // Rendering
  MetalRTRenderer *gpuRenderer;
  SDL_Texture *gpuTexture;
  
  // Simulation components
  BlackHole *blackHole;
  Camera *camera;
  CinematicCamera *cinematicCamera;
  HUD *hud;
  ResolutionManager *resolutionManager;
  VideoRecorder *videoRecorder;
  
  // Window properties (dynamic)
  int windowWidth;
  int windowHeight;
  int renderWidth;   // Internal rendering resolution
  int renderHeight;  // Internal rendering resolution
  bool isFullscreen;
  bool isResizing; // Flag to prevent recursive resize events
  
  // State
  bool running;
  int currentFPS;
  bool isRecording;
  int colorMode; // 0=blue, 1=orange, 2=red
  float colorIntensity; // Brightness multiplier for accretion disk (default 1.0)
  bool isMusicMuted; // Music mute state
  float currentMusicVolume; // Current music volume (0.0 to 1.0)
  float targetMusicVolume; // Target music volume for fading
  bool isMusicFading; // Whether music is currently fading
  
  // Private methods
  void handleEvents();
  void update(double deltaTime);
  void render(double elapsedTime);
  void updateWindowTitle();
  void cleanup();
  void toggleFullscreen();
  void handleWindowResize(int width, int height);
  void recreateRenderTargets();
  void changeResolution(bool increase);
  
  // Video recording
  void startRecording();
  void stopRecording();
  
  // Helper to convert camera data for GPU
  void prepareCameraData(CameraData &data);
};

