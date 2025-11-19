#!/bin/bash

# One-stop script for building, signing, and packaging macOS application
# Note: Signing is required - the script will exit if signing fails
#
# Usage:
#   ./sign_package.sh          # Clean build, sign, and package
#   ./sign_package.sh --no-build # Skip build, just sign and package (requires existing build)

# Get the project root directory (where this script is located)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${PROJECT_ROOT}"

EXPORT_DIR="export"
APP_NAME="BlackHoleSim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"
DMG_FILE="${EXPORT_DIR}/${APP_NAME}-1.0.dmg"

# Check for --no-build flag
SKIP_BUILD=false
if [[ "$1" == "--no-build" ]]; then
    SKIP_BUILD=true
fi

echo "=========================================="
echo "  Black Hole Simulation - Release Build"
echo "=========================================="
echo ""

# Step 1: Clean previous builds (unless skipping build)
if [ "$SKIP_BUILD" = false ]; then
    echo "[1/5] Cleaning previous builds..."
    make clean > /dev/null 2>&1 || true
    echo "✓ Cleaned"

    # Step 2: Build the application
    echo "[2/5] Building application..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    if ! make; then
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "✗ Build failed!"
        exit 1
    fi
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "✓ Build successful"
    
    STEP_OFFSET=0
else
    echo "[1/4] Skipping build (using existing build)..."
    # Verify executable exists
    if [ ! -f "${EXPORT_DIR}/blackhole_sim" ]; then
        echo "✗ Error: Executable not found. Run without --no-build first."
        exit 1
    fi
    echo "✓ Using existing build"
    STEP_OFFSET=1
fi

# Step 3: Create app bundle
STEP_NUM=$((3 - STEP_OFFSET))
TOTAL_STEPS=$((5 - STEP_OFFSET))
echo "[${STEP_NUM}/${TOTAL_STEPS}] Creating app bundle..."
if ! make app; then
    echo "✗ App bundle creation failed!"
    exit 1
fi
echo "✓ App bundle created: ${APP_BUNDLE}"

# Step 4: Sign the app
STEP_NUM=$((4 - STEP_OFFSET))
echo "[${STEP_NUM}/${TOTAL_STEPS}] Signing application..."

# Check if signing identity is set via environment variable
if [ -n "${MACOS_SIGNING_IDENTITY:-}" ]; then
    # Signing identity provided - signing is required
    echo "  Using signing identity: ${MACOS_SIGNING_IDENTITY}"
    if ! make sign; then
        echo ""
        echo "❌ Signing failed! Cannot proceed without valid signature."
        echo "   Please check your signing identity and try again."
        exit 1
    fi
    echo "✓ App signed and verified successfully"
else
    # No signing identity set - try interactive signing
    echo "  No MACOS_SIGNING_IDENTITY set, attempting interactive signing..."
    if ! make sign; then
        echo ""
        echo "❌ Signing failed! Cannot proceed without valid signature."
        echo "   Please select a signing identity or set MACOS_SIGNING_IDENTITY environment variable."
        exit 1
    fi
    echo "✓ App signed and verified successfully"
fi

# Step 5: Create DMG
STEP_NUM=$((5 - STEP_OFFSET))
echo "[${STEP_NUM}/${TOTAL_STEPS}] Creating DMG..."
if ! make dmg; then
    echo "✗ DMG creation failed!"
    exit 1
fi
echo "✓ DMG created: ${DMG_FILE}"

echo ""
echo "=========================================="
echo "  Release Build Complete!"
echo "=========================================="
echo ""
echo "Output files:"
echo "  • Executable: ${EXPORT_DIR}/blackhole_sim"
echo "  • App Bundle: ${APP_BUNDLE}"
echo "  • DMG File:   ${DMG_FILE}"
echo ""

echo "To test the app bundle:"
echo "  open ${APP_BUNDLE}"
echo ""
echo "To test the DMG:"
echo "  open ${DMG_FILE}"
echo ""

