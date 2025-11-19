#!/bin/bash

# Script to create macOS app bundle
set -e

EXPORT_DIR="export"
APP_NAME="BlackHoleSim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"
CONTENTS_DIR="${APP_BUNDLE}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

# Create export directory if it doesn't exist
mkdir -p "${EXPORT_DIR}"

# Clean previous bundle
rm -rf "${APP_BUNDLE}"

# Create bundle structure
mkdir -p "${MACOS_DIR}"
mkdir -p "${RESOURCES_DIR}"

# Copy executable and set execute permissions
cp "${EXPORT_DIR}/blackhole_sim" "${MACOS_DIR}/${APP_NAME}"
chmod +x "${MACOS_DIR}/${APP_NAME}"

# Note: Libraries are statically linked, so no need to copy .dylib files
# If you need dynamic libraries in the future, uncomment the Frameworks section below

# Copy assets
if [ -d "assets" ]; then
    cp -r assets "${RESOURCES_DIR}/"
fi

# Copy Metal shader library
if [ -f "build/default.metallib" ]; then
    cp "build/default.metallib" "${RESOURCES_DIR}/"
    echo "✓ Copied Metal shader library"
else
    echo "⚠ Warning: Metal shader library not found at build/default.metallib"
fi

# Create Info.plist
cat > "${CONTENTS_DIR}/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.blackhole.simulation</string>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundleDisplayName</key>
    <string>Black Hole Simulation</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>BHLS</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
</dict>
</plist>
EOF

# Create icon set if icon exists
if [ -f "assets/export/iOS-Default-1024x1024@1x.png" ]; then
    ICON_PATH="${RESOURCES_DIR}/AppIcon.icns"
    # Convert PNG to ICNS (requires iconutil or sips)
    if command -v iconutil &> /dev/null; then
        ICONSET_DIR="${RESOURCES_DIR}/AppIcon.iconset"
        mkdir -p "${ICONSET_DIR}"
        
        # Create various sizes for iconset (suppress verbose sips output)
        sips -z 16 16 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_16x16.png" >/dev/null 2>&1 || true
        sips -z 32 32 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_16x16@2x.png" >/dev/null 2>&1 || true
        sips -z 32 32 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_32x32.png" >/dev/null 2>&1 || true
        sips -z 64 64 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_32x32@2x.png" >/dev/null 2>&1 || true
        sips -z 128 128 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_128x128.png" >/dev/null 2>&1 || true
        sips -z 256 256 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_128x128@2x.png" >/dev/null 2>&1 || true
        sips -z 256 256 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_256x256.png" >/dev/null 2>&1 || true
        sips -z 512 512 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_256x256@2x.png" >/dev/null 2>&1 || true
        sips -z 512 512 "assets/export/iOS-Default-1024x1024@1x.png" --out "${ICONSET_DIR}/icon_512x512.png" >/dev/null 2>&1 || true
        cp "assets/export/iOS-Default-1024x1024@1x.png" "${ICONSET_DIR}/icon_512x512@2x.png" >/dev/null 2>&1 || true
        
        # Create icns file
        iconutil -c icns "${ICONSET_DIR}" -o "${ICON_PATH}" 2>/dev/null || true
        rm -rf "${ICONSET_DIR}"
    fi
fi

echo "App bundle created: ${APP_BUNDLE}"

