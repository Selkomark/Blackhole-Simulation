#pragma once

#include <string>

// Save ARGB8888 pixel data as PNG file
// pixels: ARGB8888 format (4 bytes per pixel: A, R, G, B)
// width, height: image dimensions
// filename: output file path
// Returns true on success, false on error
bool savePNG(const void* pixels, int width, int height, const std::string& filename);

