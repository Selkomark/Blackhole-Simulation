#!/bin/bash

# Helper script to upload .itmsp package manually
# Use this when automatic upload fails in CI/CD

set -e

ITMSP_PATH="${1:-export/Blackhole Sim.itmsp}"

if [ ! -d "$ITMSP_PATH" ]; then
    echo "❌ Error: .itmsp package not found at: $ITMSP_PATH"
    echo ""
    echo "Usage:"
    echo "  ./scripts/upload_itmsp_manual.sh [path/to/Blackhole Sim.itmsp]"
    echo ""
    echo "Or download from GitHub Actions artifacts and extract, then:"
    echo "  ./scripts/upload_itmsp_manual.sh /path/to/extracted/Blackhole Sim.itmsp"
    exit 1
fi

# Check for Transporter
TRANSPORTER_BIN="/Applications/Transporter.app/Contents/itms/bin/iTMSTransporter"

if [ ! -f "$TRANSPORTER_BIN" ]; then
    echo "❌ Transporter.app not found"
    echo ""
    echo "Please install Transporter from the Mac App Store:"
    echo "  https://apps.apple.com/app/transporter/id1450874784"
    exit 1
fi

# Check for credentials
if [ -z "$TRANSPORTER_APPLE_ID" ] || [ -z "$TRANSPORTER_PASSWORD" ]; then
    echo "⚠️  Credentials not set in environment"
    echo ""
    echo "Set them before running:"
    echo "  export TRANSPORTER_APPLE_ID=\"your@email.com\""
    echo "  export TRANSPORTER_PASSWORD=\"xxxx-xxxx-xxxx-xxxx\""
    echo ""
    echo "Or use Transporter GUI:"
    echo "  open -a Transporter \"$ITMSP_PATH\""
    exit 1
fi

echo "📤 Uploading to App Store Connect..."
echo "Package: $ITMSP_PATH"
echo ""

# Clean extended attributes
xattr -cr "$ITMSP_PATH" 2>/dev/null || true

# Upload
"$TRANSPORTER_BIN" \
  -m upload \
  -u "$TRANSPORTER_APPLE_ID" \
  -p "$TRANSPORTER_PASSWORD" \
  -f "$(cd "$(dirname "$ITMSP_PATH")" && pwd)/$(basename "$ITMSP_PATH")" \
  -v eXtreme

echo ""
echo "✅ Upload complete!"
echo ""
echo "Next steps:"
echo "1. Go to https://appstoreconnect.apple.com/"
echo "2. Navigate to My Apps → Blackhole Sim"
echo "3. Go to the Build section"
echo "4. Wait 10-30 minutes for processing"
echo "5. Select your processed build"

