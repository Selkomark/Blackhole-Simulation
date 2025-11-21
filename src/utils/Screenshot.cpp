#include "../../include/utils/Screenshot.h"
#include <png.h>
#include <cstdio>
#include <vector>
#include <stdexcept>

// External logging function
extern void appLog(const std::string& message, bool isError = false);

bool savePNG(const void* pixels, int width, int height, const std::string& filename) {
    if (!pixels || width <= 0 || height <= 0) {
        appLog("[SCREENSHOT] Invalid parameters for PNG save", true);
        return false;
    }
    
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        std::string errMsg = "[SCREENSHOT] Could not open file for writing: " + filename;
        appLog(errMsg, true);
        return false;
    }
    
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        appLog("[SCREENSHOT] Failed to create PNG write struct", true);
        return false;
    }
    
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        appLog("[SCREENSHOT] Failed to create PNG info struct", true);
        return false;
    }
    
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        appLog("[SCREENSHOT] Error during PNG creation", true);
        return false;
    }
    
    png_init_io(png, fp);
    
    // Set image properties
    // ARGB8888 format: we need to convert to RGBA for PNG
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    
    // Convert BGRA to RGBA for PNG
    // Input is BGRA: B=byte0, G=byte1, R=byte2, A=byte3 (from Metal renderer)
    // Output is RGBA: R=byte0, G=byte1, B=byte2, A=byte3 (for PNG)
    const uint8_t* bgra = static_cast<const uint8_t*>(pixels);
    std::vector<uint8_t> rgbaData(width * height * 4);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            uint8_t b = bgra[idx + 0];
            uint8_t g = bgra[idx + 1];
            uint8_t r = bgra[idx + 2];
            uint8_t a = bgra[idx + 3];
            
            // Convert to RGBA
            rgbaData[idx + 0] = r;
            rgbaData[idx + 1] = g;
            rgbaData[idx + 2] = b;
            rgbaData[idx + 3] = a;
        }
    }
    
    // Set row pointers
    std::vector<png_bytep> rowPointers(height);
    for (int y = 0; y < height; y++) {
        rowPointers[y] = const_cast<png_bytep>(&rgbaData[y * width * 4]);
    }
    
    // Write PNG using standard API
    png_write_info(png, info);
    png_write_image(png, rowPointers.data());
    png_write_end(png, nullptr);
    
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    
    return true;
}

