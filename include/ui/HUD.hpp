#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../camera/CinematicCamera.hpp"
#include "../camera/Camera.hpp"

/**
 * Heads-Up Display for rendering on-screen information
 */
class HUD {
public:
  HUD(SDL_Renderer *renderer, TTF_Font *font);
  
  // Render the hints overlay
  void renderHints(bool showHints, CinematicMode mode, int fps, int windowWidth, int windowHeight);
  
  // Render camera axis indicators (for debugging rotation)
  void renderCameraAxes(const Camera *camera, int windowWidth, int windowHeight);
  
  // Toggle hints visibility
  void toggleHints() { hintsVisible = !hintsVisible; }
  
  // Check if hints are visible
  bool areHintsVisible() const { return hintsVisible; }

private:
  SDL_Renderer *renderer;
  TTF_Font *font;
  bool hintsVisible;
  
  // Render a single text line
  void renderText(const char *text, int x, int y, SDL_Color color);
};

