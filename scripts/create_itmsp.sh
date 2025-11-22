#!/bin/bash

# Script to create .itmsp package for App Store Connect upload
# .itmsp (iTunes Metadata Package) is the required format for Transporter

set -e

APP_NAME="Blackhole Simulation"
APP_BUNDLE="export/${APP_NAME}.app"
BUNDLE_ID="com.blackhole.simulation"
VERSION="${APP_VERSION:-1.0}"  # Use APP_VERSION from env if set (for CI/CD)
ITMSP_NAME="${APP_NAME}.itmsp"
ITMSP_PATH="export/${ITMSP_NAME}"

echo "=========================================="
echo "Creating .itmsp package for App Store"
echo "=========================================="
echo ""

# Check if app bundle exists
if [ ! -d "$APP_BUNDLE" ]; then
    echo "❌ App bundle not found: $APP_BUNDLE"
    exit 1
fi

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

echo "✅ Created .itmsp package: $ITMSP_PATH"
echo ""
echo "Contents:"
ls -lh "$ITMSP_PATH"
echo ""
echo "=========================================="
echo "Next Steps:"
echo "=========================================="
echo ""
echo "1. Open Transporter.app:"
echo "   open -a Transporter"
echo ""
echo "2. Drag and drop this folder into Transporter:"
echo "   $(pwd)/$ITMSP_PATH"
echo ""
echo "3. Click 'Deliver'"
echo ""
echo "Or open Transporter with the .itmsp directly:"
echo "   open -a Transporter \"$(pwd)/$ITMSP_PATH\""
echo ""

