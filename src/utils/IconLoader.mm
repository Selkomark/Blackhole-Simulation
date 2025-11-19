#import <Cocoa/Cocoa.h>
#import <SDL2/SDL.h>
#import <cstring>

// Get the path to the app bundle's Resources directory
const char* getBundleResourcesPath() {
    @autoreleasepool {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* resourcesPath = [bundle resourcePath];
        if (resourcesPath) {
            // Return a C string that persists (static storage)
            static char path[1024];
            strncpy(path, [resourcesPath UTF8String], sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            return path;
        }
        return nullptr;
    }
}

// Load PNG icon and set as SDL window icon
// iconPath can be relative to Resources directory or absolute
void loadWindowIcon(SDL_Window* window, const char* iconPath) {
    @autoreleasepool {
        NSString* pathStr = nil;
        
        // If path is absolute, use it directly
        if (iconPath[0] == '/') {
            pathStr = [NSString stringWithUTF8String:iconPath];
        } else {
            // Try to resolve relative to bundle Resources first
            const char* resourcesPath = getBundleResourcesPath();
            if (resourcesPath) {
                NSString* resourcesPathStr = [NSString stringWithUTF8String:resourcesPath];
                pathStr = [resourcesPathStr stringByAppendingPathComponent:[NSString stringWithUTF8String:iconPath]];
            } else {
                // Fallback to relative path (for development)
                pathStr = [NSString stringWithUTF8String:iconPath];
            }
        }
        
        NSImage* image = [[NSImage alloc] initWithContentsOfFile:pathStr];
        
        if (!image) {
            NSLog(@"Failed to load icon from: %@", pathStr);
            return;
        }
        
        // Get the largest representation (for Retina displays)
        NSImageRep* bestRep = nil;
        CGFloat bestSize = 0;
        for (NSImageRep* rep in [image representations]) {
            CGFloat size = rep.pixelsWide;
            if (size > bestSize) {
                bestSize = size;
                bestRep = rep;
            }
        }
        
        if (!bestRep) {
            bestRep = [image bestRepresentationForRect:NSMakeRect(0, 0, 1024, 1024) context:nil hints:nil];
        }
        
        if (!bestRep) {
            NSLog(@"Failed to get image representation");
            return;
        }
        
        // Convert to RGBA format
        NSSize size = NSMakeSize(bestRep.pixelsWide, bestRep.pixelsHigh);
        NSBitmapImageRep* bitmapRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                              pixelsWide:size.width
                                                                              pixelsHigh:size.height
                                                                           bitsPerSample:8
                                                                         samplesPerPixel:4
                                                                                hasAlpha:YES
                                                                                isPlanar:NO
                                                                          colorSpaceName:NSCalibratedRGBColorSpace
                                                                             bytesPerRow:0
                                                                            bitsPerPixel:0];
        
        [NSGraphicsContext saveGraphicsState];
        NSGraphicsContext* context = [NSGraphicsContext graphicsContextWithBitmapImageRep:bitmapRep];
        [NSGraphicsContext setCurrentContext:context];
        [image drawInRect:NSMakeRect(0, 0, size.width, size.height)];
        [NSGraphicsContext restoreGraphicsState];
        
        // Create SDL surface from bitmap data
        Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
#else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
#endif
        
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            [bitmapRep bitmapData],
            size.width,
            size.height,
            32,
            [bitmapRep bytesPerRow],
            rmask, gmask, bmask, amask
        );
        
        if (surface) {
            SDL_SetWindowIcon(window, surface);
            SDL_FreeSurface(surface);
        } else {
            NSLog(@"Failed to create SDL surface: %s", SDL_GetError());
        }
    }
}

