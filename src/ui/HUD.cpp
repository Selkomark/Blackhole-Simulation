#include "../../include/ui/HUD.hpp"
#include "../../include/utils/Vector3.hpp"
#include "../../include/utils/ResolutionManager.hpp"
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

HUD::HUD(SDL_Renderer *renderer, TTF_Font *font)
    : renderer(renderer), font(font), hintsVisible(true) {}

// Helper function to format resolution as readable string (4K, 1080p, etc.)
std::string formatResolution(int width, int height, ResolutionManager* resolutionManager) {
  if (!resolutionManager) {
    return std::to_string(width) + "×" + std::to_string(height);
  }
  
  const Resolution& res = resolutionManager->getCurrent();
  if (res.name && res.name[0] != '\0') {
    // Extract common format names
    std::string name = res.name;
    if (name.find("4K") != std::string::npos || name.find("2160p") != std::string::npos) {
      return "4K";
    } else if (name.find("1080p") != std::string::npos) {
      return "1080p";
    } else if (name.find("1440p") != std::string::npos || name.find("QHD") != std::string::npos) {
      return "1440p";
    } else if (name.find("720p") != std::string::npos) {
      return "720p";
    } else if (name.find("5K") != std::string::npos) {
      return "5K";
    } else if (name.find("8K") != std::string::npos) {
      return "8K";
    }
    // Return the name as-is if it's already formatted
    return name;
  }
  
  // Fallback to dimensions
  return std::to_string(width) + "×" + std::to_string(height);
}

void HUD::renderHints(bool showHints, CinematicMode mode, int fps, int windowWidth, int windowHeight, ResolutionManager* resolutionManager, int colorMode, float colorIntensity, bool isMusicMuted) {
  if (!showHints || !font)
    return;

  // Format resolution
  std::string resolutionStr = formatResolution(windowWidth, windowHeight, resolutionManager);
  
  // Color mode names
  const char* colorNames[] = {"Blue", "Orange", "Red", "White"};
  std::string colorModeStr = colorNames[colorMode % 4];
  
  // Format intensity
  std::ostringstream intensityStream;
  intensityStream << std::fixed << std::setprecision(1) << colorIntensity << "x";
  std::string intensityStr = intensityStream.str();

  // Define hints array with key and description separated
  struct HintLine {
    std::string key;
    std::string description;
    bool isSeparator;
    bool isInfo; // For Resolution/FPS lines that don't have keys
  };
  
  HintLine hints[] = {
    {"L/J", "Rotate Up Axis", false, false},
    {"I/K", "Rotate Right Axis", false, false},
    {"O/U", "Rotate Forward Axis", false, false},
    {"", "", true, false}, // Separator
    {"W/S", "Move Up/Down", false, false},
    {"A/D", "Zoom In/Out", false, false},
    {"", "", true, false}, // Separator
    {"R", "Reset Camera", false, false},
    {"T", "Camera: " + std::string(getCinematicModeName(mode)), false, false},
    {"C", "Color: " + colorModeStr, false, false},
    {"+/-", "Change Resolution", false, false},
    {"Shift +/-", "Intensity: " + intensityStr, false, false},
    {"", "", true, false}, // Separator
    {"Cmd+R", "Start Recording", false, false},
    {"Enter/Esc/Q", "Stop Recording", false, false},
    {"", "", true, false}, // Separator
    {"F", "Fullscreen", false, false},
    {"M", "Music: " + std::string(isMusicMuted ? "Muted" : "Playing"), false, false},
    {"Tab", "Toggle Help", false, false},
    {"", "", true, false}, // Separator
    {"ESC/Q", "Quit", false, false},
    {"", "", true, false}, // Separator
    {"Resolution:", resolutionStr, false, true},
    {"FPS:", std::to_string(fps), false, true}
  };
  
  const int numHints = sizeof(hints) / sizeof(hints[0]); // Calculate array size automatically

  // Calculate maximum widths for table alignment
  int maxKeyWidth = 0;
  int maxDescriptionWidth = 0;
  int lineHeight = 26; // Increased for larger font
  int padding = 12;
  int textPadding = 16;
  int columnSpacing = 8; // Space between key and description columns
  
  for (int i = 0; i < numHints; i++) {
    if (!hints[i].isSeparator) {
      if (!hints[i].key.empty()) {
        SDL_Surface *keySurface = TTF_RenderText_Blended(font, hints[i].key.c_str(), {255, 255, 255, 255});
        if (keySurface) {
          maxKeyWidth = std::max(maxKeyWidth, static_cast<int>(keySurface->w));
          SDL_FreeSurface(keySurface);
        }
      }
      SDL_Surface *descSurface = TTF_RenderText_Blended(font, hints[i].description.c_str(), {255, 255, 255, 255});
      if (descSurface) {
        maxDescriptionWidth = std::max(maxDescriptionWidth, static_cast<int>(descSurface->w));
        SDL_FreeSurface(descSurface);
      }
    }
  }
  
  // Calculate total width needed for table layout
  int maxTextWidth = maxKeyWidth + columnSpacing + maxDescriptionWidth;
  
  // Calculate overlay dimensions based on content
  int overlayWidth = maxTextWidth + (textPadding * 2);
  int overlayHeight = (numHints * lineHeight) + (textPadding * 2);
  
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

  // Render text with proper table alignment
  int y = overlayY + textPadding;
  int keyColumnX = overlayX + textPadding;
  int descColumnX = keyColumnX + maxKeyWidth + columnSpacing;
  
  for (int i = 0; i < numHints; i++) {
    // Skip separator lines (they're already accounted for in spacing)
    if (hints[i].isSeparator) {
      y += lineHeight;
      continue;
    }
    
    // Color coding based on content type
    SDL_Color keyColor = textColor; // Default white for keys
    SDL_Color descColor = textColor; // Default white for descriptions
    
    // Highlight key rotation controls (L/J, I/K, O/U) in blue
    if (i == 0 || i == 1 || i == 2) {
      keyColor = highlightColor;
      descColor = highlightColor;
    }
    // Highlight recording controls in red/orange
    else if (i == 11) { // Cmd+R - Start Recording
      keyColor = {255, 100, 100, 255}; // Light red
      descColor = {255, 100, 100, 255};
    }
    else if (i == 12) { // Enter/Esc/Q - Stop Recording
      keyColor = {255, 150, 100, 255}; // Orange-red
      descColor = {255, 150, 100, 255};
    }
    // Highlight Tab key in blue
    else if (i == 15) {
      keyColor = highlightColor;
      descColor = highlightColor;
    }
    // Highlight Reset Camera in orange
    else if (i == 7) {
      keyColor = cinematicColor;
      descColor = cinematicColor;
    }
    // Highlight Mode in orange
    else if (i == 8) {
      keyColor = cinematicColor;
      descColor = cinematicColor;
    }
    // Highlight Resolution in cyan-blue
    else if (i == 19) {
      descColor = highlightColor; // Info line, no key
    }
    // Highlight FPS in green
    else if (i == 20) {
      descColor = fpsColor; // Info line, no key
    }
    
    // Render key column (right-aligned within its column)
    if (!hints[i].key.empty()) {
      SDL_Surface *keySurface = TTF_RenderText_Blended(font, hints[i].key.c_str(), keyColor);
      if (keySurface) {
        int keyX = keyColumnX + maxKeyWidth - keySurface->w; // Right-align key
        SDL_Texture *keyTexture = SDL_CreateTextureFromSurface(renderer, keySurface);
        if (keyTexture) {
          SDL_Rect keyRect = {keyX, y, keySurface->w, keySurface->h};
          SDL_RenderCopy(renderer, keyTexture, nullptr, &keyRect);
          SDL_DestroyTexture(keyTexture);
        }
        SDL_FreeSurface(keySurface);
      }
    }
    
    // Render description column (left-aligned)
    if (!hints[i].description.empty()) {
      renderText(hints[i].description.c_str(), descColumnX, y, descColor);
    }
    
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

void HUD::renderMusicCredits(bool isMusicMuted, int windowWidth, int windowHeight) {
  // Suppress unused parameter warnings
  (void)windowWidth;
  (void)windowHeight;
  
  // Don't show credits if music is muted
  if (isMusicMuted || !font)
    return;
  
  // Get actual renderer output size (accounts for high DPI)
  int rendererWidth, rendererHeight;
  SDL_GetRendererOutputSize(renderer, &rendererWidth, &rendererHeight);
  
  // Music credit text
  const char* creditText = "'Interstellar Theme' - Hans Zimmer. Performed by Blackavec.";
  
  // Semi-transparent white/gray color
  SDL_Color creditColor = {200, 200, 200, 255};
  
  // Render the text surface to get dimensions
  SDL_Surface *surface = TTF_RenderText_Blended(font, creditText, creditColor);
  if (surface) {
    // Position at bottom-right corner with padding
    int padding = 20;
    int x = rendererWidth - surface->w - padding;
    int y = rendererHeight - surface->h - padding;
    
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
}

