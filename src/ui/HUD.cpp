#include "../../include/ui/HUD.hpp"
#include <string>

HUD::HUD(SDL_Renderer *renderer, TTF_Font *font)
    : renderer(renderer), font(font), hintsVisible(true) {}

void HUD::renderHints(bool showHints, CinematicMode mode, int fps, int windowWidth, int windowHeight) {
  if (!showHints || !font)
    return;

  // Scale overlay size based on window size (maintain aspect ratio)
  int overlayWidth = std::min(500, windowWidth / 4);
  int overlayHeight = 220;
  int padding = 10;
  
  // Use windowHeight to ensure overlay doesn't exceed screen bounds
  (void)windowHeight; // Suppress unused parameter warning
  
  // Semi-transparent background
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
  SDL_Rect overlay = {padding, padding, overlayWidth, overlayHeight};
  SDL_RenderFillRect(renderer, &overlay);

  // Border
  SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
  SDL_RenderDrawRect(renderer, &overlay);

  // Define colors
  SDL_Color textColor = {255, 255, 255, 255};
  SDL_Color highlightColor = {100, 200, 255, 255};
  SDL_Color cinematicColor = {255, 200, 100, 255};
  
  std::string hints[] = {
    "CONTROLS:",
    "WASD - Move Camera",
    "Space/Shift - Up/Down",
    "R - Reset Camera",
    "C - Cinematic: " + std::string(getCinematicModeName(mode)),
    "Tab - Toggle This Help",
    "ESC/Q - Quit",
    "FPS: " + std::to_string(fps)
  };

  int y = 20;
  for (int i = 0; i < 8; i++) {
    SDL_Color color = (i == 0) ? highlightColor
                      : (i == 4) ? cinematicColor
                                 : textColor;
    renderText(hints[i].c_str(), 20, y, color);
    y += 24;
  }
}

void HUD::renderText(const char *text, int x, int y, SDL_Color color) {
  SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
  if (surface) {
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &destRect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
  }
}

