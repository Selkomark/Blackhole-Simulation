#pragma once

#include <string>

// Show macOS save dialog and return selected path, or empty string if cancelled
std::string showSaveDialog(const std::string& defaultFilename);

// Show macOS save dialog for PNG images
std::string showSaveDialogPNG(const std::string& defaultFilename);

