#include "../../include/core/Application.hpp"
#include "../../include/utils/SaveDialog.h"
#include "../../include/utils/IconLoader.h"
#include <iostream>
#include <chrono>
#include <string>
#include <ctime>
#include <algorithm>

Application::Application()
    : window(nullptr), sdlRenderer(nullptr), font(nullptr),
      gpuRenderer(nullptr), gpuTexture(nullptr),
      blackHole(nullptr), camera(nullptr), cinematicCamera(nullptr), hud(nullptr),
      resolutionManager(nullptr), videoRecorder(nullptr),
      windowWidth(1920), windowHeight(1080), renderWidth(1920), renderHeight(1080),
      isFullscreen(false), isResizing(false),
      running(false), currentFPS(0), isRecording(false) {}

Application::~Application() {
  cleanup();
}

bool Application::initialize() {
  // Initialize SDL
  std::cerr << "[INIT] Initializing SDL..." << std::endl;
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "[ERROR] SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }
  std::cerr << "[OK] SDL initialized successfully" << std::endl;

  // Initialize resolution manager (loads saved resolution or defaults to 1080p)
  std::cerr << "[INIT] Creating resolution manager..." << std::endl;
  resolutionManager = new ResolutionManager();
  
  // Get selected resolution for rendering
  const Resolution& res = resolutionManager->getCurrent();
  renderWidth = res.width;
  renderHeight = res.height;
  std::cerr << "[OK] Resolution manager initialized: " << renderWidth << "x" << renderHeight << std::endl;
  
  // Window size - use a reasonable default that matches common screen sizes
  // This will be the display size, rendering resolution is separate
  windowWidth = 1920;
  windowHeight = 1080;

  // Initialize SDL_ttf
  std::cerr << "[INIT] Initializing SDL_ttf..." << std::endl;
  if (TTF_Init() < 0) {
    std::cerr << "[ERROR] SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
    return false;
  }
  std::cerr << "[OK] SDL_ttf initialized successfully" << std::endl;

  // Create window (resizable)
  std::cerr << "[INIT] Creating window (" << windowWidth << "x" << windowHeight << ")..." << std::endl;
  window = SDL_CreateWindow("Black Hole Simulation | Smooth Orbit",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             windowWidth, windowHeight,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "[ERROR] Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }
  std::cerr << "[OK] Window created successfully" << std::endl;
  
  // Load and set window icon (path relative to Resources directory in bundle)
  loadWindowIcon(window, "assets/export/iOS-Default-1024x1024@1x.png");
  
  // Get actual window size (may differ due to high DPI)
  int actualWidth, actualHeight;
  SDL_GetWindowSize(window, &actualWidth, &actualHeight);
  windowWidth = actualWidth;
  windowHeight = actualHeight;

  // Create SDL renderer (without VSYNC to prevent pausing when idle)
  std::cerr << "[INIT] Creating SDL renderer..." << std::endl;
  sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!sdlRenderer) {
    std::cerr << "[ERROR] Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }
  std::cerr << "[OK] SDL renderer created successfully" << std::endl;
  
  // Set default render draw color to black
  SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
  
  // On macOS with high DPI, get the renderer output size
  // which accounts for the scale factor (may be 2x on Retina displays)
  int rendererOutputW, rendererOutputH;
  SDL_GetRendererOutputSize(sdlRenderer, &rendererOutputW, &rendererOutputH);
  
  // Don't set logical size - use physical window coordinates directly
  // Reset any scaling to 1:1
  SDL_RenderSetScale(sdlRenderer, 1.0f, 1.0f);
  
  // Set viewport to full renderer output (not window size, which might be in points)
  SDL_Rect fullViewport = {0, 0, rendererOutputW, rendererOutputH};
  SDL_RenderSetViewport(sdlRenderer, &fullViewport);

  // Load font with larger size for better readability
  font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 24);
  if (!font) {
    std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
  }

  // Initialize GPU renderer (Metal) with rendering resolution
  std::cerr << "[INIT] Initializing Metal renderer at " << renderWidth << "x" << renderHeight << "..." << std::endl;
  gpuRenderer = metal_rt_renderer_create(renderWidth, renderHeight);
  if (!gpuRenderer) {
    std::cerr << "[ERROR] GPU renderer failed to initialize!" << std::endl;
    std::cerr << "[ERROR] Metal GPU acceleration is required for this simulation." << std::endl;
    return false;
  }
  std::cerr << "[OK] Metal renderer initialized successfully" << std::endl;

  gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
  
  // Startup messages removed - use HUD instead

  // Initialize simulation components
  blackHole = new BlackHole(1.0);
  // Camera starts further back, looking at black hole center (0, 0, 0)
  Vector3 initialPos(0, 3, -20); // Increased distance from -15 to -20 for better initial view
  camera = new Camera(initialPos, Vector3(0, 0, 0), 60.0); // Points at black hole center
  cinematicCamera = new CinematicCamera(*camera, initialPos);
  
  // Ensure camera is properly initialized to look at black hole center
  // Force update camera look direction to establish correct initial orientation
  camera->lookAt(Vector3(0, 0, 0));
  hud = new HUD(sdlRenderer, font);
  videoRecorder = new VideoRecorder();

  running = true;
  std::cerr << "Application initialization complete, entering main loop" << std::endl;
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

        // FPS calculation - measure actual rendering performance
        // Use a timer that measures the actual render time, not just frame count
        static auto lastFPSTime = std::chrono::high_resolution_clock::now();
        auto currentFPSTime = std::chrono::high_resolution_clock::now();
        double fpsDeltaTime = std::chrono::duration<double>(currentFPSTime - lastFPSTime).count();
        
        frameCount++;
        fpsUpdateTime += deltaTime;
        
        if (fpsUpdateTime >= 0.5) {
          // Calculate FPS based on actual time elapsed
          if (fpsDeltaTime > 0.0) {
            currentFPS = static_cast<int>(frameCount / fpsUpdateTime);
          }
          frameCount = 0;
          fpsUpdateTime = 0.0;
          lastFPSTime = currentFPSTime;
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
      // Check for Command+R (start recording)
      if (e.key.keysym.sym == SDLK_r && (e.key.keysym.mod & KMOD_GUI)) {
        if (!isRecording) {
          std::cout << "Command+R pressed - starting recording..." << std::endl;
          startRecording();
        } else {
          std::cout << "Already recording!" << std::endl;
        }
        break;
      }
      
      // When recording, Esc and Q should stop recording instead of quitting
      if (isRecording) {
        if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q) {
          stopRecording();
          break;
        }
        // Enter also stops recording
        if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
          stopRecording();
          break;
        }
      }
      
      switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
          // Only handle ESC for quitting if not recording (handled above)
          if (isFullscreen) {
            toggleFullscreen(); // Exit fullscreen on ESC
          } else {
            running = false; // Quit if windowed
          }
          break;
        case SDLK_q:
          // Only quit if not recording (handled above)
          running = false;
          break;
        case SDLK_f:
          toggleFullscreen();
          break;
        case SDLK_r:
          // Only reset camera if Command is not pressed (Command+R is handled above)
          if (!(e.key.keysym.mod & KMOD_GUI)) {
            cinematicCamera->reset();
          }
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
  
  // Capture frame for video recording if recording (before texture update)
  if (isRecording && videoRecorder && pixels) {
    videoRecorder->addFrame(pixels, renderWidth, renderHeight);
  }
  
  if (pixels && gpuTexture) {
    // Always update texture - force update even if pixels appear unchanged
    // This ensures animation continues even when camera is stationary
    // Explicitly update the entire texture region (at rendering resolution)
    SDL_Rect updateRect = {0, 0, renderWidth, renderHeight};
    int result = SDL_UpdateTexture(gpuTexture, &updateRect, pixels, renderWidth * 4);
    if (result != 0) {
      static int updateErrorCount = 0;
      if (updateErrorCount++ < 3) {
        std::cerr << "SDL_UpdateTexture error: " << SDL_GetError() << std::endl;
      }
    }
    
    // Clear renderer with black before copying to ensure fresh frame
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(sdlRenderer);
    
    // Reset all renderer state to defaults
    SDL_RenderSetViewport(sdlRenderer, nullptr); // Reset viewport to full renderer output
    SDL_RenderSetScale(sdlRenderer, 1.0f, 1.0f); // Ensure 1:1 scale
    
    // Get current renderer output size (accounts for high DPI scaling)
    int currentOutputW, currentOutputH;
    SDL_GetRendererOutputSize(sdlRenderer, &currentOutputW, &currentOutputH);
    
    // Calculate aspect ratios to maintain proper aspect ratio (letterbox/pillarbox)
    float renderAspect = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);
    float outputAspect = static_cast<float>(currentOutputW) / static_cast<float>(currentOutputH);
    
    SDL_Rect srcRect = {0, 0, renderWidth, renderHeight}; // Full source texture
    SDL_Rect dstRect;
    
    if (renderAspect > outputAspect) {
      // Source is wider - letterbox (black bars top/bottom)
      int scaledHeight = static_cast<int>(currentOutputW / renderAspect);
      int offsetY = (currentOutputH - scaledHeight) / 2;
      dstRect = {0, offsetY, currentOutputW, scaledHeight};
    } else {
      // Source is taller - pillarbox (black bars left/right)
      int scaledWidth = static_cast<int>(currentOutputH * renderAspect);
      int offsetX = (currentOutputW - scaledWidth) / 2;
      dstRect = {offsetX, 0, scaledWidth, currentOutputH};
    }
    
    // Copy texture maintaining aspect ratio
    int copyResult = SDL_RenderCopy(sdlRenderer, gpuTexture, &srcRect, &dstRect);
    if (copyResult != 0) {
      static int errorCount = 0;
      if (errorCount++ < 3) {
        std::cerr << "SDL_RenderCopy error: " << SDL_GetError() << std::endl;
      }
    }
    
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

  // Reset viewport to full window for HUD rendering
  SDL_RenderSetViewport(sdlRenderer, nullptr); // Use full renderer output
  SDL_RenderSetScale(sdlRenderer, 1.0f, 1.0f); // Ensure 1:1 scale for HUD
  
  // Render HUD (hide hints if recording)
  bool showHints = hud->areHintsVisible() && !isRecording;
  hud->renderHints(showHints, cinematicCamera->getMode(), currentFPS, windowWidth, windowHeight, resolutionManager);

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
  // Don't call lookAt here as it resets user rotations - let CinematicCamera handle orientation
  float forwardLen = std::sqrt(camera->forward.x * camera->forward.x + 
                                camera->forward.y * camera->forward.y + 
                                camera->forward.z * camera->forward.z);
  if (forwardLen < 0.001f) {
    // Only set a default forward if completely invalid - don't reset user rotations
    // This should rarely happen as CinematicCamera maintains valid vectors
    data.forward[0] = 0.0f;
    data.forward[1] = 0.0f;
    data.forward[2] = 1.0f; // Default forward
    data.right[0] = 1.0f;
    data.right[1] = 0.0f;
    data.right[2] = 0.0f;
    data.up[0] = 0.0f;
    data.up[1] = 1.0f;
    data.up[2] = 0.0f;
  } else {
    // Normalize and use camera vectors
    float invLen = 1.0f / forwardLen;
    data.forward[0] = camera->forward.x * invLen;
    data.forward[1] = camera->forward.y * invLen;
    data.forward[2] = camera->forward.z * invLen;
    
    // Normalize right and up vectors too
    float rightLen = std::sqrt(camera->right.x * camera->right.x + 
                               camera->right.y * camera->right.y + 
                               camera->right.z * camera->right.z);
    float upLen = std::sqrt(camera->up.x * camera->up.x + 
                            camera->up.y * camera->up.y + 
                            camera->up.z * camera->up.z);
    if (rightLen > 0.001f) {
      float invRightLen = 1.0f / rightLen;
      data.right[0] = camera->right.x * invRightLen;
      data.right[1] = camera->right.y * invRightLen;
      data.right[2] = camera->right.z * invRightLen;
    } else {
      data.right[0] = 1.0f;
      data.right[1] = 0.0f;
      data.right[2] = 0.0f;
    }
    if (upLen > 0.001f) {
      float invUpLen = 1.0f / upLen;
      data.up[0] = camera->up.x * invUpLen;
      data.up[1] = camera->up.y * invUpLen;
      data.up[2] = camera->up.z * invUpLen;
    } else {
      data.up[0] = 0.0f;
      data.up[1] = 1.0f;
      data.up[2] = 0.0f;
    }
  }
  data.fov = camera->fov;
}

void Application::toggleFullscreen() {
  isFullscreen = !isFullscreen;
  SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  
  // Get new window size after fullscreen toggle
  SDL_GetWindowSize(window, &windowWidth, &windowHeight);
  
  // In fullscreen, window size matches native display resolution
  // Rendering resolution is still controlled by resolution manager
  // This allows changing quality without changing window size
  // Note: Resolution is preserved when toggling fullscreen
  
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
  // Update window size (for display scaling)
  int actualWidth, actualHeight;
  SDL_GetWindowSize(window, &actualWidth, &actualHeight);
  windowWidth = actualWidth;
  windowHeight = actualHeight;
  
  // Resize Metal renderer to rendering resolution (not window size)
  metal_rt_renderer_resize(gpuRenderer, renderWidth, renderHeight);
  
  // Recreate SDL texture with rendering resolution
  if (gpuTexture) {
    SDL_DestroyTexture(gpuTexture);
    gpuTexture = nullptr;
  }
  
  gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
  
  if (!gpuTexture) {
    std::cerr << "Failed to recreate texture at " << renderWidth << "×" << renderHeight << std::endl;
  }
  
  updateWindowTitle();
}

void Application::updateWindowTitle() {
  std::string title = "Black Hole Simulation - " +
                      std::string(cinematicCamera->getModeName()) +
                      " - FPS: " + std::to_string(currentFPS);
  if (isRecording) {
    title = "🔴 " + title;
  }
  SDL_SetWindowTitle(window, title.c_str());
}


void Application::changeResolution(bool increase) {
  // Don't allow resolution change during recording
  if (isRecording) {
    return;
  }
  
  if (increase) {
    resolutionManager->next();
  } else {
    resolutionManager->previous();
  }
  
  const Resolution& res = resolutionManager->getCurrent();
  
  // Calculate new rendering resolution (window size stays the same)
  int newRenderWidth = res.width;
  int newRenderHeight = res.height;
  
  // Skip if same rendering resolution
  if (newRenderWidth == renderWidth && newRenderHeight == renderHeight) {
    return;
  }
  
  // Update rendering resolution (window size stays unchanged)
  renderWidth = newRenderWidth;
  renderHeight = newRenderHeight;
  
  // Save resolution preference
  resolutionManager->saveResolution();
  
  // Recreate render targets with new resolution
  recreateRenderTargets();
}

void Application::startRecording() {
  if (isRecording || !videoRecorder) {
    return;
  }
  
  // Get formatted resolution name (e.g., "1080p", "4K", etc.)
  std::string resolutionName;
  if (resolutionManager) {
    const Resolution& res = resolutionManager->getCurrent();
    if (res.name && res.name[0] != '\0') {
      std::string name = res.name;
      // Format common resolution names
      if (name.find("4K") != std::string::npos || name.find("2160p") != std::string::npos) {
        resolutionName = "4K";
      } else if (name.find("1080p") != std::string::npos) {
        resolutionName = "1080p";
      } else if (name.find("1440p") != std::string::npos || name.find("QHD") != std::string::npos) {
        resolutionName = "1440p";
      } else if (name.find("720p") != std::string::npos) {
        resolutionName = "720p";
      } else if (name.find("5K") != std::string::npos) {
        resolutionName = "5K";
      } else if (name.find("8K") != std::string::npos) {
        resolutionName = "8K";
      } else {
        // Use the name as-is, but clean it up (remove spaces, etc.)
        resolutionName = name;
        // Replace spaces with underscores for filename
        std::replace(resolutionName.begin(), resolutionName.end(), ' ', '_');
      }
    } else {
      // Fallback to dimensions if no name
      resolutionName = std::to_string(renderWidth) + "x" + std::to_string(renderHeight);
    }
  } else {
    // Fallback to dimensions
    resolutionName = std::to_string(renderWidth) + "x" + std::to_string(renderHeight);
  }
  
  // Generate filename with formatted resolution and timestamp
  std::time_t now = std::time(nullptr);
  std::tm* tm = std::localtime(&now);
  char filename[256];
  std::snprintf(filename, sizeof(filename), "blackhole_recording_%s_%04d%02d%02d_%02d%02d%02d.mp4",
                resolutionName.c_str(),
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec);
  
  if (videoRecorder->startRecording(filename, renderWidth, renderHeight, currentFPS > 0 ? currentFPS : 60)) {
    isRecording = true;
    updateWindowTitle();
    std::cout << "Recording started at " << renderWidth << "×" << renderHeight << std::endl;
  } else {
    std::cerr << "Failed to start recording!" << std::endl;
  }
}

void Application::stopRecording() {
  if (!isRecording || !videoRecorder) {
    return;
  }
  
  // Stop recording first
  videoRecorder->stopRecording();
  std::string tempFilename = videoRecorder->getFilename();
  isRecording = false;
  updateWindowTitle();
  
  // Show save dialog
  std::string savePath = showSaveDialog(tempFilename);
  
  if (!savePath.empty()) {
    // Move file to user-selected location
    if (videoRecorder->moveFile(savePath)) {
      std::cout << "Recording saved to: " << savePath << std::endl;
    } else {
      std::cerr << "Failed to move recording to: " << savePath << std::endl;
      std::cerr << "Original file is still at: " << tempFilename << std::endl;
    }
  } else {
    // User cancelled - keep file in original location
    std::cout << "Recording saved to: " << tempFilename << std::endl;
  }
}

void Application::cleanup() {
  // Stop recording if active
  if (isRecording) {
    stopRecording();
  }
  
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
  
  delete videoRecorder;
  delete resolutionManager;
  delete hud;
  delete cinematicCamera;
  delete camera;
  delete blackHole;
  
  TTF_Quit();
  SDL_Quit();
}

