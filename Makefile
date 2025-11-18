# Compiler and flags
CXX := clang++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra
OBJC_FLAGS := -fobjc-arc

# vcpkg configuration
VCPKG_INSTALLED := ./vcpkg/installed/arm64-osx
INCLUDES := -I$(VCPKG_INSTALLED)/include
LDFLAGS := -L$(VCPKG_INSTALLED)/lib
LIBS := -lSDL2 -lSDL2_ttf -lfreetype -lpng16 -lbz2 -lz -lbrotlidec -lbrotlicommon
RPATH := -Wl,-rpath,$(VCPKG_INSTALLED)/lib

# macOS frameworks
FRAMEWORKS := -framework Cocoa -framework IOKit -framework CoreVideo \
              -framework CoreAudio -framework AudioToolbox -framework ForceFeedback \
              -framework Carbon -framework Metal -framework MetalKit \
              -framework Foundation -framework GameController -framework CoreHaptics \
              -liconv

# Directories
BUILD_DIR := build
SRC_DIR := src
SHADER_DIR := shaders

# Source files organized by module
SOURCES := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/core/Application.cpp \
	$(SRC_DIR)/camera/Camera.cpp \
	$(SRC_DIR)/camera/CinematicCamera.cpp \
	$(SRC_DIR)/ui/HUD.cpp \
	$(SRC_DIR)/physics/BlackHole.cpp \
	$(SRC_DIR)/rendering/MetalRTRenderer.mm \
	$(SRC_DIR)/utils/ResolutionManager.cpp

# Object files
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp,$(SOURCES)))
OBJECTS += $(patsubst $(SRC_DIR)/%.mm,$(BUILD_DIR)/%.o,$(filter %.mm,$(SOURCES)))

# Metal shader files
METAL_SOURCE := $(SHADER_DIR)/RayTracing.metal
METAL_AIR := $(BUILD_DIR)/RayTracing.air
METAL_LIB := $(BUILD_DIR)/default.metallib

# Output executable
TARGET := blackhole_sim

# Default target
all: $(TARGET)

# Create build directory structure
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/core $(BUILD_DIR)/camera $(BUILD_DIR)/ui \
	          $(BUILD_DIR)/physics $(BUILD_DIR)/rendering $(BUILD_DIR)/utils

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
$(TARGET): $(METAL_LIB) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS) $(RPATH) $(FRAMEWORKS)

# Run the simulation
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Rebuild from scratch
rebuild: clean all

.PHONY: all run clean rebuild
