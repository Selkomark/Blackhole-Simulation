# App Store Connect Release Guide

Complete guide for building, signing, and uploading Black Hole Simulation to the Mac App Store using `xcrun altool`.

## Prerequisites

### 1. Apple Developer Account
- Active Apple Developer Program membership ($99/year)
- Access to [App Store Connect](https://appstoreconnect.apple.com/)
- Team ID: `YOUR_TEAM_ID` (find yours at [developer.apple.com/account](https://developer.apple.com/account))

### 2. Transporter App
- **Required**: Install [Transporter](https://apps.apple.com/app/transporter/id1450874784) from the Mac App Store
- This is the recommended tool for macOS App Store uploads
- Alternative: Use Xcode Organizer (Window → Organizer → Distribute App)

**Note**: For GitHub Actions CI/CD, the `.itmsp` package is created automatically as an artifact that can be downloaded and uploaded manually via Transporter.

#### Step 1: Generate API Key
1. Log in to [App Store Connect](https://appstoreconnect.apple.com/)
2. Go to **Users and Access** → **Keys** tab
3. Click **+** (Generate API Key)
4. Enter a name: `Blackhole Simulation Upload`
5. Select **App Manager** or **Admin** role
6. Click **Generate**
7. **Download the `.p8` key file** immediately (you can only download it once!)
8. **Copy the Key ID** (shown after generation)
9. **Copy the Issuer ID** (found at the top of the Keys page)

#### Step 2: Store API Key Securely
```bash
# Store the .p8 key file in a secure location
mkdir -p ~/.appstoreconnect
mv ~/Downloads/AuthKey_XXXXXXXX.p8 ~/.appstoreconnect/
chmod 600 ~/.appstoreconnect/AuthKey_XXXXXXXX.p8
```

#### Step 3: Set Environment Variables
Add to your `~/.zshrc` or `~/.bash_profile`:
```bash
export APPSTORE_API_KEY_PATH="$HOME/.appstoreconnect/AuthKey_XXXXXXXX.p8"
export APPSTORE_API_KEY_ID="YOUR_KEY_ID"
export APPSTORE_ISSUER_ID="YOUR_ISSUER_ID"
```

Then reload:
```bash
source ~/.zshrc  # or source ~/.bash_profile
```

### 3. App Store Certificate

You need an **App Store** certificate (not Developer ID):

#### Check Existing Certificates
```bash
security find-identity -v -p codesigning
```

Look for:
- ✅ `Apple Distribution: Your Name (YOUR_TEAM_ID)` - **Use this**
- ✅ `3rd Party Mac Developer Application: Your Name (YOUR_TEAM_ID)` - **Also valid**
- ❌ `Developer ID Application: ...` - **Wrong type** (for direct distribution only)

#### Create App Store Certificate (if needed)
1. Go to [Apple Developer Portal](https://developer.apple.com/account/)
2. **Certificates, Identifiers & Profiles** → **Certificates**
3. Click **+** → **Mac App Distribution** (or **Mac Development** for testing)
4. Upload CSR (create via Keychain Access → Certificate Assistant)
5. Download and install the certificate

## Build and Prepare App

### Step 1: Clean and Build
```bash
# Clean previous builds
make clean

# Build the application
make

# Create app bundle
make app
```

### Step 2: Sign with App Store Certificate

**Important**: Use an **App Store** certificate, not Developer ID!

```bash
# Option A: Interactive selection
make sign

# Option B: Specify App Store certificate manually
export MACOS_SIGNING_IDENTITY="Apple Distribution: Your Name (YOUR_TEAM_ID)"
make sign
```

Verify signature:
```bash
codesign -dvv "export/Blackhole Simulation.app"
```

You should see:
```
Authority=Apple Distribution: Your Name (YOUR_TEAM_ID)
```

### Step 3: Create .itmsp Package for Upload

For App Store submission, you need to create an `.itmsp` (iTunes Metadata Package) file. This is done automatically by the upload script:

```bash
make upload
```

The script creates:
- `.itmsp` directory containing your app bundle
- `metadata.xml` file with app metadata

**Manual creation** (if needed):
```bash
./scripts/create_itmsp.sh
```

## Upload to App Store Connect

### Step 1: Verify API Key Setup
```bash
# Check environment variables are set
echo "Key Path: $APPSTORE_API_KEY_PATH"
echo "Key ID: $APPSTORE_API_KEY_ID"
echo "Issuer ID: $APPSTORE_ISSUER_ID"

# Verify key file exists
ls -la "$APPSTORE_API_KEY_PATH"
```

### Step 2: Create .itmsp Package and Upload via Transporter

**Recommended Method: Use the automated script**

```bash
make upload
```

This will:
1. Build and sign your app (if needed)
2. Create a `.itmsp` package (iTunes Metadata Package)
3. Open Transporter.app automatically

**Manual Method:**

```bash
# Create .itmsp package
./scripts/create_itmsp.sh

# Open Transporter with the package
open -a Transporter "export/Blackhole Simulation.itmsp"
```

### Step 3: Upload via Transporter GUI

1. **Transporter should open automatically** with your `.itmsp` package
2. **Sign in** with your Apple ID (if prompted)
3. **Click "DELIVER"** to upload to App Store Connect
4. **Wait for upload** to complete (shows progress in Transporter)

**Note**: Upload typically takes 5-15 minutes depending on file size and network speed.

## Post-Upload Steps

### Step 1: Wait for Processing
1. Go to [App Store Connect](https://appstoreconnect.apple.com/)
2. Navigate to **My Apps** → **Blackhole Simulation**
3. Go to the **Build** section
4. Wait for processing (typically 10-30 minutes)
5. You'll see status: "Processing" → "Ready to Submit"

### Step 2: Select Build
1. Once processing completes, refresh the Build section
2. Select your processed build from the dropdown
3. The build will be associated with your app version

### Step 3: Complete App Store Listing
Ensure all required fields are filled:
- ✅ App description
- ✅ Screenshots (required)
- ✅ App preview (optional but recommended)
- ✅ Keywords
- ✅ Support URL
- ✅ Privacy Policy URL
- ✅ Age rating
- ✅ Category (Education)

### Step 4: Submit for Review
1. Review all information
2. Click **Add for Review** or **Submit for Review**
3. Answer any additional questions
4. Submit

## Troubleshooting

### Error: "Cannot find the ITMSP file"
- Ensure you're uploading the `.itmsp` folder (not `.zip` or `.pkg`)
- Run `./scripts/create_itmsp.sh` to create the package
- Drag the entire `.itmsp` folder into Transporter (not just the contents)

### Error: "Transporter.app not found"
- Install Transporter from the Mac App Store
- Or use Xcode Organizer (Window → Organizer → Distribute App)

### Error: "Certificate not found"
```bash
# List available certificates
security find-identity -v -p codesigning

# Ensure you're using App Store certificate, not Developer ID
# Look for "Apple Distribution" or "3rd Party Mac Developer Application"
```

### Error: "App is not signed"
```bash
# Re-sign the app
make sign

# Verify signature
codesign -dvv "export/Blackhole Simulation.app"
spctl --assess --verbose "export/Blackhole Simulation.app"
```

### Upload Fails: "Invalid bundle" or "Can't find Info.plist"
- Ensure the app is properly signed with an App Store certificate
- Verify `Info.plist` exists in `export/Blackhole Simulation.app/Contents/Info.plist`
- Check that `CFBundleIdentifier` matches your App ID: `com.blackhole.simulation`
- Verify version numbers match App Store Connect
- Try recreating the `.itmsp` package: `./scripts/create_itmsp.sh`

### Build Not Appearing in App Store Connect
- Wait 10-30 minutes for processing
- Check email for processing errors
- Verify the bundle ID matches your App ID in App Store Connect
- Ensure version number is higher than previous submissions

## Quick Reference

### Environment Variables
```bash
export APPSTORE_API_KEY_PATH="$HOME/.appstoreconnect/AuthKey_XXXXXXXX.p8"
export APPSTORE_API_KEY_ID="YOUR_KEY_ID"
export APPSTORE_ISSUER_ID="YOUR_ISSUER_ID"
```

### Upload Command
```bash
# Automated: Creates .itmsp and opens Transporter
make upload

# Or manually:
./scripts/create_itmsp.sh
open -a Transporter "export/Blackhole Simulation.itmsp"
```

### Verification Commands
```bash
# Check certificate
security find-identity -v -p codesigning

# Verify app signature
codesign -dvv "export/Blackhole Simulation.app"

# Check Gatekeeper
spctl --assess --verbose "export/Blackhole Simulation.app"
```

## Important Notes

1. **No Notarization Required**: For App Store submissions, Apple handles notarization during review. You don't need to notarize before uploading.

2. **App Store Certificate Only**: Use an App Store certificate (`Apple Distribution`), not a Developer ID certificate.

3. **API Key Security**: Keep your `.p8` API key file secure. Never commit it to git. Store it in `~/.appstoreconnect/` with `chmod 600`.

4. **Version Numbers**: Each submission must have a higher version number than the previous one.

5. **Processing Time**: Allow 10-30 minutes for Apple to process your upload before it appears in App Store Connect.

## Checklist

Before uploading:
- [ ] App Store certificate installed and verified (`security find-identity -v -p codesigning`)
- [ ] App built and signed (`make clean && make && make app && make sign`)
- [ ] Signature verified (`codesign -dvv`)
- [ ] `.itmsp` package created (`make upload` or `./scripts/create_itmsp.sh`)
- [ ] Transporter.app installed (from Mac App Store)
- [ ] App Store Connect listing completed (description, screenshots, etc.)

After uploading:
- [ ] Upload completed successfully
- [ ] Build appears in App Store Connect (after processing)
- [ ] Build selected in app version
- [ ] All metadata completed
- [ ] Submitted for review

---

**Last Updated**: January 2025  
**App Bundle ID**: `com.blackhole.simulation`  
**Team ID**: `YOUR_TEAM_ID` (find yours at [developer.apple.com/account](https://developer.apple.com/account))

