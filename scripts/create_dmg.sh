#!/bin/bash

# Script to create DMG file for macOS distribution
set -e

EXPORT_DIR="export"
APP_NAME="Blackhole Sim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"
DMG_NAME="${APP_NAME}"
# Use APP_VERSION from environment if set, otherwise default to 1.0
VERSION="${APP_VERSION:-1.0}"
DMG_FILE="${EXPORT_DIR}/${DMG_NAME}-${VERSION}.dmg"
DMG_TEMP="${EXPORT_DIR}/${DMG_NAME}-temp.dmg"
DMG_DIR="${EXPORT_DIR}/dmg_build"

# Create export directory if it doesn't exist
mkdir -p "${EXPORT_DIR}"

# Clean previous builds
rm -rf "${DMG_DIR}"
rm -f "${DMG_FILE}" "${DMG_TEMP}"

# Check if app bundle exists
if [ ! -d "${APP_BUNDLE}" ]; then
    echo "Error: App bundle not found. Run 'make app' first."
    exit 1
fi

# Create DMG directory structure
mkdir -p "${DMG_DIR}"

# Copy app bundle
cp -R "${APP_BUNDLE}" "${DMG_DIR}/"

# Create Applications symlink
ln -s /Applications "${DMG_DIR}/Applications"

# Copy README if it exists
if [ -f "README_DMG.txt" ]; then
    cp README_DMG.txt "${DMG_DIR}/README.txt"
    echo "✓ Added README.txt to DMG"
else
    echo "⚠ Warning: README_DMG.txt not found, skipping README in DMG"
fi

# Calculate size needed for DMG
SIZE=$(du -sm "${DMG_DIR}" | cut -f1)
SIZE=$((SIZE + 20)) # Add 20MB buffer

# Create temporary DMG
hdiutil create -srcfolder "${DMG_DIR}" -volname "${APP_NAME}" \
    -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${SIZE}M "${DMG_TEMP}"

# Mount the DMG
DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "${DMG_TEMP}" | \
    egrep '^/dev/' | sed 1q | awk '{print $1}')

# Wait for mount
sleep 2

# Set volume icon and background (optional)
VOLUME_NAME="${APP_NAME}"
VOLUME_PATH="/Volumes/${VOLUME_NAME}"

# Set window properties
echo "Setting DMG window properties..."
osascript <<EOF || true
tell application "Finder"
    tell disk "${VOLUME_NAME}"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set bounds of container window to {400, 100, 920, 420}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set position of item "${APP_NAME}.app" of container window to {160, 205}
        set position of item "Applications" of container window to {360, 205}
        if exists file "README.txt" then
            set position of item "README.txt" of container window to {160, 320}
        end if
        close
    end tell
end tell
EOF

# Unmount
hdiutil detach "${DEVICE}"

# Convert to compressed read-only DMG
hdiutil convert "${DMG_TEMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FILE}"

# Clean up
rm -rf "${DMG_DIR}"
rm -f "${DMG_TEMP}"

echo "DMG created: ${DMG_FILE}"

