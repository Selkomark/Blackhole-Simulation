#!/bin/bash

# Script to prepare Black Hole Simulation for App Store Connect upload
# Creates .itmsp package and uploads via Transporter CLI
# Requires: App Store certificate
#
# Environment Variables (required):
#   TRANSPORTER_APPLE_ID - Your Apple ID email
#   TRANSPORTER_PASSWORD - App-specific password (not your regular password)
#
# Usage:
#   export TRANSPORTER_APPLE_ID="your@email.com"
#   export TRANSPORTER_PASSWORD="xxxx-xxxx-xxxx-xxxx"
#   make upload

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
APP_NAME="Blackhole Sim"
APP_BUNDLE="export/${APP_NAME}.app"
ITMSP_NAME="${APP_NAME}.itmsp"
ITMSP_PATH="export/${ITMSP_NAME}"
BUNDLE_ID="com.blackhole.simulation"
VERSION="1.0"

# Transporter configuration
TRANSPORTER_APP="/Applications/Transporter.app"
# TRANSPORTER_BIN will be found dynamically below (supports CI/CD environments)
APPLE_ID="${TRANSPORTER_APPLE_ID:-}"
APP_SPECIFIC_PASSWORD="${TRANSPORTER_PASSWORD:-}"

# Extract Team ID from app signature for verification
DETECTED_TEAM_ID=""
if [ -d "$APP_BUNDLE" ]; then
    SIGNING_INFO=$(codesign -dvv "$APP_BUNDLE" 2>&1 | grep "TeamIdentifier=" | head -1 || true)
    if [ -n "$SIGNING_INFO" ]; then
        DETECTED_TEAM_ID=$(echo "$SIGNING_INFO" | sed -n 's/.*TeamIdentifier=\([^[:space:]]*\).*/\1/p')
    fi
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}App Store Connect Upload Preparation${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Check if app bundle exists
if [ ! -d "$APP_BUNDLE" ]; then
    echo -e "${YELLOW}⚠ App bundle not found. Building...${NC}"
    echo ""
    echo "Running: make clean && make && make app"
    make clean
    make
    make app
    echo ""
fi

# Verify app is signed
echo -e "${BLUE}Checking app signature...${NC}"
SIGNING_INFO=$(codesign -dvv "$APP_BUNDLE" 2>&1 | grep "Authority=" | head -1 || true)

if [ -z "$SIGNING_INFO" ]; then
    echo -e "${YELLOW}⚠ App is not signed. Signing now...${NC}"
    make sign
    SIGNING_INFO=$(codesign -dvv "$APP_BUNDLE" 2>&1 | grep "Authority=" | head -1 || true)
fi

# Extract Team ID from signature
TEAM_ID_INFO=$(codesign -dvv "$APP_BUNDLE" 2>&1 | grep "TeamIdentifier=" | head -1 || true)
if [ -n "$TEAM_ID_INFO" ]; then
    DETECTED_TEAM_ID=$(echo "$TEAM_ID_INFO" | sed -n 's/.*TeamIdentifier=\([^[:space:]]*\).*/\1/p')
fi

# Check if using App Store certificate
if echo "$SIGNING_INFO" | grep -q "Developer ID Application"; then
    echo -e "${RED}❌ Error: App is signed with Developer ID certificate${NC}"
    echo "For App Store submission, you need an App Store certificate:"
    echo "  - Apple Distribution: ..."
    echo "  - 3rd Party Mac Developer Application: ..."
    echo ""
    echo "Please sign with an App Store certificate:"
    echo "  export MACOS_SIGNING_IDENTITY=\"Apple Distribution: Your Name (TEAM_ID)\""
    echo "  make sign"
    exit 1
fi

if echo "$SIGNING_INFO" | grep -qE "(Apple Distribution|3rd Party Mac Developer)"; then
    echo -e "${GREEN}✓ App is signed with App Store certificate${NC}"
    echo "  $SIGNING_INFO"
    if [ -n "$DETECTED_TEAM_ID" ]; then
        echo -e "${BLUE}  Team ID: ${DETECTED_TEAM_ID}${NC}"
        echo ""
        echo -e "${YELLOW}⚠ Team Verification:${NC}"
        echo "  Make sure this Team ID matches your App Store Connect team:"
        echo "  1. Go to https://appstoreconnect.apple.com/"
        echo "  2. Check the team selector (top right)"
        echo "  3. Verify Team ID matches: ${DETECTED_TEAM_ID}"
        echo ""
        echo "  If you have multiple teams, ensure you're uploading to the correct one!"
        echo ""
    fi
else
    echo -e "${YELLOW}⚠ Warning: Could not verify certificate type${NC}"
    echo "  $SIGNING_INFO"
fi

echo ""

# Create .itmsp package
echo -e "${BLUE}Creating .itmsp package for Transporter...${NC}"

# Check if .itmsp already exists (e.g., created by create_itmsp.sh in CI/CD)
if [ -d "$ITMSP_PATH" ] && [ -f "$ITMSP_PATH/metadata.xml" ] && [ -f "$ITMSP_PATH/${APP_NAME}.pkg" ]; then
    echo -e "${GREEN}✓ .itmsp package already exists, using existing package${NC}"
    echo "   Package: $ITMSP_PATH"
    SKIP_ITMSP_CREATION=true
else
# Remove old .itmsp if exists
if [ -d "$ITMSP_PATH" ]; then
    echo "Removing old .itmsp package..."
    rm -rf "$ITMSP_PATH"
fi

# Create .itmsp directory
mkdir -p "$ITMSP_PATH"
    SKIP_ITMSP_CREATION=false
fi

# Only create .itmsp if it doesn't already exist
if [ "$SKIP_ITMSP_CREATION" != "true" ]; then
    # Create .pkg installer from app bundle (required for App Store uploads)
    echo "Creating .pkg installer..."
PKG_FILE="${APP_NAME}.pkg"
PKG_PATH="$ITMSP_PATH/$PKG_FILE"

# Build the .pkg installer
# The app will be installed to /Applications
productbuild --component "$APP_BUNDLE" /Applications "$PKG_PATH"

if [ ! -f "$PKG_PATH" ]; then
    echo -e "${RED}❌ Failed to create .pkg installer${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Created unsigned .pkg installer${NC}"

# Sign the .pkg with "3rd Party Mac Developer Installer" certificate
echo "Signing .pkg installer..."

# Find the installer certificate (extract hash, not name - more reliable)
# Note: "Missing required extension" in output is misleading - certificate may still work
INSTALLER_LINE=$(security find-identity -v -p macappstore | grep "3rd Party Mac Developer Installer" | head -1)

if [ -z "$INSTALLER_LINE" ]; then
    echo -e "${YELLOW}⚠️  Warning: No valid '3rd Party Mac Developer Installer' certificate found${NC}"
    echo "   The .pkg will be uploaded unsigned, which will cause validation errors."
    echo ""
    echo "Your certificate is either missing or invalid (shows 'Missing required extension')."
    echo ""
    echo "To fix this:"
    echo "1. Go to https://developer.apple.com/account/resources/certificates/list"
    echo "2. Revoke the old 'Mac Installer Distribution' certificate (if exists)"
    echo "3. Create a NEW 'Mac Installer Distribution' certificate"
    echo "4. Download and double-click to install it in Keychain"
    echo "5. Run 'make upload' again - signing will happen automatically"
    echo ""
else
    # Extract the certificate hash (first 40 hex chars)
    INSTALLER_HASH=$(echo "$INSTALLER_LINE" | awk '{print $2}')
    INSTALLER_NAME=$(echo "$INSTALLER_LINE" | sed 's/.*"\(.*\)"/\1/')
    
    echo "Found installer certificate: $INSTALLER_NAME"
    echo "Certificate hash: $INSTALLER_HASH"
    
    # Use the hash for signing (more reliable than name)
    if productsign --sign "$INSTALLER_HASH" "$PKG_PATH" "${PKG_PATH}.signed" 2>&1; then
        if [ -f "${PKG_PATH}.signed" ]; then
            mv "${PKG_PATH}.signed" "$PKG_PATH"
            echo -e "${GREEN}✓ Signed .pkg installer${NC}"
            
            # Verify the signature
            if pkgutil --check-signature "$PKG_PATH" &>/dev/null; then
                echo -e "${GREEN}✓ Package signature verified${NC}"
            fi
        else
            echo -e "${RED}❌ Failed to create signed .pkg${NC}"
            echo "   The .pkg will be uploaded unsigned."
        fi
    else
        echo -e "${RED}❌ productsign failed${NC}"
        echo "   The certificate may be invalid or expired."
        echo "   The .pkg will be uploaded unsigned."
    fi
fi

# Clean extended attributes
echo "Cleaning extended attributes..."
xattr -cr "$ITMSP_PATH" 2>/dev/null || true

# Create metadata.xml
# For macOS apps, reference the .pkg file (not the .app bundle)
echo "Creating metadata.xml..."
cat > "$ITMSP_PATH/metadata.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<package version="software5.4" xmlns="http://apple.com/itunes/importer">
<software_assets>
        <asset type="bundle">
            <data_file>
<file_name>${PKG_FILE}</file_name>
            </data_file>
        </asset>
    </software_assets>
</package>
EOF

echo -e "${GREEN}✓ Created .itmsp package: $ITMSP_PATH${NC}"
echo ""
else
    # Use existing .itmsp, just clean extended attributes
    echo "Cleaning extended attributes on existing .itmsp..."
    xattr -cr "$ITMSP_PATH" 2>/dev/null || true
fi

# Command-line upload (required for CI/CD)
echo -e "${BLUE}Using command-line upload...${NC}"
echo ""

# Validate required environment variables
if [ -z "$APPLE_ID" ]; then
    echo -e "${RED}❌ Error: TRANSPORTER_APPLE_ID environment variable is required${NC}"
    echo ""
    echo "Set it before running:"
    echo "  export TRANSPORTER_APPLE_ID=\"your@email.com\""
    echo "  export TRANSPORTER_PASSWORD=\"xxxx-xxxx-xxxx-xxxx\""
    echo "  make upload"
    exit 1
fi

if [ -z "$APP_SPECIFIC_PASSWORD" ]; then
    echo -e "${RED}❌ Error: TRANSPORTER_PASSWORD environment variable is required${NC}"
    echo ""
    echo "⚠️  IMPORTANT: Use an APP-SPECIFIC PASSWORD, not your regular password!"
    echo ""
    echo "Create one at: https://appleid.apple.com/account/manage"
    echo "  - Go to 'Sign-In and Security' → 'App-Specific Passwords'"
    echo "  - Click '+' to generate a new password for 'Transporter'"
    echo "  - Copy the password (shown only once)"
    echo ""
    echo "Then set:"
    echo "  export TRANSPORTER_PASSWORD=\"xxxx-xxxx-xxxx-xxxx\""
    exit 1
fi

# Find iTMSTransporter CLI binary (try multiple locations)
TRANSPORTER_BIN=""

# 1. Standard location (when Transporter.app is installed)
if [ -f "$TRANSPORTER_APP/Contents/itms/bin/iTMSTransporter" ]; then
    TRANSPORTER_BIN="$TRANSPORTER_APP/Contents/itms/bin/iTMSTransporter"
fi

# 2. Check if in PATH
if [ -z "$TRANSPORTER_BIN" ] && command -v iTMSTransporter &> /dev/null; then
    TRANSPORTER_BIN=$(command -v iTMSTransporter)
fi

# 3. Check Xcode locations (comprehensive search)
if [ -z "$TRANSPORTER_BIN" ]; then
    XCODE_PATH=$(xcode-select -p 2>/dev/null || echo "")
    
    # Try various Xcode paths (including AppStoreService.framework)
    XCODE_PATHS=(
        "/Applications/Xcode.app/Contents/SharedFrameworks/ContentDeliveryServices.framework/Versions/A/Frameworks/AppStoreService.framework/Versions/A/Support/iTMSTransporter"
        "/Applications/Xcode.app/Contents/itms/bin/iTMSTransporter"
        "/Applications/Xcode.app/Contents/Developer/usr/bin/iTMSTransporter"
        "/Library/Developer/CommandLineTools/itms/bin/iTMSTransporter"
        "/Library/Developer/CommandLineTools/usr/bin/iTMSTransporter"
    )
    
    # Add paths relative to xcode-select path if available
    if [ -n "$XCODE_PATH" ]; then
        XCODE_PATHS+=(
            "$XCODE_PATH/../SharedFrameworks/ContentDeliveryServices.framework/Versions/A/Frameworks/AppStoreService.framework/Versions/A/Support/iTMSTransporter"
            "$XCODE_PATH/../itms/bin/iTMSTransporter"
            "$XCODE_PATH/../Developer/usr/bin/iTMSTransporter"
        )
    fi
    
    for path in "${XCODE_PATHS[@]}"; do
        if [ -f "$path" ] && [ -x "$path" ]; then
            TRANSPORTER_BIN="$path"
            break
        fi
    done
    
    # Also search inside Xcode.app more thoroughly if still not found
    if [ -z "$TRANSPORTER_BIN" ] && [ -d "/Applications/Xcode.app" ]; then
        FOUND_IN_XCODE=$(find /Applications/Xcode.app -name "iTMSTransporter" -type f -perm +111 2>/dev/null | head -1)
        if [ -n "$FOUND_IN_XCODE" ] && [ -f "$FOUND_IN_XCODE" ] && [ -x "$FOUND_IN_XCODE" ]; then
            TRANSPORTER_BIN="$FOUND_IN_XCODE"
        fi
    fi
fi

# 4. Check /usr/local/itms (Command Line Tools)
if [ -z "$TRANSPORTER_BIN" ] && [ -f "/usr/local/itms/bin/iTMSTransporter" ]; then
    TRANSPORTER_BIN="/usr/local/itms/bin/iTMSTransporter"
fi

# 5. Try to find it anywhere in common directories
if [ -z "$TRANSPORTER_BIN" ]; then
    FOUND=$(find /Applications /Library/Developer /usr/local -name "iTMSTransporter" -type f 2>/dev/null | head -1)
    if [ -n "$FOUND" ]; then
        TRANSPORTER_BIN="$FOUND"
    fi
fi

# Check if command-line binary exists
if [ -z "$TRANSPORTER_BIN" ] || [ ! -f "$TRANSPORTER_BIN" ]; then
    echo -e "${RED}❌ Transporter CLI binary (iTMSTransporter) not found${NC}"
    echo ""
    echo "Searched locations:"
    echo "  - $TRANSPORTER_APP/Contents/itms/bin/iTMSTransporter"
    echo "  - In PATH (via 'command -v')"
    if [ -n "$XCODE_PATH" ]; then
        echo "  - $XCODE_PATH/../itms/bin/iTMSTransporter"
        echo "  - $XCODE_PATH/../SharedFrameworks/ContentDeliveryServices.framework/Versions/A/itms/bin/iTMSTransporter"
    fi
    echo "  - /Applications/Xcode.app/Contents/SharedFrameworks/ContentDeliveryServices.framework/Versions/A/itms/bin/iTMSTransporter"
    echo "  - /Library/Developer/CommandLineTools/itms/bin/iTMSTransporter"
    echo "  - /usr/local/itms/bin/iTMSTransporter"
    echo "  - Common directories (/Applications, /Library/Developer, /usr/local)"
    echo ""
    echo "For local development, install Transporter from the Mac App Store:"
    echo "  https://apps.apple.com/app/transporter/id1450874784"
    echo ""
    echo "For CI/CD, the workflow should install Transporter automatically."
    echo "If this error persists, check the 'Install Transporter' step in the workflow."
    exit 1
fi

echo -e "${GREEN}✓ Found Transporter CLI: $TRANSPORTER_BIN${NC}"
echo ""

# Validate package structure first
echo -e "${BLUE}Validating .itmsp package structure...${NC}"
if [ ! -f "$ITMSP_PATH/metadata.xml" ]; then
    echo -e "${RED}❌ Error: metadata.xml not found in .itmsp package${NC}"
    exit 1
fi
if [ ! -f "$ITMSP_PATH/${APP_NAME}.pkg" ]; then
    echo -e "${RED}❌ Error: .pkg installer not found in .itmsp package${NC}"
    exit 1
fi

# Validate metadata.xml format
if ! grep -q "<software_assets>" "$ITMSP_PATH/metadata.xml"; then
    echo -e "${RED}❌ Error: Invalid metadata.xml format${NC}"
    exit 1
fi

# Check if metadata.xml has apple_id attribute (should be omitted for macOS)
if grep -q 'apple_id=' "$ITMSP_PATH/metadata.xml"; then
    echo -e "${YELLOW}⚠ Warning: metadata.xml contains apple_id attribute${NC}"
    echo "  This can cause 'Client configuration failed' errors"
    echo "  Removing apple_id attribute..."
    # Remove apple_id attribute from metadata.xml
    sed -i '' 's/ apple_id="[^"]*"//' "$ITMSP_PATH/metadata.xml"
    echo -e "${GREEN}✓ Fixed metadata.xml${NC}"
fi

# Check pkg signature
if ! pkgutil --check-signature "$ITMSP_PATH/${APP_NAME}.pkg" &>/dev/null; then
    echo -e "${YELLOW}⚠ Warning: .pkg signature validation failed${NC}"
else
    echo -e "${GREEN}✓ .pkg signature verified${NC}"
fi

# Check for extended attributes that might cause issues
EXT_ATTRS=$(xattr -l "$ITMSP_PATH" 2>/dev/null | grep -v 'com.apple.quarantine' | wc -l | tr -d ' ')
if [ "$EXT_ATTRS" -gt 0 ]; then
    echo -e "${YELLOW}⚠ Warning: .itmsp package has extended attributes${NC}"
    echo "   Cleaning them now..."
    xattr -cr "$ITMSP_PATH" 2>/dev/null || true
fi

echo -e "${GREEN}✓ Package structure validated${NC}"
echo ""

echo "Uploading to App Store Connect..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Apple ID: ${APPLE_ID}"
if [ -n "$DETECTED_TEAM_ID" ]; then
    echo "Team ID: ${DETECTED_TEAM_ID}"
fi
echo "Package: $(pwd)/$ITMSP_PATH"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
if [ -n "$DETECTED_TEAM_ID" ]; then
    echo -e "${YELLOW}⚠ Important: Verify Team Selection${NC}"
    echo "  Your app is signed with Team ID: ${DETECTED_TEAM_ID}"
    echo "  Make sure you're uploading to the correct team in App Store Connect!"
    echo "  If you have multiple teams, select the matching team when signing in."
    echo ""
fi

# Upload using command-line Transporter
# Capture both stdout and stderr, and also display in real-time
echo "Starting upload (this may take a few minutes)..."
echo ""

# Try to detect provider short name (helps with multi-team accounts)
echo -e "${BLUE}Detecting provider information...${NC}"
PROVIDER_OUTPUT=$($TRANSPORTER_BIN -m provider -u "$APPLE_ID" -p "$APP_SPECIFIC_PASSWORD" 2>&1)
PROVIDER_SHORT_NAME=$(echo "$PROVIDER_OUTPUT" | grep -A 5 "Provider listing:" | grep -E "^[0-9]+" | awk '{print $NF}' | head -1)

if [ -n "$PROVIDER_SHORT_NAME" ]; then
    echo -e "${GREEN}✓ Detected provider: $PROVIDER_SHORT_NAME${NC}"
    echo ""
    # Build upload command with provider
    UPLOAD_CMD="$TRANSPORTER_BIN -m upload -u \"$APPLE_ID\" -p \"$APP_SPECIFIC_PASSWORD\" -asc_provider \"$PROVIDER_SHORT_NAME\" -f \"$(pwd)/$ITMSP_PATH\" -v eXtreme"
else
    echo -e "${YELLOW}⚠ Could not detect provider, uploading without -asc_provider flag${NC}"
    echo ""
    # Build upload command without provider
    UPLOAD_CMD="$TRANSPORTER_BIN -m upload -u \"$APPLE_ID\" -p \"$APP_SPECIFIC_PASSWORD\" -f \"$(pwd)/$ITMSP_PATH\" -v eXtreme"
fi

# Run upload and capture output
# Use PIPESTATUS to get the actual exit code of iTMSTransporter, not tee
eval "$UPLOAD_CMD" 2>&1 | tee /tmp/transporter_upload.log

UPLOAD_EXIT=${PIPESTATUS[0]}

if [ $UPLOAD_EXIT -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✅ Upload successful!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Go to App Store Connect: https://appstoreconnect.apple.com/"
    echo "2. Navigate to My Apps → Blackhole Sim"
    echo "3. Go to the Build section"
    echo "4. Wait 10-30 minutes for processing"
    echo "5. Select your processed build from the dropdown"
    echo ""
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}❌ Upload failed (exit code: $UPLOAD_EXIT)${NC}"
    echo -e "${RED}========================================${NC}"
    echo ""
    
    # Show error details from log file
    if [ -f /tmp/transporter_upload.log ]; then
        echo -e "${YELLOW}Error details from Transporter:${NC}"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        
        # Show ERROR lines
        ERROR_LINES=$(grep -i "ERROR\|Error\|error\|failed\|Failed\|FAILED" /tmp/transporter_upload.log | tail -20)
        if [ -n "$ERROR_LINES" ]; then
            echo "$ERROR_LINES"
        else
            # If no ERROR lines, show last 30 lines of output
            echo "Last 30 lines of output:"
            tail -30 /tmp/transporter_upload.log
        fi
        
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        
        # Check for specific error patterns
        if grep -qi "Client configuration failed" /tmp/transporter_upload.log; then
            echo -e "${RED}⚠ 'Client configuration failed' - Most Common Cause:${NC}"
            echo ""
            echo -e "${YELLOW}❌ The app likely doesn't exist in App Store Connect yet${NC}"
            echo ""
            echo "This error occurs when:"
            echo "  • The app hasn't been created in App Store Connect"
            echo "  • The bundle ID doesn't match an existing app"
            echo "  • Your Apple ID doesn't have access to the app"
            echo ""
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            echo "REQUIRED: Create the app in App Store Connect first"
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            echo ""
            echo "Steps:"
            echo "1. Go to: https://appstoreconnect.apple.com/"
            echo "2. Select Team ID: ${DETECTED_TEAM_ID:-72583G5MNU} (check team selector top-right)"
            echo "3. Click '+' → 'New App'"
            echo "4. Fill in:"
            echo "   - Platform: macOS"
            echo "   - Name: Blackhole Sim"
            echo "   - Primary Language: (your language)"
            echo "   - Bundle ID: com.blackhole.simulation (MUST match exactly)"
            echo "   - SKU: (any unique ID, e.g., blackhole-sim-001)"
            echo "5. Click 'Create'"
            echo "6. Wait a few minutes for App Store Connect to set up"
            echo "7. Then run: make upload"
            echo ""
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            echo ""
            echo "Other possible causes (less common):"
            echo "  • Bundle ID mismatch with existing app"
            echo "  • Team ID/provider mismatch"
            echo "  • App Store Connect API issues (check status.apple.com)"
            echo ""
        fi
        
        if grep -qi "is not a regular file" /tmp/transporter_upload.log; then
            echo -e "${RED}⚠ 'is not a regular file' Error:${NC}"
            echo ""
            echo "This error occurs when Transporter tries to validate the app bundle."
            echo "App bundles (.app) are directories, not files, which is correct."
            echo ""
            echo "Possible causes:"
            echo "  • Transporter version issue (try updating Transporter)"
            echo "  • App bundle structure issue (verify Contents/MacOS/Info.plist exist)"
            echo "  • Metadata.xml format issue"
            echo ""
            echo "Troubleshooting steps:"
            echo "1. Verify app bundle structure:"
            echo "   ls -la \"$ITMSP_PATH/${APP_NAME}.app/Contents/\""
            echo ""
            echo "2. Check if Transporter is up to date:"
            echo "   Check Mac App Store for Transporter updates"
            echo ""
            echo "3. Try using Transporter GUI instead:"
            echo "   open -a Transporter \"$(pwd)/$ITMSP_PATH\""
            echo ""
            echo "4. Contact Apple Developer Support if issue persists"
            echo ""
        fi
        
        if grep -qi "authentication\|credential\|password" /tmp/transporter_upload.log; then
            echo -e "${YELLOW}Authentication-related error detected${NC}"
            echo "Check your app-specific password"
            echo ""
        fi
    fi
    
    echo "Troubleshooting:"
    echo "1. Verify your app-specific password is correct"
    echo "2. Check that 2FA is enabled on your Apple ID"
    echo "3. Ensure the app is signed with an App Store certificate (not Developer ID)"
    echo "4. Verify the .itmsp package structure is correct"
    echo ""
    echo "Full log saved to: /tmp/transporter_upload.log"
    exit 1
fi

echo "Package location:"
echo "  $(pwd)/$ITMSP_PATH"
echo ""
