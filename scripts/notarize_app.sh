#!/bin/bash

# Script to notarize macOS app bundle with Apple
#
# This script automates the notarization process required for macOS distribution.
# Notarization allows Gatekeeper to verify your app without user intervention.
#
# Prerequisites:
#   1. App must be signed with a Developer ID Application certificate
#   2. App-specific password from appleid.apple.com (not your regular password)
#   3. Apple ID email and Team ID
#
# Usage:
#   Option 1: Using keychain profile (recommended)
#     xcrun notarytool store-credentials "profile-name" \
#       --apple-id "your@email.com" --team-id "TEAM_ID" --password "app-password"
#     export NOTARY_KEYCHAIN_PROFILE="profile-name"
#     ./scripts/notarize_app.sh
#
#   Option 2: Using environment variables
#     export NOTARY_APPLE_ID="your@email.com"
#     export NOTARY_TEAM_ID="TEAM_ID"
#     export NOTARY_PASSWORD="app-specific-password"
#     ./scripts/notarize_app.sh
#
#   Option 3: Interactive prompts (if credentials not set)
#     ./scripts/notarize_app.sh
#
#   Option 4: Using Makefile
#     make notarize
#
# The script will:
#   1. Verify the app is signed
#   2. Create a zip file for submission
#   3. Submit to Apple's notarization service
#   4. Wait for completion (5-15 minutes)
#   5. Staple the notarization ticket to the app
#   6. Verify notarization and Gatekeeper status
#
# After notarization, recreate the DMG:
#   make dmg
#
# See PACKAGING.md for detailed documentation.

set -e

EXPORT_DIR="export"
APP_NAME="Blackhole Simulation"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"
ZIP_FILE="${EXPORT_DIR}/${APP_NAME}.zip"

# Check if app bundle exists
if [ ! -d "${APP_BUNDLE}" ]; then
    echo "❌ Error: App bundle not found: ${APP_BUNDLE}"
    echo "   Run 'make app' and 'make sign' first."
    exit 1
fi

# Check if app is signed
if ! codesign --verify --verbose "${APP_BUNDLE}" >/dev/null 2>&1; then
    echo "❌ Error: App bundle is not signed."
    echo "   Run 'make sign' first to sign the app."
    exit 1
fi

# Check if app is signed with Developer ID certificate (required for notarization)
SIGNING_AUTHORITY=$(codesign -dvv "${APP_BUNDLE}" 2>&1 | grep "Authority=Developer ID Application" || true)
if [ -z "$SIGNING_AUTHORITY" ]; then
    echo "❌ Error: App is not signed with a Developer ID Application certificate."
    echo ""
    echo "   Notarization requires a Developer ID Application certificate."
    echo "   Current signature:"
    codesign -dvv "${APP_BUNDLE}" 2>&1 | grep "Authority=" | head -1 || true
    echo ""
    echo "   Please sign the app with a Developer ID Application certificate:"
    echo "   export MACOS_SIGNING_IDENTITY='Developer ID Application: Your Name (TEAM_ID)'"
    echo "   make sign"
    exit 1
fi

echo ""
echo "🚀 Starting notarization process..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check for keychain profile first (recommended method)
KEYCHAIN_PROFILE="${NOTARY_KEYCHAIN_PROFILE:-}"

# If no keychain profile, check for direct credentials
if [ -z "$KEYCHAIN_PROFILE" ]; then
    APPLE_ID="${NOTARY_APPLE_ID:-}"
    TEAM_ID="${NOTARY_TEAM_ID:-}"
    PASSWORD="${NOTARY_PASSWORD:-}"
    
    # If credentials are not set, prompt for them
    if [ -z "$APPLE_ID" ] || [ -z "$TEAM_ID" ] || [ -z "$PASSWORD" ]; then
        echo "📋 Notarization requires Apple ID credentials."
        echo ""
        echo "Option 1: Use keychain profile (recommended)"
        echo "  Store credentials once:"
        echo "    xcrun notarytool store-credentials \"profile-name\" \\"
        echo "      --apple-id \"your@email.com\" \\"
        echo "      --team-id \"TEAM_ID\" \\"
        echo "      --password \"app-specific-password\""
        echo ""
        echo "  Then set: export NOTARY_KEYCHAIN_PROFILE=\"profile-name\""
        echo ""
        echo "Option 2: Set environment variables"
        echo "  export NOTARY_APPLE_ID=\"your@email.com\""
        echo "  export NOTARY_TEAM_ID=\"TEAM_ID\""
        echo "  export NOTARY_PASSWORD=\"app-specific-password\""
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        
        # Try to extract team ID from signing identity
        SIGNING_IDENTITY=$(codesign -dvv "${APP_BUNDLE}" 2>&1 | grep "Authority=Developer ID Application" | head -1 | sed -n 's/.*Developer ID Application: [^(]*(\([^)]*\)).*/\1/p')
        if [ -n "$SIGNING_IDENTITY" ]; then
            echo "💡 Detected Team ID from signature: ${SIGNING_IDENTITY}"
            echo ""
            TEAM_ID="$SIGNING_IDENTITY"
        fi
        
        # Prompt for missing values
        if [ -z "$APPLE_ID" ]; then
            echo -n "👉 Enter Apple ID email: "
            read APPLE_ID
        fi
        
        if [ -z "$TEAM_ID" ]; then
            echo -n "👉 Enter Team ID: "
            read TEAM_ID
        fi
        
        if [ -z "$PASSWORD" ]; then
            echo -n "👉 Enter app-specific password: "
            read -s PASSWORD
            echo ""
        fi
    fi
fi

# Step 1: Create zip file for notarization
echo "📦 Creating zip file for notarization..."
if [ -f "${ZIP_FILE}" ]; then
    rm -f "${ZIP_FILE}"
fi

cd "${EXPORT_DIR}"
ditto -c -k --keepParent "${APP_NAME}.app" "${APP_NAME}.zip"
cd ..

if [ ! -f "${ZIP_FILE}" ]; then
    echo "❌ Failed to create zip file"
    exit 1
fi

echo "✅ Zip file created: ${ZIP_FILE}"
echo ""

# Step 2: Submit for notarization
echo "📤 Submitting app for notarization..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ -n "$KEYCHAIN_PROFILE" ]; then
    echo "   Using keychain profile: ${KEYCHAIN_PROFILE}"
    echo "   Submitting... (this may take 5-15 minutes)"
    echo ""
    SUBMIT_OUTPUT=$(xcrun notarytool submit "${ZIP_FILE}" \
        --keychain-profile "${KEYCHAIN_PROFILE}" \
        --wait 2>&1)
else
    echo "   Using Apple ID: ${APPLE_ID}"
    echo "   Submitting... (this may take 5-15 minutes)"
    echo ""
    SUBMIT_OUTPUT=$(xcrun notarytool submit "${ZIP_FILE}" \
        --apple-id "${APPLE_ID}" \
        --team-id "${TEAM_ID}" \
        --password "${PASSWORD}" \
        --wait 2>&1)
fi

SUBMIT_EXIT=$?

# Check for submission errors
if [ $SUBMIT_EXIT -ne 0 ]; then
    echo ""
    echo "❌ Notarization submission failed!"
    echo "$SUBMIT_OUTPUT"
    echo ""
    echo "Common issues:"
    echo "  - Invalid Apple ID or password"
    echo "  - App-specific password required (not regular password)"
    echo "  - Team ID mismatch"
    echo "  - Network connectivity issues"
    exit 1
fi

# Extract submission ID for reference
SUBMISSION_ID=$(echo "$SUBMIT_OUTPUT" | grep -i "id:" | head -1 | sed -n 's/.*[Ii][Dd]:[[:space:]]*\([^[:space:]]*\).*/\1/p' || echo "")

echo "$SUBMIT_OUTPUT"
echo ""

# Check if notarization succeeded
if echo "$SUBMIT_OUTPUT" | grep -qi "status:.*accepted\|status:.*success"; then
    echo "✅ Notarization accepted!"
else
    echo "⚠️  Notarization status unclear. Checking logs..."
    if [ -n "$SUBMISSION_ID" ]; then
        echo "   Submission ID: ${SUBMISSION_ID}"
        if [ -n "$KEYCHAIN_PROFILE" ]; then
            xcrun notarytool log "${SUBMISSION_ID}" --keychain-profile "${KEYCHAIN_PROFILE}" || true
        else
            xcrun notarytool log "${SUBMISSION_ID}" \
                --apple-id "${APPLE_ID}" \
                --team-id "${TEAM_ID}" \
                --password "${PASSWORD}" || true
        fi
    fi
    echo ""
    echo "⚠️  Please check the notarization status manually."
    exit 1
fi

echo ""

# Step 3: Staple the ticket to the app
echo "📎 Stapling notarization ticket to app..."
if xcrun stapler staple "${APP_BUNDLE}" 2>&1; then
    echo "✅ Ticket stapled successfully"
else
    echo "❌ Failed to staple ticket"
    exit 1
fi

echo ""

# Step 4: Verify notarization
echo "🔍 Verifying notarization..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if xcrun stapler validate "${APP_BUNDLE}" 2>&1; then
    echo "✅ Notarization verified!"
else
    echo "⚠️  Validation check failed (this may be normal if ticket hasn't propagated yet)"
fi

echo ""

# Step 5: Check Gatekeeper assessment
echo "🛡️  Gatekeeper assessment:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
set +e
SPCTL_OUTPUT=$(spctl --assess --verbose "${APP_BUNDLE}" 2>&1)
SPCTL_EXIT=$?
set -e

if [ $SPCTL_EXIT -eq 0 ]; then
    echo "✅ Gatekeeper assessment passed!"
else
    echo "⚠️  Gatekeeper assessment: $SPCTL_OUTPUT"
    echo "   (This may take a few minutes to propagate)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Notarization complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📦 Notarized app: ${APP_BUNDLE}"
echo ""
echo "💡 Next steps:"
echo "   1. Recreate DMG with notarized app: make dmg"
echo "   2. Test the app on a clean macOS system"
echo "   3. The app should now pass Gatekeeper without user intervention"
echo ""

