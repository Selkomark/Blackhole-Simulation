CXX = clang++
OBJCXX = clang++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -I./vcpkg/installed/arm64-osx/include
OBJCXXFLAGS = -std=c++20 -O3 -Wall -Wextra -I./vcpkg/installed/arm64-osx/include -fobjc-arc
LDFLAGS = -L./vcpkg/installed/arm64-osx/lib -lSDL2 -lSDL2_ttf -lfreetype -lpng16 -lbz2 -lz -lbrotlidec -lbrotlicommon -Wl,-rpath,./vcpkg/installed/arm64-osx/lib \
          -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio -framework AudioToolbox \
          -framework ForceFeedback -framework Carbon -framework Metal -framework MetalKit -framework Foundation \
          -framework GameController -framework CoreHaptics -liconv

SRC_DIR = src
SHADER_DIR = shaders
BUILD_DIR = build
TARGET = blackhole_sim

CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
MM_SRCS = $(wildcard $(SRC_DIR)/*.mm)
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SRCS))
MM_OBJS = $(patsubst $(SRC_DIR)/%.mm, $(BUILD_DIR)/%.o, $(MM_SRCS))
OBJS = $(CPP_OBJS) $(MM_OBJS)

METAL_SRC = $(SHADER_DIR)/RayTracing.metal
METAL_AIR = $(BUILD_DIR)/RayTracing.air
METAL_LIB = $(BUILD_DIR)/default.metallib

all: $(METAL_LIB) $(TARGET)

$(METAL_AIR): $(METAL_SRC)
	@mkdir -p $(BUILD_DIR)
	xcrun -sdk macosx metal -c $(METAL_SRC) -o $(METAL_AIR)

$(METAL_LIB): $(METAL_AIR)
	xcrun -sdk macosx metallib $(METAL_AIR) -o $(METAL_LIB)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.mm
	@mkdir -p $(BUILD_DIR)
	$(OBJCXX) $(OBJCXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
