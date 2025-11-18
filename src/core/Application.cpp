#include "../../include/core/Application.hpp"
#include <iostream>
#include <chrono>
#include <string>

Application::Application()
    : window(nullptr), sdlRenderer(nullptr), font(nullptr),
      gpuRenderer(nullptr), gpuTexture(nullptr),
      blackHole(nullptr), camera(nullptr), cinematicCamera(nullptr), hud(nullptr),
      resolutionManager(nullptr),
      windowWidth(1920), windowHeight(1080), isFullscreen(false), isResizing(false),
      running(false), currentFPS(0) {}

Application::~Application() {
  cleanup();
}

bool Application::initialize() {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }

  // Initialize resolution manager and set to 1080p (default)
  resolutionManager = new ResolutionManager();
  resolutionManager->setResolution(5); // 1080p FHD
  
  // Get selected resolution
  const Resolution& res = resolutionManager->getCurrent();
  windowWidth = res.width;
  windowHeight = res.height;

  // Initialize SDL_ttf
  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
    return false;
  }

  // Create window (resizable)
  window = SDL_CreateWindow("Black Hole Simulation | Smooth Orbit",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             windowWidth, windowHeight,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }
  
  // Get actual window size (may differ due to high DPI)
  int actualWidth, actualHeight;
  SDL_GetWindowSize(window, &actualWidth, &actualHeight);
  windowWidth = actualWidth;
  windowHeight = actualHeight;

  // Create SDL renderer (without VSYNC to prevent pausing when idle)
  sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!sdlRenderer) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }

  // Load font
  font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);
  if (!font) {
    std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
  }

  // Initialize GPU renderer (Metal) with actual window size
  gpuRenderer = metal_rt_renderer_create(windowWidth, windowHeight);
  if (!gpuRenderer) {
    std::cerr << "ERROR: GPU renderer failed to initialize!\n";
    std::cerr << "Metal GPU acceleration is required for this simulation.\n";
    return false;
  }

  gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
  
  // Startup messages removed - use HUD instead

  // Initialize simulation components
  blackHole = new BlackHole(1.0);
  // Camera starts further back, looking at black hole center (0, 0, 0)
  Vector3 initialPos(0, 3, -20); // Increased distance from -15 to -20 for better initial view
  camera = new Camera(initialPos, Vector3(0, 0, 0), 60.0); // Points at black hole center
  cinematicCamera = new CinematicCamera(*camera, initialPos);
  hud = new HUD(sdlRenderer, font);

  running = true;
  return true;
}

void Application::run() {
  auto startTime = std::chrono::high_resolution_clock::now();
  auto lastTime = std::chrono::high_resolution_clock::now();
  
  int frameCount = 0;
  double fpsUpdateTime = 0.0;

  while (running) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
    
    // Clamp deltaTime to reasonable bounds (1ms to 100ms)
    if (deltaTime < 0.001) {
      deltaTime = 0.001; // Minimum 1ms
    } else if (deltaTime > 0.1) {
      deltaTime = 0.1; // Maximum 100ms (prevent large jumps)
    }
    
    lastTime = currentTime;
    double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
    
    // Ensure elapsedTime always advances (debug check)
    static double lastElapsedTime = -1.0;
    if (elapsedTime <= lastElapsedTime) {
        // Time didn't advance - this shouldn't happen
        elapsedTime = lastElapsedTime + deltaTime;
    }
    lastElapsedTime = elapsedTime;

    // Always process events (non-blocking)
    handleEvents();
    
    // Always update and render, regardless of input
    update(deltaTime);
    render(elapsedTime);

    // FPS calculation
    frameCount++;
    fpsUpdateTime += deltaTime;
    if (fpsUpdateTime >= 0.5) {
      currentFPS = static_cast<int>(frameCount / fpsUpdateTime);
      frameCount = 0;
      fpsUpdateTime = 0.0;
      updateWindowTitle();
    }
    
    // No delay - keep rendering at full speed
    // The loop must continue running continuously
  }
}

void Application::handleEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    if (e.type == SDL_QUIT) {
      running = false;
    } else if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
          if (isFullscreen) {
            toggleFullscreen(); // Exit fullscreen on ESC
          } else {
            running = false; // Quit if windowed
          }
          break;
        case SDLK_q:
          running = false;
          break;
        case SDLK_f:
          toggleFullscreen();
          break;
        case SDLK_r:
          cinematicCamera->reset();
          // Camera reset logging removed
          break;
        case SDLK_TAB:
          hud->toggleHints();
          // Hints toggle logging removed
          break;
        case SDLK_c:
          cinematicCamera->cycleMode();
          // Mode change logging removed
          updateWindowTitle();
          break;
        case SDLK_PLUS:
        case SDLK_EQUALS:
          changeResolution(true);
          break;
        case SDLK_MINUS:
        case SDLK_UNDERSCORE:
          changeResolution(false);
          break;
      }
    } else if (e.type == SDL_WINDOWEVENT) {
      if ((e.window.event == SDL_WINDOWEVENT_RESIZED || 
           e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) && !isResizing) {
        handleWindowResize(e.window.data1, e.window.data2);
      }
    }
  }
}

void Application::update(double deltaTime) {
  // Always update, even if deltaTime is small
  // This ensures camera and animation state stays current
  const Uint8 *keyStates = SDL_GetKeyboardState(nullptr);
  
  // Update camera - this must happen every frame
  // Even in manual mode with no input, camera look direction needs updating
  cinematicCamera->update(deltaTime, keyStates);
}

void Application::render(double elapsedTime) {
  // Always render, regardless of input state or mode
  // This ensures animation continues even in manual mode when idle
  
  // Prepare camera data for GPU
  CameraData gpuCam;
  prepareCameraData(gpuCam);

  // Render with Metal GPU - elapsedTime always advances
  // The black hole animation is driven by elapsedTime, not camera movement
  // Force render every frame, even if camera doesn't move
  float renderTime = static_cast<float>(elapsedTime);
  
  // Debug logging removed for performance
  
  metal_rt_renderer_render(gpuRenderer, &gpuCam, renderTime);
  const void *pixels = metal_rt_renderer_get_pixels(gpuRenderer);
  
  if (pixels && gpuTexture) {
    // Always update texture - force update even if pixels appear unchanged
    // This ensures animation continues even when camera is stationary
    // Explicitly update the entire texture region
    SDL_Rect updateRect = {0, 0, windowWidth, windowHeight};
    int result = SDL_UpdateTexture(gpuTexture, &updateRect, pixels, windowWidth * 4);
    if (result != 0) {
      static int updateErrorCount = 0;
      if (updateErrorCount++ < 3) {
        std::cerr << "SDL_UpdateTexture error: " << SDL_GetError() << std::endl;
      }
    }
    
    // Clear renderer before copying to ensure fresh frame
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, gpuTexture, nullptr, nullptr);
    
    // Force renderer to flush commands
    SDL_RenderFlush(sdlRenderer);
  } else {
    // If rendering fails, log error but continue loop
    static int errorCount = 0;
    if (errorCount++ < 5) {
      std::cerr << "Warning: Render failed - pixels: " << (pixels ? "OK" : "NULL") 
                << ", texture: " << (gpuTexture ? "OK" : "NULL") << std::endl;
    }
  }

  // Render HUD
  hud->renderHints(hud->areHintsVisible(), cinematicCamera->getMode(), currentFPS, windowWidth, windowHeight);

  // Always present - this must happen every frame
  // Force presentation even if SDL thinks nothing changed
  SDL_RenderPresent(sdlRenderer);
  
  // Force window update on macOS - prevent throttling
  #ifdef __APPLE__
  // Process events to keep window active and prevent macOS throttling
  SDL_PumpEvents();
  #endif
}

void Application::prepareCameraData(CameraData &data) {
  // Always ensure camera data is valid, even when switching modes
  data.position[0] = camera->position.x;
  data.position[1] = camera->position.y;
  data.position[2] = camera->position.z;
  
  // Ensure forward vector is valid (non-zero)
  float forwardLen = std::sqrt(camera->forward.x * camera->forward.x + 
                                camera->forward.y * camera->forward.y + 
                                camera->forward.z * camera->forward.z);
  if (forwardLen < 0.001f) {
    // Fallback: look at black hole center
    camera->lookAt(Vector3(0, 0, 0));
  }
  
  data.forward[0] = camera->forward.x;
  data.forward[1] = camera->forward.y;
  data.forward[2] = camera->forward.z;
  data.right[0] = camera->right.x;
  data.right[1] = camera->right.y;
  data.right[2] = camera->right.z;
  data.up[0] = camera->up.x;
  data.up[1] = camera->up.y;
  data.up[2] = camera->up.z;
  data.fov = camera->fov;
}

void Application::toggleFullscreen() {
  isFullscreen = !isFullscreen;
  SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  
  // Get new window size after fullscreen toggle
  SDL_GetWindowSize(window, &windowWidth, &windowHeight);
  handleWindowResize(windowWidth, windowHeight);
  
  // Fullscreen toggle logging removed
}

void Application::handleWindowResize(int width, int height) {
  if (width == windowWidth && height == windowHeight) {
    return; // No change
  }
  
  windowWidth = width;
  windowHeight = height;
  
  // Window resize logging removed
  recreateRenderTargets();
}

void Application::recreateRenderTargets() {
  // Ensure we have the actual window size (accounting for DPI scaling)
  int actualWidth, actualHeight;
  SDL_GetWindowSize(window, &actualWidth, &actualHeight);
  windowWidth = actualWidth;
  windowHeight = actualHeight;
  
  // Resize Metal renderer first (this updates internal resolution)
  metal_rt_renderer_resize(gpuRenderer, windowWidth, windowHeight);
  
  // Recreate SDL texture with new dimensions
  if (gpuTexture) {
    SDL_DestroyTexture(gpuTexture);
    gpuTexture = nullptr;
  }
  
  gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
  
  if (!gpuTexture) {
    std::cerr << "Failed to recreate texture at " << windowWidth << "×" << windowHeight << std::endl;
  } else {
    // Render target recreation logging removed
  }
  
  updateWindowTitle();
}

void Application::updateWindowTitle() {
  std::string title = "Black Hole Simulation - " +
                      std::string(cinematicCamera->getModeName()) +
                      " - " + std::to_string(windowWidth) + "×" + std::to_string(windowHeight) +
                      " - FPS: " + std::to_string(currentFPS);
  SDL_SetWindowTitle(window, title.c_str());
}


void Application::changeResolution(bool increase) {
  if (increase) {
    resolutionManager->next();
  } else {
    resolutionManager->previous();
  }
  
  const Resolution& res = resolutionManager->getCurrent();
  
  int newWidth, newHeight;
  if (res.width == 0 && res.height == 0) {
    // Native resolution
    SDL_DisplayMode displayMode;
    if (SDL_GetDesktopDisplayMode(0, &displayMode) == 0) {
      newWidth = displayMode.w;
      newHeight = displayMode.h;
    } else {
      newWidth = 1920;
      newHeight = 1080;
    }
  } else {
    newWidth = res.width;
    newHeight = res.height;
  }
  
  // Skip if same resolution
  if (newWidth == windowWidth && newHeight == windowHeight) {
    return;
  }
  
  // Set flag to prevent recursive resize events
  isResizing = true;
  
  // Resize window first
  SDL_SetWindowSize(window, newWidth, newHeight);
  
  // Process events to ensure window resize completes
  SDL_PumpEvents();
  SDL_FlushEvents(SDL_WINDOWEVENT, SDL_WINDOWEVENT);
  
  // Get actual window size (may differ due to high DPI scaling)
  int actualWidth, actualHeight;
  SDL_GetWindowSize(window, &actualWidth, &actualHeight);
  
  // Update to actual size
  windowWidth = actualWidth;
  windowHeight = actualHeight;
  
  // Force immediate resize of render targets
  recreateRenderTargets();
  
  // Clear flag
  isResizing = false;
}

void Application::cleanup() {
  if (gpuRenderer)
    metal_rt_renderer_destroy(gpuRenderer);
  if (gpuTexture)
    SDL_DestroyTexture(gpuTexture);
  if (font)
    TTF_CloseFont(font);
  if (sdlRenderer)
    SDL_DestroyRenderer(sdlRenderer);
  if (window)
    SDL_DestroyWindow(window);
  
  delete resolutionManager;
  delete hud;
  delete cinematicCamera;
  delete camera;
  delete blackHole;
  
  TTF_Quit();
  SDL_Quit();
}

