#include "../../include/core/Application.hpp"
#include <iostream>
#include <chrono>
#include <string>

Application::Application()
    : window(nullptr), sdlRenderer(nullptr), font(nullptr),
      gpuRenderer(nullptr), gpuTexture(nullptr),
      blackHole(nullptr), camera(nullptr), cinematicCamera(nullptr), hud(nullptr),
      windowWidth(1920), windowHeight(1080), isFullscreen(false),
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

  // Initialize SDL_ttf
  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
    return false;
  }

  // Create window (resizable)
  window = SDL_CreateWindow("Black Hole Simulation | GPU | Smooth Orbit",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             windowWidth, windowHeight,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }
  
  // Get actual window size (may differ due to high DPI)
  SDL_GetWindowSize(window, &windowWidth, &windowHeight);

  // Create SDL renderer
  sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!sdlRenderer) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return false;
  }

  // Load font
  font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);
  if (!font) {
    std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
  }

  // Initialize GPU renderer (Metal)
  gpuRenderer = metal_rt_renderer_create(windowWidth, windowHeight);
  if (!gpuRenderer) {
    std::cerr << "ERROR: GPU renderer failed to initialize!\n";
    std::cerr << "Metal GPU acceleration is required for this simulation.\n";
    return false;
  }

  gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
  
  std::cout << "\n=== BLACK HOLE SIMULATION | GPU ACCELERATED ===\n";
  std::cout << "Resolution: " << windowWidth << " × " << windowHeight << "\n";
  std::cout << "F          - Toggle fullscreen\n";
  std::cout << "C          - Cycle cinematic modes\n";
  std::cout << "WASD       - Move camera (manual mode)\n";
  std::cout << "Space      - Move camera up\n";
  std::cout << "Shift      - Move camera down\n";
  std::cout << "R          - Reset camera position\n";
  std::cout << "Tab        - Toggle hints overlay\n";
  std::cout << "ESC/Q      - Quit\n";
  std::cout << "===========================================\n\n";
  std::cout << "Metal GPU renderer initialized successfully\n";

  // Initialize simulation components
  blackHole = new BlackHole(1.0);
  Vector3 initialPos(0, 3, -15);
  camera = new Camera(initialPos, Vector3(0, 0, 0), 60.0);
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
    lastTime = currentTime;
    double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();

    handleEvents();
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
          std::cout << "Camera reset" << std::endl;
          break;
        case SDLK_TAB:
          hud->toggleHints();
          std::cout << "Hints: " << (hud->areHintsVisible() ? "ON" : "OFF") << std::endl;
          break;
        case SDLK_c:
          cinematicCamera->cycleMode();
          std::cout << "Cinematic Mode: " << cinematicCamera->getModeName() << std::endl;
          updateWindowTitle();
          break;
      }
    } else if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_RESIZED || 
          e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        handleWindowResize(e.window.data1, e.window.data2);
      }
    }
  }
}

void Application::update(double deltaTime) {
  const Uint8 *keyStates = SDL_GetKeyboardState(nullptr);
  cinematicCamera->update(deltaTime, keyStates);
}

void Application::render(double elapsedTime) {
  SDL_RenderClear(sdlRenderer);

  // Prepare camera data for GPU
  CameraData gpuCam;
  prepareCameraData(gpuCam);

  // Render with Metal GPU
  metal_rt_renderer_render(gpuRenderer, &gpuCam, elapsedTime);
  const void *pixels = metal_rt_renderer_get_pixels(gpuRenderer);
  SDL_UpdateTexture(gpuTexture, nullptr, pixels, windowWidth * 4);
  SDL_RenderCopy(sdlRenderer, gpuTexture, nullptr, nullptr);

  // Render HUD
  hud->renderHints(hud->areHintsVisible(), cinematicCamera->getMode(), currentFPS, windowWidth, windowHeight);

  SDL_RenderPresent(sdlRenderer);
}

void Application::prepareCameraData(CameraData &data) {
  data.position[0] = camera->position.x;
  data.position[1] = camera->position.y;
  data.position[2] = camera->position.z;
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
  
  std::cout << "Fullscreen: " << (isFullscreen ? "ON" : "OFF") << std::endl;
}

void Application::handleWindowResize(int width, int height) {
  if (width == windowWidth && height == windowHeight) {
    return; // No change
  }
  
  windowWidth = width;
  windowHeight = height;
  
  std::cout << "Window resized to: " << width << " × " << height << std::endl;
  recreateRenderTargets();
}

void Application::recreateRenderTargets() {
  // Resize Metal renderer
  metal_rt_renderer_resize(gpuRenderer, windowWidth, windowHeight);
  
  // Recreate SDL texture
  if (gpuTexture) {
    SDL_DestroyTexture(gpuTexture);
  }
  gpuTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
  
  updateWindowTitle();
}

void Application::updateWindowTitle() {
  std::string title = "Black Hole Simulation | GPU | " +
                      std::string(cinematicCamera->getModeName()) +
                      " | " + std::to_string(windowWidth) + "×" + std::to_string(windowHeight) +
                      " | FPS: " + std::to_string(currentFPS);
  SDL_SetWindowTitle(window, title.c_str());
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
  
  delete hud;
  delete cinematicCamera;
  delete camera;
  delete blackHole;
  
  TTF_Quit();
  SDL_Quit();
}

