#import <Cocoa/Cocoa.h>
#include <string>

// Show macOS save dialog and return selected path, or empty string if cancelled
std::string showSaveDialog(const std::string& defaultFilename) {
    @autoreleasepool {
        NSSavePanel *savePanel = [NSSavePanel savePanel];
        
        // Set default filename
        NSString *defaultName = [NSString stringWithUTF8String:defaultFilename.c_str()];
        [savePanel setNameFieldStringValue:defaultName];
        
        // Set allowed file types (using deprecated API for compatibility)
        // The warning is acceptable as the API still works
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [savePanel setAllowedFileTypes:@[@"mp4"]];
        #pragma clang diagnostic pop
        [savePanel setAllowsOtherFileTypes:NO];
        
        // Set title
        [savePanel setTitle:@"Save Video Recording"];
        [savePanel setMessage:@"Choose where to save your video recording:"];
        
        // Show dialog
        NSInteger result = [savePanel runModal];
        
        if (result == NSModalResponseOK) {
            NSURL *url = [savePanel URL];
            if (url) {
                const char *path = [[url path] UTF8String];
                return std::string(path);
            }
        }
        
        return std::string(); // Cancelled or error
    }
}

