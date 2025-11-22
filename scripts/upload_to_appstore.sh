#!/bin/bash

# Script to prepare Black Hole Simulation for App Store Connect upload
# Creates .itmsp package for use with Transporter GUI
# Requires: App Store certificate

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
APP_NAME="Blackhole Simulation"
APP_BUNDLE="export/${APP_NAME}.app"
ITMSP_NAME="${APP_NAME}.itmsp"
ITMSP_PATH="export/${ITMSP_NAME}"
BUNDLE_ID="com.blackhole.simulation"
VERSION="1.0"

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
else
    echo -e "${YELLOW}⚠ Warning: Could not verify certificate type${NC}"
    echo "  $SIGNING_INFO"
fi

echo ""

# Create .itmsp package
echo -e "${BLUE}Creating .itmsp package for Transporter...${NC}"

# Remove old .itmsp if exists
if [ -d "$ITMSP_PATH" ]; then
    echo "Removing old .itmsp package..."
    rm -rf "$ITMSP_PATH"
fi

# Create .itmsp directory
mkdir -p "$ITMSP_PATH"

# Copy app bundle into .itmsp
echo "Copying app bundle..."
cp -R "$APP_BUNDLE" "$ITMSP_PATH/"

# Create metadata.xml
echo "Creating metadata.xml..."
cat > "$ITMSP_PATH/metadata.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<package version="software5.7" xmlns="http://apple.com/itunes/importer">
    <software_assets apple_id="${BUNDLE_ID}">
        <asset type="bundle">
            <data_file>
                <file_name>${APP_NAME}.app</file_name>
            </data_file>
        </asset>
    </software_assets>
</package>
EOF

echo -e "${GREEN}✓ Created .itmsp package: $ITMSP_PATH${NC}"
echo ""

# Check if Transporter is available
if [ -d "/Applications/Transporter.app" ]; then
    echo -e "${BLUE}Opening Transporter with .itmsp package...${NC}"
    open -a Transporter "$ITMSP_PATH"
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Ready for upload!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Transporter should now be open with your app."
    echo ""
    echo "Next steps:"
    echo "1. Sign in to Transporter with your Apple ID (if prompted)"
    echo "2. Click 'DELIVER' to upload to App Store Connect"
    echo "3. Wait for upload to complete"
    echo "4. Go to App Store Connect and wait 10-30 minutes for processing"
    echo "5. Select your processed build in the Build section"
    echo ""
else
    echo -e "${YELLOW}⚠ Transporter.app not found${NC}"
    echo ""
    echo "Please install Transporter from the Mac App Store, then:"
    echo "1. Open Transporter.app"
    echo "2. Drag and drop this folder:"
    echo "   $(pwd)/$ITMSP_PATH"
    echo "3. Click 'DELIVER'"
    echo ""
fi

echo "Package location:"
echo "  $(pwd)/$ITMSP_PATH"
echo ""
