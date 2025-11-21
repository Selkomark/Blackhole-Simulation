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
APP_NAME="Blackhole Simulation"
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

# Determine total steps based on whether we're skipping build
if [ "$SKIP_BUILD" = false ]; then
    TOTAL_STEPS=6
    STEP_OFFSET=0
else
    TOTAL_STEPS=5
    STEP_OFFSET=1
fi

# Step 1: Clean previous builds (unless skipping build)
if [ "$SKIP_BUILD" = false ]; then
    echo "[1/${TOTAL_STEPS}] Cleaning previous builds..."
    make clean > /dev/null 2>&1 || true
    echo "✓ Cleaned"

    # Step 2: Build the application
    echo "[2/${TOTAL_STEPS}] Building application..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    if ! make; then
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "✗ Build failed!"
        exit 1
    fi
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "✓ Build successful"
else
    echo "[1/${TOTAL_STEPS}] Skipping build (using existing build)..."
    # Verify executable exists
    if [ ! -f "${EXPORT_DIR}/blackhole_sim" ]; then
        echo "✗ Error: Executable not found. Run without --no-build first."
        exit 1
    fi
    echo "✓ Using existing build"
fi

# Step 3: Create app bundle
STEP_NUM=$((3 - STEP_OFFSET))
echo "[${STEP_NUM}/${TOTAL_STEPS}] Creating app bundle..."
if ! make app; then
    echo "✗ App bundle creation failed!"
    exit 1
fi
echo "✓ App bundle created: ${APP_BUNDLE}"

# Step 4: Sign the app
STEP_NUM=$((4 - STEP_OFFSET))
echo "[${STEP_NUM}/${TOTAL_STEPS}] Signing application..."

# Call signing script directly (not via make sign) to avoid recreating app bundle
if [ -n "${MACOS_SIGNING_IDENTITY:-}" ]; then
    # Signing identity provided - signing is required
    echo "  Using signing identity: ${MACOS_SIGNING_IDENTITY}"
    if ! ./scripts/sign_app.sh; then
        echo ""
        echo "❌ Signing failed! Cannot proceed without valid signature."
        echo "   Please check your signing identity and try again."
        exit 1
    fi
    echo "✓ App signed and verified successfully"
else
    # No signing identity set - try interactive signing
    echo "  No MACOS_SIGNING_IDENTITY set, attempting interactive signing..."
    if ! ./scripts/sign_app.sh; then
        echo ""
        echo "❌ Signing failed! Cannot proceed without valid signature."
        echo "   Please select a signing identity or set MACOS_SIGNING_IDENTITY environment variable."
        exit 1
    fi
    echo "✓ App signed and verified successfully"
fi

# Step 5: Notarize the app (if credentials are available)
STEP_NUM=$((5 - STEP_OFFSET))

# Check if notarization credentials are available
NOTARIZE_APP=false
if [ -n "${NOTARY_KEYCHAIN_PROFILE:-}" ]; then
    NOTARIZE_APP=true
    echo "[${STEP_NUM}/${TOTAL_STEPS}] Notarizing application..."
    echo "  Using keychain profile: ${NOTARY_KEYCHAIN_PROFILE}"
elif [ -n "${NOTARY_APPLE_ID:-}" ] && [ -n "${NOTARY_TEAM_ID:-}" ] && [ -n "${NOTARY_PASSWORD:-}" ]; then
    NOTARIZE_APP=true
    echo "[${STEP_NUM}/${TOTAL_STEPS}] Notarizing application..."
    echo "  Using Apple ID: ${NOTARY_APPLE_ID}"
else
    echo "[${STEP_NUM}/${TOTAL_STEPS}] Skipping notarization (credentials not configured)..."
    echo "  To enable notarization, set one of:"
    echo "    • NOTARY_KEYCHAIN_PROFILE (recommended)"
    echo "    • NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD"
    echo ""
fi

NOTARIZATION_SUCCESS=false
if [ "$NOTARIZE_APP" = true ]; then
    if ./scripts/notarize_app.sh; then
        NOTARIZATION_SUCCESS=true
        echo "✓ App notarized successfully"
    else
        echo ""
        echo "⚠️  Notarization failed, but continuing with packaging..."
        echo "   The app is signed but not notarized. Users may see Gatekeeper warnings."
        echo ""
    fi
fi

# Step 6: Create DMG
STEP_NUM=$((6 - STEP_OFFSET))
echo "[${STEP_NUM}/${TOTAL_STEPS}] Creating DMG..."
# Call DMG script directly (not via make dmg) to avoid recreating app bundle
if ! ./scripts/create_dmg.sh; then
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
if [ "$NOTARIZATION_SUCCESS" = true ]; then
    echo "  • Status:     Signed and Notarized ✅"
elif [ "$NOTARIZE_APP" = true ]; then
    echo "  • Status:     Signed (Notarization failed)"
else
    echo "  • Status:     Signed (Notarization skipped)"
fi
echo "  • DMG File:   ${DMG_FILE}"
echo ""

echo "To test the app bundle:"
echo "  open ${APP_BUNDLE}"
echo ""
echo "To test the DMG:"
echo "  open ${DMG_FILE}"
echo ""

if [ "$NOTARIZE_APP" = false ]; then
    echo "💡 Tip: To enable notarization, set up credentials:"
    echo "   export NOTARY_KEYCHAIN_PROFILE=\"notarization-profile\""
    echo "   # Or set: NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD"
    echo ""
fi

