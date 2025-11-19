#pragma once

#include <SDL2/SDL.h>

// Get the path to the app bundle's Resources directory
// Returns a path that must be freed by the caller, or nullptr if not in a bundle
const char* getBundleResourcesPath();

// Load PNG icon and set as SDL window icon
// iconPath can be relative to Resources directory or absolute
void loadWindowIcon(SDL_Window* window, const char* iconPath);

