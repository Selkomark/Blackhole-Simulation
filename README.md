# Black Hole Simulation

A real-time GPU-accelerated black hole simulation using C++20 with Metal ray tracing and accurate physics equations. Features gravitational lensing, volumetric accretion disk rendering, and cinematic camera movements.

![Black Hole Simulation](https://img.shields.io/badge/C%2B%2B-20-blue) ![Platform](https://img.shields.io/badge/platform-macOS-lightgrey) ![GPU](https://img.shields.io/badge/Metal-GPU-orange)

## Features

- **GPU Accelerated**: Real-time Metal compute shader ray tracing for maximum performance
- **Dynamic Resolution**: Renders at any window size, adapts to screen resolution
- **Fullscreen Support**: Toggle fullscreen mode with F key, ESC to exit
- **Resizable Window**: Drag window edges to resize, rendering adapts automatically
- **Accurate Physics**: Schwarzschild metric geodesic integration using RK4 (Runge-Kutta 4th order)
- **Gravitational Lensing**: Light rays bend according to general relativity equations
- **Volumetric Rendering**: Realistic accretion disk with white-hot temperature gradients
- **Cinematic Camera**: 5 cinematic modes including smooth orbit, wave motion, rising spiral, and close fly-by
- **Continuous Animation**: Camera is always in motion for dynamic viewing experience
- **Real-time Performance**: 30+ FPS at any resolution on Apple Silicon

## Prerequisites

- **macOS** with **Apple Silicon** (M1/M2/M3/M4) or Intel with Metal support
- **Xcode Command Line Tools** (for clang++ and Metal compiler)
- **Homebrew** (for installing dependencies)

> **⚠️ GPU Required**: This simulation requires Metal GPU acceleration and will not run without it.

## Installation

### 1. Clone the Repository

```bash
cd /path/to/your/projects
git clone <your-repo-url> blackhole_simulation
cd blackhole_simulation
```

### 2. Install vcpkg and Dependencies

The project uses vcpkg for dependency management. The setup script will:
- Download and bootstrap vcpkg locally
- Install SDL2

```bash
# Download vcpkg
curl -L -o vcpkg.zip https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip
unzip -q vcpkg.zip
mv vcpkg-master vcpkg

# Bootstrap vcpkg
./vcpkg/bootstrap-vcpkg.sh

# Install SDL2
./vcpkg/vcpkg install sdl2
```

### 3. Build the Project

```bash
make
```

## Running the Simulation

### Quick Run

```bash
./blackhole_sim
```

The simulation window will open with the camera automatically orbiting the black hole.

### Build and Run Script

For convenience, use the build-and-run script:

```bash
./run.sh
```

This will automatically build the project and then run the simulation.

### Auto-Rebuild on File Changes

To automatically rebuild and run the simulation whenever you make changes to the code:

```bash
./watch.sh
```

This script watches for changes in `src/`, `shaders/`, `include/`, and `Makefile`, then automatically rebuilds and restarts the simulation. Requires `fswatch` (install with `brew install fswatch`).

**Note**: The watch script will kill any existing simulation instance before starting a new one.

### Using Cursor

**Quick Run Script:**
```bash
./cursor-run.sh
```

This script builds and runs the simulation with helpful output. You can execute it directly from Cursor's integrated terminal.

**Watch Mode (Auto-rebuild on changes):**
```bash
./watch.sh
```

This will automatically rebuild and restart the simulation whenever you save changes to source files, shaders, or the Makefile.

### Using VS Code Tasks

If you're using VS Code, you can use the built-in tasks:

- **Build**: `Cmd+Shift+B` (or `Ctrl+Shift+B`) - Builds the project
- **Run Task**: `Cmd+Shift+P` → "Tasks: Run Task" → Select:
  - `Run Black Hole Simulation` - Builds and runs once
  - `Watch and Auto-Run` - Watches for changes and auto-rebuilds/runs
  - `Clean Build` - Cleans and rebuilds from scratch

## Controls

| Key | Action |
|-----|--------|
| **F** | Toggle fullscreen mode |
| **C** | Cycle through cinematic camera modes |
| **W** | Move camera forward (Manual mode only) |
| **A** | Move camera left (Manual mode only) |
| **S** | Move camera backward (Manual mode only) |
| **D** | Move camera right (Manual mode only) |
| **Space** | Move camera up (Manual mode only) |
| **Shift** | Move camera down (Manual mode only) |
| **R** | Reset camera position |
| **Tab** | Toggle control hints overlay |
| **ESC** | Exit fullscreen (if in fullscreen) or quit |
| **Q** | Quit simulation |

### Cinematic Camera Modes

Press **C** to cycle through these modes:

1. **Manual Control** - Gentle auto-orbit with manual control override (camera always moves)
2. **Smooth Orbit** - Elegant circular motion with subtle height variations
3. **Wave Motion** - Figure-8 pattern with dramatic height changes
4. **Rising Spiral** - Spiraling upward with varying orbital radius
5. **Close Fly-by** - Fast, close orbit with dynamic height changes

> **Note**: The camera is **always in motion** for a dynamic viewing experience. Even in Manual mode, there's a gentle background orbit that you can control with WASD keys.

## Project Structure

The project is organized by domain/responsibility for better maintainability:

```
blackhole_simulation/
├── src/
│   ├── main.cpp                    # Entry point (minimal)
│   ├── core/
│   │   └── Application.cpp         # Main application loop, SDL, event handling
│   ├── camera/
│   │   ├── Camera.cpp              # Base camera struct
│   │   └── CinematicCamera.cpp    # Cinematic camera system (5 modes)
│   ├── ui/
│   │   └── HUD.cpp                 # Heads-up display, overlay rendering
│   ├── physics/
│   │   └── BlackHole.cpp           # Physics, geodesic integration
│   └── rendering/
│       └── MetalRTRenderer.mm      # Metal GPU ray tracing renderer
│
├── include/
│   ├── core/
│   │   └── Application.hpp
│   ├── camera/
│   │   ├── Camera.hpp
│   │   └── CinematicCamera.hpp
│   ├── ui/
│   │   └── HUD.hpp
│   ├── physics/
│   │   └── BlackHole.hpp
│   ├── rendering/
│   │   └── MetalRTRenderer.h
│   └── utils/
│       └── Vector3.hpp             # Math utilities
│
├── shaders/
│   └── RayTracing.metal            # Metal compute shader
│
├── build/                          # Build artifacts (organized by module)
├── vcpkg/                          # Dependency manager
├── Makefile                        # Build system
└── README.md
```

### Module Responsibilities

- **Core**: Application lifecycle, SDL window/renderer, main loop, event handling
- **Camera**: Camera system with base Camera struct and CinematicCamera controller
- **UI**: HUD rendering, on-screen hints, text display
- **Physics**: BlackHole simulation, Schwarzschild geodesics, RK4 integration
- **Rendering**: Metal GPU ray tracing implementation
- **Utils**: Shared utilities like Vector3 math

### Key Benefits

- **Separation of Concerns**: Each module has a single responsibility
- **Maintainability**: Easy to locate and modify specific functionality
- **Testability**: Modules can be tested independently
- **Scalability**: New features can be added without affecting existing code

## Technical Details

### Physics Implementation

- **Schwarzschild Metric**: Models spacetime curvature around a non-rotating black hole
- **RK4 Integration**: 4th-order Runge-Kutta method for numerical stability
- **Adaptive Step Size**: Dynamically adjusts based on distance to event horizon

### Rendering Pipeline

1. **Ray Generation**: Each pixel generates a ray from the camera
2. **Geodesic Tracing**: Ray path is integrated through curved spacetime
3. **Volumetric Sampling**: Accretion disk density is sampled along the ray
4. **Light Accumulation**: Beer's Law for absorption, temperature-based emission
5. **Tone Mapping**: HDR to LDR conversion with gamma correction

### Performance Optimizations

- **Metal GPU Acceleration**: Parallel ray tracing on thousands of GPU cores
- **Compute Shaders**: Optimized Metal shaders for maximum throughput
- **Adaptive Step Size**: Dynamically adjusts integration steps based on curvature
- **Efficient Memory**: Shared memory for camera data, streaming texture updates

## Troubleshooting

### Build Errors

**Problem**: `SDL2/SDL.h` file not found
```bash
# Ensure SDL2 is installed via vcpkg
./vcpkg/vcpkg install sdl2
```

**Problem**: Linker errors about missing frameworks
```bash
# The Makefile should include all necessary frameworks
# If issues persist, check that you're on macOS
```

### Performance Issues

**Low FPS (<10 FPS)**:
- Reduce window resolution (edit `WIDTH` and `HEIGHT` in `main.cpp`)
- Increase `stepSize` in `BlackHole::trace()` (trade accuracy for speed)
- Reduce `maxDist` parameter (limit ray tracing distance)

## Customization

### Adjust Black Hole Mass

Edit `main.cpp`:
```cpp
BlackHole bh(1.0);  // Change mass (affects Schwarzschild radius)
```

### Change Camera Settings

Edit `main.cpp`:
```cpp
Camera cam(Vector3(0, 3, -15), Vector3(0, 0, 0), 60.0);
//         ^position           ^lookAt          ^FOV
```

### Modify Accretion Disk

Edit `BlackHole.cpp` in the `diskDensity()` function to change:
- Disk inner/outer radius
- Thickness
- Procedural patterns

## Clean Build

```bash
make clean
make
```

## License

This project is provided as-is for educational and research purposes.

## Acknowledgments

- Physics equations based on Schwarzschild metric from general relativity
- Rendering techniques inspired by ray tracing literature
- Built with SDL2 for cross-platform graphics

---

**Enjoy exploring the warped spacetime around a black hole!** 🌌
