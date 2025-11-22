# Compiler and flags
CXX := clang++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra
OBJC_FLAGS := -fobjc-arc

# vcpkg configuration
VCPKG_INSTALLED := ./vcpkg_installed/arm64-osx
INCLUDES := -I$(VCPKG_INSTALLED)/include
LDFLAGS := -L$(VCPKG_INSTALLED)/lib
LIBS := -lSDL2 -lSDL2_ttf -lSDL2_mixer -lvorbisfile -lvorbis -logg -lwavpack \
        -lfreetype -lpng16 -lbz2 -lz -lbrotlidec -lbrotlicommon \
        -lavcodec -lavformat -lavutil -lswscale -lswresample -lavfilter -lavdevice
RPATH := -Wl,-rpath,$(VCPKG_INSTALLED)/lib

# macOS frameworks
FRAMEWORKS := -framework Cocoa -framework IOKit -framework CoreVideo \
              -framework CoreAudio -framework AudioToolbox -framework ForceFeedback \
              -framework Carbon -framework Metal -framework MetalKit \
              -framework Foundation -framework GameController -framework CoreHaptics \
              -framework VideoToolbox -framework CoreMedia -framework AVFoundation \
              -liconv

# Directories
BUILD_DIR := build
SRC_DIR := src
SHADER_DIR := shaders
EXPORT_DIR := export

# Source files organized by module
SOURCES := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/core/Application.cpp \
	$(SRC_DIR)/camera/Camera.cpp \
	$(SRC_DIR)/camera/CinematicCamera.cpp \
	$(SRC_DIR)/ui/HUD.cpp \
	$(SRC_DIR)/physics/BlackHole.cpp \
	$(SRC_DIR)/rendering/MetalRTRenderer.mm \
	$(SRC_DIR)/utils/ResolutionManager.cpp \
	$(SRC_DIR)/utils/VideoRecorder.cpp \
	$(SRC_DIR)/utils/Screenshot.cpp \
	$(SRC_DIR)/utils/SaveDialog.mm \
	$(SRC_DIR)/utils/IconLoader.mm

# Object files
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp,$(SOURCES)))
OBJECTS += $(patsubst $(SRC_DIR)/%.mm,$(BUILD_DIR)/%.o,$(filter %.mm,$(SOURCES)))

# Metal shader files
METAL_SOURCE := $(SHADER_DIR)/RayTracing.metal
METAL_AIR := $(BUILD_DIR)/RayTracing.air
METAL_LIB := $(BUILD_DIR)/default.metallib

# Output executable
TARGET := $(EXPORT_DIR)/blackhole_sim

# Default target
all: $(EXPORT_DIR) $(TARGET)

# Create build directory structure
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/core $(BUILD_DIR)/camera $(BUILD_DIR)/ui \
	          $(BUILD_DIR)/physics $(BUILD_DIR)/rendering $(BUILD_DIR)/utils

# Create export directory
$(EXPORT_DIR):
	@mkdir -p $(EXPORT_DIR)

# Compile Metal shaders
$(METAL_AIR): $(METAL_SOURCE) | $(BUILD_DIR)
	xcrun -sdk macosx metal -c $< -o $@

$(METAL_LIB): $(METAL_AIR)
	xcrun -sdk macosx metallib $< -o $@

# Compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile Objective-C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.mm | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJC_FLAGS) -c $< -o $@

# Link executable
$(TARGET): $(METAL_LIB) $(OBJECTS) | $(EXPORT_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS) $(RPATH) $(FRAMEWORKS)

# Run the simulation
run: $(TARGET)
	./$(TARGET)

# Launch app bundle (bypasses Gatekeeper for unnotarized apps)
launch: app
	@./scripts/launch_app.sh

# Create macOS app bundle
app: $(TARGET)
	@./scripts/create_app_bundle.sh

# Sign the app bundle (requires MACOS_SIGNING_IDENTITY env var)
sign: app
	./scripts/sign_app.sh

# Notarize the app bundle (requires NOTARY_KEYCHAIN_PROFILE or NOTARY_APPLE_ID/TEAM_ID/PASSWORD)
notarize: sign
	@./scripts/notarize_app.sh

# Prepare .itmsp package for App Store Connect upload via Transporter
upload: sign
	@./scripts/upload_to_appstore.sh

# Create DMG for distribution
dmg: app
	@./scripts/create_dmg.sh

# Full release build: build, create app, sign, and create DMG
release: clean all sign dmg

# One-stop sign and package script
package: clean
	@./sign_package.sh

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(EXPORT_DIR)

# Rebuild from scratch
rebuild: clean all

.PHONY: all run clean rebuild app sign notarize upload dmg release package
