# Black Hole Simulation

A real-time black hole simulation using C++20 with accurate physics equations and ray tracing. Features gravitational lensing, volumetric accretion disk rendering, and multi-threaded performance optimization.

![Black Hole Simulation](https://img.shields.io/badge/C%2B%2B-20-blue) ![Platform](https://img.shields.io/badge/platform-macOS-lightgrey)

## Features

- **Accurate Physics**: Schwarzschild metric geodesic integration using RK4 (Runge-Kutta 4th order)
- **Gravitational Lensing**: Light rays bend according to general relativity equations
- **Volumetric Rendering**: Realistic accretion disk with temperature gradients and procedural patterns
- **Multi-threaded**: Thread pool architecture utilizing all CPU cores
- **Interactive Controls**: Manual camera movement and automatic orbit mode
- **Real-time Performance**: Optimized for smooth frame rates on modern hardware

## Prerequisites

- **macOS** (tested on M4 Pro)
- **Xcode Command Line Tools** (for clang++)
- **Homebrew** (for installing dependencies)

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

```bash
./blackhole_sim
```

The simulation window will open with the camera automatically orbiting the black hole.

## Controls

| Key | Action |
|-----|--------|
| **W** | Move camera forward |
| **A** | Move camera left |
| **S** | Move camera backward |
| **D** | Move camera right |
| **Space** | Move camera up |
| **Shift** | Move camera down |
| **P** | Toggle auto-orbit mode |
| **R** | Reset camera position |
| **ESC** or **Q** | Quit simulation |

> **Note**: Manual movement automatically disables auto-orbit mode. Press `P` to re-enable it.

## Project Structure

```
blackhole_simulation/
├── src/
│   ├── main.cpp          # Entry point, SDL2 loop, controls
│   ├── Renderer.cpp      # ThreadPool, rendering logic
│   └── BlackHole.cpp     # Physics, geodesic integration
├── include/
│   ├── Vector3.hpp       # Math primitives
│   ├── Renderer.hpp      # Renderer & ThreadPool classes
│   └── BlackHole.hpp     # BlackHole class
├── vcpkg/                # Local dependency manager (gitignored)
├── build/                # Build artifacts (gitignored)
├── Makefile              # Build system
├── .gitignore
└── README.md
```

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

- **Thread Pool**: Persistent worker threads eliminate creation overhead
- **Work Chunking**: Screen divided into 8-row chunks to reduce lock contention
- **Multi-core**: Utilizes all available CPU cores
- **Optimized Math**: Efficient vector operations and integration

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
