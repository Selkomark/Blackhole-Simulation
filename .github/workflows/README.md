# GitHub Actions Workflows

## Build and Package Workflow

The `build-and-package.yml` workflow automatically builds the BlackHoleSim application for both Intel (x86_64) and Apple Silicon (arm64) architectures whenever code is pushed to the `main` branch.

### What it does:

1. **Builds for both architectures** using a matrix strategy:
   - `arm64`: Built on `macos-14` (Apple Silicon)
   - `x86_64`: Built on `macos-13` (Intel)

2. **Installs dependencies** using vcpkg for each architecture

3. **Compiles the application** with architecture-specific settings

4. **Creates app bundles** and DMG files

5. **Uploads artifacts** that can be downloaded from the Actions tab

### Artifacts:

After a successful build, you'll find:
- `BlackHoleSim-arm64-dmg`: DMG file for Apple Silicon Macs
- `BlackHoleSim-x86_64-dmg`: DMG file for Intel Macs
- `BlackHoleSim-arm64-app`: App bundle for Apple Silicon
- `BlackHoleSim-x86_64-app`: App bundle for Intel

### Code Signing:

The workflow currently builds **unsigned** apps. To enable code signing:

1. Export your Developer ID certificate as a `.p12` file
2. Add it as a GitHub secret named `MACOS_CERTIFICATE`
3. Add the certificate password as `MACOS_CERTIFICATE_PASSWORD`
4. Add your signing identity as `MACOS_SIGNING_IDENTITY`
5. Uncomment the signing steps in the workflow

### Manual Triggering:

You can manually trigger the workflow from the Actions tab:
1. Go to Actions → Build and Package
2. Click "Run workflow"
3. Select the branch and click "Run workflow"

### Troubleshooting:

- **Intel builds failing**: GitHub is phasing out Intel macOS runners. If `macos-13` becomes unavailable, remove `x86_64` from the matrix or use a universal build approach.
- **vcpkg installation slow**: Dependencies are cached between runs, but the first run may take 10-20 minutes.
- **DMG creation fails**: Ensure all scripts in `scripts/` are executable (they should be automatically made executable by the workflow).

