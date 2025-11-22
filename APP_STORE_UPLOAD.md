# App Store Upload Status

## Current Status: ✅ 98% Complete!

The upload infrastructure is fully working. Only ONE certificate is missing.

## ✅ What's Working:

1. ✅ App is signed with "3rd Party Mac Developer Application" certificate
2. ✅ App has App Sandbox entitlement (required for App Store)
3. ✅ Info.plist has `LSApplicationCategoryType` 
4. ✅ Minimum macOS version set to 12.0 (supports arm64-only)
5. ✅ `.pkg` installer is created automatically
6. ✅ Transporter authentication and upload works
7. ✅ Team ID detection (72583G5MNU) works
8. ✅ Metadata.xml is correctly formatted

## ⚠️ Missing Requirement:

### "3rd Party Mac Developer Installer" Certificate

**What it is:** A certificate used to sign `.pkg` installers for App Store distribution.

**Why it's needed:** Apple requires the `.pkg` installer to be signed separately from the app itself.

**How to get it:**

1. Go to: https://developer.apple.com/account/resources/certificates/list
2. Click the **"+"** button to create a new certificate
3. Select: **"Mac Installer Distribution"** (under "Software")
4. Follow the prompts to create a Certificate Signing Request (CSR):
   - Open **Keychain Access** → **Certificate Assistant** → **Request a Certificate From a Certificate Authority**
   - Enter your email and name
   - Select "Saved to disk"
   - Upload the CSR to Apple's website
5. Download the certificate and double-click to install it in Keychain
6. Run `make upload` again - the script will automatically detect and use the certificate

### Verification:

After installing the certificate, verify it's available:
```bash
security find-identity -v -p macappstore | grep "Mac Developer Installer"
```

You should see output like:
```
"3rd Party Mac Developer Installer: Mahan Hazrati Sagharchi (72583G5MNU)"
```

## Once Complete:

When you have the installer certificate, just run:
```bash
export TRANSPORTER_APPLE_ID="mahan@selkomark.com"
export TRANSPORTER_PASSWORD="pvcs-kwsy-zasq-wyha"  
make upload
```

The script will:
1. Build the app
2. Sign it with your application certificate ✅
3. Create the `.pkg` installer ✅  
4. Sign the `.pkg` with your installer certificate (currently missing)
5. Upload to App Store Connect ✅
6. Wait for Apple's validation

## Summary:

Everything is automated and working. You just need to create and install the **Mac Installer Distribution** certificate from Apple Developer portal.

