# macOS Packaging & Distribution Guide

Complete guide for building, signing, and distributing the Black Hole Simulation application for macOS.

## Quick Start

### One-Stop Script (Recommended)
```bash
# Build, sign, and package everything automatically
./sign_package.sh

# Or using Makefile:
make package
```

This script will:
1. Clean previous builds
2. Build the application
3. Create app bundle
4. Attempt to sign (if certificate available)
5. Create DMG file

### Step-by-Step Commands
```bash
# Build everything: clean, build, create app bundle, sign, and create DMG
make release

# Or individual steps:
make          # Build the application
make app      # Create app bundle
make sign     # Sign the app (requires certificate)
make dmg      # Create DMG file
```

## Prerequisites

1. **Xcode Command Line Tools** (for `codesign`, `hdiutil`, etc.)
   ```bash
   xcode-select --install
   ```

2. **Code Signing Certificate** (optional but recommended)
   - For distribution: Developer ID Application certificate
   - For testing: Development certificate
   - Get from: [Apple Developer Portal](https://developer.apple.com/account/)

### Obtaining a Developer ID Application Certificate

To distribute your app outside the App Store, you need a **Developer ID Application** certificate:

#### Step 1: Enroll in Apple Developer Program
- Sign up at [developer.apple.com/programs](https://developer.apple.com/programs/)
- Annual fee: $99 USD
- Required for Developer ID certificates

#### Step 2: Generate a Certificate Signing Request (CSR)
1. Open **Keychain Access** (Applications > Utilities)
2. Go to **Keychain Access** > **Certificate Assistant** > **Request a Certificate From a Certificate Authority**
3. Fill in:
   - **User Email Address**: Your Apple ID email
   - **Common Name**: Your name or organization name
   - **CA Email Address**: Leave blank
   - Select **Saved to disk**
4. Click **Continue** and save the `.certSigningRequest` file

#### Step 3: Create Developer ID Certificate
1. Log in to [Apple Developer Portal](https://developer.apple.com/account/)
2. Navigate to **Certificates, Identifiers & Profiles**
3. Click the **+** button (top left)
4. Under **Software**, select **Developer ID Application**
5. Click **Continue**
6. Upload your CSR file (from Step 2)
7. Click **Continue**, then **Download**
8. Save the `.cer` file

#### Step 4: Install the Certificate

**Method 1: Double-click (Recommended)**
1. Double-click the downloaded `.cer` file
2. It will open in Keychain Access and install automatically
3. Verify installation:
   ```bash
   security find-identity -v -p codesigning
   ```
   You should see: `Developer ID Application: Your Name (TEAM_ID)`

**Method 2: Manual Import via Keychain Access**
If double-clicking fails:
1. Open **Keychain Access** (Applications > Utilities)
2. Select **login** keychain (or **System** if you have admin rights)
3. Go to **File** > **Import Items...**
4. Select your `.cer` file
5. Enter your password if prompted

**Method 3: Command Line Import**
If GUI methods fail:
```bash
security import /path/to/certificate.cer -k ~/Library/Keychains/login.keychain-db
```

**Troubleshooting Import Errors:**

**Error -25294 (Duplicate Item):**
- The certificate may already exist. Check Keychain Access > **My Certificates**
- Delete any existing/expired Developer ID certificates
- Try importing again

**Error -25299 (Invalid Certificate):**
- Re-download the certificate from Apple Developer Portal
- Ensure you're importing the correct certificate type (Developer ID Application, not Development)

**Certificate Not Showing Up:**
- Make sure you're looking in the correct keychain (usually **login**)
- Check **My Certificates** category in Keychain Access
- Verify with: `security find-identity -v -p codesigning`

#### Step 5: Use the Certificate
Once installed, the signing script will automatically detect and use it:
```bash
make sign
```

Or set it manually:
```bash
export MACOS_SIGNING_IDENTITY='Developer ID Application: Your Name (TEAM_ID)'
make sign
```

**Important**: Even with a Developer ID certificate, macOS **requires notarization** for the app to run without user intervention. See the [Notarization](#notarization-required-for-gatekeeper) section below.

**For Testing (Without Notarization):**
If you need to test the app before notarization:
1. Right-click (or Control-click) on `BlackHoleSim.app`
2. Select **Open** from the context menu
3. Click **Open** in the security dialog
4. macOS will remember this choice for future launches

**Note**: Developer ID certificates are valid for 5 years. Keep your CSR file safe for renewals.

## Build Outputs

All build outputs are placed in the `export/` folder:
- `export/blackhole_sim` - Compiled executable
- `export/BlackHoleSim.app` - App bundle
- `export/BlackHoleSim-1.0.dmg` - DMG file

## Building the App Bundle

### Step 1: Build the Application
```bash
make clean
make
```

This creates `export/blackhole_sim`.

### Step 2: Create App Bundle
```bash
make app
```

This creates `export/BlackHoleSim.app` with:
- Executable in `Contents/MacOS/`
- Assets in `Contents/Resources/`
- Info.plist with app metadata
- App icon (if available)

## Code Signing

### Interactive Signing (Recommended)

When you run `make sign` or `./sign_package.sh`, the script will:

1. **Display all available signing identities** from your keychain
2. **Prompt you to select** which identity to use
3. **Sign the application** with your selected identity
4. **Verify the signature** automatically

Example:
```bash
make sign
```

You'll see:
```
Available code signing identities:
-----------------------------------
  [1] Developer ID Application: Your Name (TEAM_ID)
  [2] Apple Development: Your Name (TEAM_ID)

Select identity number (1-2): 1
```

### Manual Signing

**Option 1: Using Environment Variable**
```bash
export MACOS_SIGNING_IDENTITY="Developer ID Application: Your Name (TEAM_ID)"
make sign
```

**Option 2: Skip Signing**
If you don't have a certificate or choose not to sign, the app will still work but macOS may show warnings.

**Note**: For distribution outside the Mac App Store, you need a "Developer ID Application" certificate from Apple Developer.

## Creating DMG for Distribution

### Create DMG File
```bash
make dmg
```

This creates `export/BlackHoleSim-1.0.dmg` with:
- App bundle
- Applications folder symlink
- README (if available)
- Proper window layout

### Manual DMG Customization

The DMG script creates a basic DMG. To customize:

1. Edit `scripts/create_dmg.sh`
2. Add background image to `assets/dmg_background.png` (optional)
3. Customize window layout in the AppleScript section

## Notarization (Required for Gatekeeper)

After signing with a Developer ID certificate, you **must** notarize the app for distribution. Notarization allows macOS Gatekeeper to verify your app without user intervention. Without notarization, users will see a warning that the app cannot be opened.

### Prerequisites
1. **Developer ID Application certificate** (see above)
2. **App-specific password** for your Apple ID:
   - Go to [appleid.apple.com](https://appleid.apple.com)
   - Sign in > **Sign-In and Security** > **App-Specific Passwords**
   - Click **Generate an app-specific password**
   - Name it (e.g., "Notarization") and copy the password
   - **Important**: Use an app-specific password, not your regular Apple ID password

### Automated Notarization (Recommended)

The project includes an automated notarization script that handles all steps:

#### Step 1: Set Up Credentials

**Option A: Keychain Profile (Recommended - Set Once)**

Store your credentials securely in the macOS Keychain:

```bash
xcrun notarytool store-credentials "notary-profile" \
    --apple-id "your@email.com" \
    --team-id "TEAM_ID" \
    --password "app-specific-password"

export NOTARY_KEYCHAIN_PROFILE="notary-profile"
```

**Option B: Environment Variables**

Set environment variables before running notarization:

```bash
export NOTARY_APPLE_ID="mahan@selkomark.com"
export NOTARY_TEAM_ID="72583G5MNU"  # Your Team ID (e.g., "72583G5MNU")
export NOTARY_PASSWORD="app-specific-password"
```

**Option C: Interactive Prompt**

If credentials aren't set, the script will prompt you interactively.

#### Step 2: Run Notarization

```bash
make notarize
```

Or run the script directly:

```bash
./scripts/notarize_app.sh
```

The script will:
1. ✅ Verify the app is signed
2. 📦 Create a zip file for submission
3. 📤 Submit to Apple's notarization service
4. ⏳ Wait for completion (typically 5-15 minutes)
5. 📎 Staple the notarization ticket to the app
6. 🔍 Verify notarization and Gatekeeper status

#### Step 3: Recreate DMG

After successful notarization, recreate the DMG with the notarized app:

```bash
make dmg
```

### Manual Notarization Steps

If you prefer to run the steps manually:

```bash
# 1. Create zip for notarization
cd export
ditto -c -k --keepParent "Blackhole Simulation.app" "Blackhole Simulation.zip"

# 2. Submit for notarization (using keychain profile)
xcrun notarytool submit "Blackhole Simulation.zip" \
    --keychain-profile "notary-profile" \
    --wait

# Or using direct credentials:
xcrun notarytool submit "Blackhole Simulation.zip" \
    --apple-id "your@email.com" \
    --team-id "TEAM_ID" \
    --password "app-specific-password" \
    --wait

# 3. Staple the ticket to the app
xcrun stapler staple "Blackhole Simulation.app"

# 4. Verify notarization
xcrun stapler validate "Blackhole Simulation.app"

# 5. Recreate DMG (with notarized app)
cd ..
make dmg
```

**Note**: Notarization typically takes 5-15 minutes. The `--wait` flag will poll until complete.

### Troubleshooting Notarization

**Error: "Invalid Apple ID or password"**
- Ensure you're using an **app-specific password**, not your regular Apple ID password
- Verify your Apple ID email is correct
- Check that 2FA is enabled on your Apple ID (required for app-specific passwords)

**Error: "Team ID mismatch"**
- Verify your Team ID matches the one in your Developer ID certificate
- Check with: `codesign -dvv "export/Blackhole Simulation.app" | grep TeamIdentifier`

**Error: "Notarization failed"**
- Check the notarization logs:
  ```bash
  xcrun notarytool log <SUBMISSION_ID> --keychain-profile "notary-profile"
  ```
- Common issues:
  - Missing entitlements
  - Unsigned helper tools
  - Hardened runtime violations
  - Missing Info.plist entries

**Gatekeeper still rejects after notarization**
- Wait a few minutes for Apple's CDN to propagate the notarization ticket
- Verify stapling: `xcrun stapler validate "export/Blackhole Simulation.app"`
- Check Gatekeeper: `spctl --assess --verbose "export/Blackhole Simulation.app"`

## Testing

### Test App Bundle

**Important**: If the app is signed with Developer ID but not notarized, macOS Gatekeeper will block it when using `open` or double-clicking. Use one of these methods:

**Method 1: Direct Launch (Recommended for Testing)**
```bash
./scripts/launch_app.sh
# Or directly:
./export/BlackHoleSim.app/Contents/MacOS/BlackHoleSim
```

**Method 2: Bypass Gatekeeper via Finder**
1. Right-click (or Control-click) on `BlackHoleSim.app`
2. Select **Open** from the context menu
3. Click **Open** in the security dialog
4. macOS will remember this choice

**Method 3: Remove Quarantine (Temporary)**
```bash
xattr -dr com.apple.quarantine export/BlackHoleSim.app
open export/BlackHoleSim.app
```

**Note**: For distribution, you must notarize the app (see Notarization section above).

### Test DMG
```bash
open export/BlackHoleSim-1.0.dmg
# Drag app to Applications folder
```

### Verify Signature

The signing script automatically verifies the signature after signing. To manually verify:

```bash
# Basic verification
codesign --verify --verbose export/BlackHoleSim.app

# Detailed signature information
codesign -dvv export/BlackHoleSim.app

# Gatekeeper assessment
spctl --assess --verbose export/BlackHoleSim.app
```

**Expected output for successful signing:**
- `✓ Code signature verified`
- `✓ Gatekeeper assessment passed` (or warning if not notarized)
- Signature details showing Authority, Identifier, and TeamIdentifier

## File Structure

```
export/
├── blackhole_sim                    # Compiled executable
├── BlackHoleSim.app/               # App bundle
│   └── Contents/
│       ├── Info.plist              # App metadata
│       ├── MacOS/
│       │   └── BlackHoleSim         # Executable (copied from export/)
│       └── Resources/
│           ├── assets/              # Game assets
│           └── AppIcon.icns         # App icon
└── BlackHoleSim-1.0.dmg            # Distribution DMG
```

## Troubleshooting

### "App is damaged" Error
- Ensure app is properly signed
- Check code signature: `codesign --verify --verbose export/BlackHoleSim.app`
- May need notarization for distribution

### Icon Not Showing
- Ensure `assets/export/iOS-Default-1024x1024@1x.png` exists
- Check that `iconutil` is available (part of Xcode)

### Signing Fails
- Verify certificate is installed: `security find-identity -v -p codesigning`
- Check certificate hasn't expired
- Ensure you're using the correct identity format

### Build Errors
- Ensure all dependencies are installed via vcpkg
- Check that Xcode Command Line Tools are installed
- Verify Metal shader compilation succeeds

## Environment Variables

- `MACOS_SIGNING_IDENTITY`: Code signing identity (e.g., "Developer ID Application: Name (TEAM_ID)")
- `NOTARY_KEYCHAIN_PROFILE`: Keychain profile name for notarization (recommended)
- `NOTARY_APPLE_ID`: Apple ID email for notarization (alternative to keychain profile)
- `NOTARY_TEAM_ID`: Team ID for notarization (alternative to keychain profile)
- `NOTARY_PASSWORD`: App-specific password for notarization (alternative to keychain profile)

## Distribution Checklist

- [ ] Application builds successfully
- [ ] App bundle created and tested
- [ ] Application signed with Developer ID certificate
- [ ] Application notarized (for Gatekeeper) - **Required for distribution**
- [ ] Notarization verified (`xcrun stapler validate`)
- [ ] Gatekeeper assessment passed (`spctl --assess`)
- [ ] DMG created and tested (with notarized app)
- [ ] DMG tested on clean macOS system
- [ ] Icon displays correctly
- [ ] All assets included in bundle

## Scripts Reference

- `sign_package.sh` - One-stop build, sign, and package script
- `scripts/create_app_bundle.sh` - App bundle creation script
- `scripts/sign_app.sh` - Code signing script
- `scripts/notarize_app.sh` - Automated notarization script
- `scripts/create_dmg.sh` - DMG creation script
- `scripts/launch_app.sh` - Launch app (bypasses Gatekeeper for testing)
- `scripts/entitlements.plist` - Code signing entitlements
