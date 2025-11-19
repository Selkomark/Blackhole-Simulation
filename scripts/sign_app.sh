#!/bin/bash

# Script to code sign macOS app bundle with interactive identity selection
set -e

EXPORT_DIR="export"
APP_NAME="BlackHoleSim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "❌ Error: App bundle not found. Run 'make app' first."
    exit 1
fi

# Function to select signing identity interactively
select_signing_identity() {
    # Send all prompts to stderr so they're visible even in command substitution
    echo "" >&2
    echo "🔍 Searching for available code signing identities..." >&2
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" >&2
    
    # Get all available identities (including hash and full identity string)
    local temp_file=$(mktemp)
    security find-identity -v -p codesigning 2>/dev/null | grep -E "^\s+[0-9]+\)" > "$temp_file" || true
    
    if [ ! -s "$temp_file" ]; then
        rm -f "$temp_file"
        echo "❌ No code signing identities found." >&2
        echo "" >&2
        echo "📝 To create a signing identity:" >&2
        echo "  1. Go to https://developer.apple.com/account" >&2
        echo "  2. Create a Developer ID Application certificate (for distribution)" >&2
        echo "     or Apple Development certificate (for testing)" >&2
        echo "  3. Download and install it in Keychain Access" >&2
        return 1
    fi
    
    # Parse and display identities
    local count=1
    local identity_map=()
    while IFS= read -r line; do
        # Extract the identity string (everything between quotes)
        local identity=$(echo "$line" | sed -n 's/.*"\(.*\)".*/\1/p')
        if [ -n "$identity" ]; then
            echo "  [$count] $identity" >&2
            identity_map+=("$identity")
            ((count++))
        fi
    done < "$temp_file"
    rm -f "$temp_file"
    
    if [ ${#identity_map[@]} -eq 0 ]; then
        echo "❌ No valid identities found." >&2
        return 1
    fi
    
    # Auto-select if only one identity is available
    if [ ${#identity_map[@]} -eq 1 ]; then
        echo "" >&2
        echo "✅ Auto-selected single available identity:" >&2
        echo "   ${identity_map[0]}" >&2
        echo "${identity_map[0]}"
        return 0
    fi
    
    # Multiple identities - prompt for selection
    echo "" >&2
    echo "📋 Multiple identities found. Please select one:" >&2
    echo "" >&2
    
    # Prompt to stderr, read from appropriate source
    local max_attempts=3
    local attempt=0
    
    while [ $attempt -lt $max_attempts ]; do
        echo -n "👉 Select identity number (1-${#identity_map[@]}): " >&2
        
        # Try to read from /dev/tty first (for interactive terminal), fallback to stdin
        local selection=""
        if [ -c /dev/tty ] 2>/dev/null; then
            read -r selection < /dev/tty 2>/dev/null || read -r selection
        else
            read -r selection
        fi
        
        # Check if input is empty
        if [ -z "$selection" ]; then
            ((attempt++))
            echo "⚠️  Empty input. Please enter a number between 1 and ${#identity_map[@]}." >&2
            if [ $attempt -lt $max_attempts ]; then
                echo "   Attempt $attempt of $max_attempts" >&2
            fi
            continue
        fi
        
        # Validate selection is numeric
        if ! [[ "$selection" =~ ^[0-9]+$ ]]; then
            ((attempt++))
            echo "⚠️  Invalid input: '$selection'. Please enter a number." >&2
            if [ $attempt -lt $max_attempts ]; then
                echo "   Attempt $attempt of $max_attempts" >&2
            fi
            continue
        fi
        
        # Validate selection is in range
        if [ "$selection" -lt 1 ] || [ "$selection" -gt ${#identity_map[@]} ]; then
            ((attempt++))
            echo "⚠️  Invalid selection: $selection. Please choose a number between 1 and ${#identity_map[@]}." >&2
            if [ $attempt -lt $max_attempts ]; then
                echo "   Attempt $attempt of $max_attempts" >&2
            fi
            continue
        fi
        
        # Valid selection - return it
        local selected_index=$((selection - 1))
        echo "✅ Selected: ${identity_map[$selected_index]}" >&2
        echo "${identity_map[$selected_index]}"
        return 0
    done
    
    # All attempts failed
    echo "" >&2
    echo "❌ Failed to select identity after $max_attempts attempts." >&2
    return 1
}

# Check for signing identity
IDENTITY="${MACOS_SIGNING_IDENTITY:-}"

# If not set via environment variable, interactively select
if [ -z "$IDENTITY" ]; then
    # Call function - prompts go to stderr, result goes to stdout
    IDENTITY=$(select_signing_identity)
    
    if [ -z "$IDENTITY" ]; then
        echo ""
        echo "❌ Error: No signing identity available or selected."
        echo ""
        echo "📝 To sign manually, set MACOS_SIGNING_IDENTITY environment variable:"
        echo "  export MACOS_SIGNING_IDENTITY='Developer ID Application: Your Name (TEAM_ID)'"
        echo "  make sign"
        echo ""
        echo "Or create a signing identity:"
        echo "  1. Go to https://developer.apple.com/account"
        echo "  2. Create a Developer ID Application certificate (for distribution)"
        echo "     or Apple Development certificate (for testing)"
        echo "  3. Download and install it in Keychain Access"
        exit 1
    fi
fi

echo ""
echo "🔐 Signing app with identity: ${IDENTITY}"
echo ""

# Sign the executable
echo "✍️  Signing application..."
if ! codesign --force --deep --sign "${IDENTITY}" \
    --options runtime \
    --entitlements scripts/entitlements.plist \
    "${APP_BUNDLE}" 2>&1; then
    echo ""
    echo "❌ Signing failed!"
    exit 1
fi

echo ""
echo "🔍 Verifying signature..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Verify signature with detailed output
VERIFY_OUTPUT=$(codesign --verify --verbose "${APP_BUNDLE}" 2>&1)
VERIFY_EXIT=$?

if [ $VERIFY_EXIT -ne 0 ]; then
    echo "❌ Signature verification failed!"
    echo "$VERIFY_OUTPUT"
    exit 1
fi

echo "✅ Code signature verified"
echo ""

# Check signature details
echo "📋 Signature details:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
codesign -dvv "${APP_BUNDLE}" 2>&1 | grep -E "Authority|Identifier|Format|Signed|TeamIdentifier" || true
echo ""

# Verify with spctl (Gatekeeper assessment)
echo "🛡️  Gatekeeper assessment:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
SPCTL_OUTPUT=$(spctl --assess --verbose "${APP_BUNDLE}" 2>&1)
SPCTL_EXIT=$?

if [ $SPCTL_EXIT -eq 0 ]; then
    echo "✅ Gatekeeper assessment passed"
else
    echo "⚠️  Gatekeeper assessment: $SPCTL_OUTPUT"
    echo "   (This is normal for apps not notarized yet)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ App signed successfully!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📦 Signed app: ${APP_BUNDLE}"
echo "🔑 Identity: ${IDENTITY}"
echo ""
