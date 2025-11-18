#include "../../include/ui/HUD.hpp"
#include "../../include/utils/Vector3.hpp"
#include <string>
#include <cmath>

HUD::HUD(SDL_Renderer *renderer, TTF_Font *font)
    : renderer(renderer), font(font), hintsVisible(true) {}

void HUD::renderHints(bool showHints, CinematicMode mode, int fps, int windowWidth, int windowHeight) {
  if (!showHints || !font)
    return;

  // Define hints array first to calculate content width
  std::string hints[] = {
    "CONTROLS",
    "L/J - Rotate Up Axis",
    "I/K - Rotate Right Axis",
    "O/U - Rotate Forward Axis",
    "WASD - Move",
    "Space/Shift - Up/Down",
    "R - Reset",
    "C - Mode: " + std::string(getCinematicModeName(mode)),
    "+/- - Resolution",
    "F - Fullscreen",
    "Tab - Toggle Help",
    "ESC/Q - Quit",
    "FPS: " + std::to_string(fps)
  };

  // Calculate maximum text width to make overlay responsive
  int maxTextWidth = 0;
  int lineHeight = 20;
  int padding = 12;
  int textPadding = 16;
  
  for (int i = 0; i < 13; i++) {
    SDL_Surface *testSurface = TTF_RenderText_Blended(font, hints[i].c_str(), {255, 255, 255, 255});
    if (testSurface) {
      maxTextWidth = std::max(maxTextWidth, static_cast<int>(testSurface->w));
      SDL_FreeSurface(testSurface);
    }
  }
  
  // Calculate overlay dimensions based on content
  int overlayWidth = maxTextWidth + (textPadding * 2);
  int overlayHeight = (13 * lineHeight) + (textPadding * 2);
  
  // Ensure overlay doesn't exceed window bounds
  overlayWidth = std::min(overlayWidth, windowWidth - (padding * 2));
  overlayHeight = std::min(overlayHeight, windowHeight - (padding * 2));
  
  // Position at bottom left
  int overlayX = padding;
  int overlayY = windowHeight - overlayHeight - padding;
  
  // Modern semi-transparent background with rounded corners effect
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 15, 15, 25, 220); // Dark blue-gray background
  SDL_Rect overlay = {overlayX, overlayY, overlayWidth, overlayHeight};
  SDL_RenderFillRect(renderer, &overlay);

  // Modern border (subtle gradient effect with two borders)
  SDL_SetRenderDrawColor(renderer, 60, 100, 180, 180); // Outer border
  SDL_RenderDrawRect(renderer, &overlay);
  
  // Inner border for depth
  SDL_Rect innerBorder = {overlayX + 1, overlayY + 1, overlayWidth - 2, overlayHeight - 2};
  SDL_SetRenderDrawColor(renderer, 100, 150, 255, 120);
  SDL_RenderDrawRect(renderer, &innerBorder);

  // Define modern colors
  SDL_Color textColor = {220, 220, 230, 255}; // Soft white
  SDL_Color highlightColor = {100, 200, 255, 255}; // Bright cyan-blue
  SDL_Color cinematicColor = {255, 180, 80, 255}; // Warm orange
  SDL_Color fpsColor = {150, 255, 150, 255}; // Light green for FPS

  // Render text with proper positioning
  int y = overlayY + textPadding;
  for (int i = 0; i < 13; i++) {
    SDL_Color color = (i == 0) ? highlightColor
                      : (i == 7) ? cinematicColor
                      : (i == 12) ? fpsColor
                                 : textColor;
    
    // Ensure text doesn't overflow - clip if necessary
    int textX = overlayX + textPadding;
    if (textX + maxTextWidth > windowWidth - padding) {
      textX = windowWidth - maxTextWidth - padding;
    }
    
    renderText(hints[i].c_str(), textX, y, color);
    y += lineHeight;
  }
}

void HUD::renderText(const char *text, int x, int y, SDL_Color color) {
  if (!text || !font)
    return;
    
  SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
  if (surface) {
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
      SDL_Rect destRect = {x, y, surface->w, surface->h};
      SDL_RenderCopy(renderer, texture, nullptr, &destRect);
      SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
  }
}

void HUD::renderCameraAxes(const Camera *camera, int windowWidth, int windowHeight) {
  if (!camera || !renderer)
    return;
  
  // Position in bottom-right corner
  int centerX = windowWidth - 150;
  int centerY = windowHeight - 150;
  float axisLength = 50.0f; // Length of axis lines in pixels
  
  // Project 3D vectors to 2D screen space (simple orthographic projection)
  // We'll show them as if looking from above (Y-up view)
  auto projectToScreen = [&](const Vector3& vec) -> std::pair<int, int> {
    // Simple projection: X->screen X, Z->screen Y (Y is up, so we ignore it for 2D view)
    int screenX = centerX + static_cast<int>(vec.x * axisLength);
    int screenY = centerY - static_cast<int>(vec.z * axisLength); // Negative Z goes down on screen
    return {screenX, screenY};
  };
  
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  
  // Draw Forward vector (RED) - camera's forward direction
  auto forwardEnd = projectToScreen(camera->forward);
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
  SDL_RenderDrawLine(renderer, centerX, centerY, forwardEnd.first, forwardEnd.second);
  renderText("F", forwardEnd.first + 5, forwardEnd.second - 10, {255, 0, 0, 255});
  
  // Draw Right vector (GREEN) - camera's right direction
  auto rightEnd = projectToScreen(camera->right);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
  SDL_RenderDrawLine(renderer, centerX, centerY, rightEnd.first, rightEnd.second);
  renderText("R", rightEnd.first + 5, rightEnd.second - 10, {0, 255, 0, 255});
  
  // Draw Up vector (BLUE) - camera's up direction
  auto upEnd = projectToScreen(camera->up);
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue
  SDL_RenderDrawLine(renderer, centerX, centerY, upEnd.first, upEnd.second);
  renderText("U", upEnd.first + 5, upEnd.second - 10, {0, 0, 255, 255});
  
  // Draw center point
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_Rect center = {centerX - 2, centerY - 2, 4, 4};
  SDL_RenderFillRect(renderer, &center);
  
  // Draw legend
  int legendY = windowHeight - 120;
  renderText("Axes:", centerX - 40, legendY, {255, 255, 255, 255});
  renderText("F=Forward (Red)", centerX - 40, legendY + 20, {255, 0, 0, 255});
  renderText("R=Right (Green)", centerX - 40, legendY + 40, {0, 255, 0, 255});
  renderText("U=Up (Blue)", centerX - 40, legendY + 60, {0, 0, 255, 255});
  
  // Also print numeric values for debugging
  char debugText[256];
  snprintf(debugText, sizeof(debugText), "F: [%.2f, %.2f, %.2f]", 
           camera->forward.x, camera->forward.y, camera->forward.z);
  renderText(debugText, centerX - 80, legendY + 80, {200, 200, 200, 255});
  
  snprintf(debugText, sizeof(debugText), "R: [%.2f, %.2f, %.2f]", 
           camera->right.x, camera->right.y, camera->right.z);
  renderText(debugText, centerX - 80, legendY + 100, {200, 200, 200, 255});
  
  snprintf(debugText, sizeof(debugText), "U: [%.2f, %.2f, %.2f]", 
           camera->up.x, camera->up.y, camera->up.z);
  renderText(debugText, centerX - 80, legendY + 120, {200, 200, 200, 255});
}

