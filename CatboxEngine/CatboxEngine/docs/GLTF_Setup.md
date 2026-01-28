# GLTF Support Setup Guide

## Adding tinygltf Dependency

### Option 1: Download Manually
1. Go to https://github.com/syoyo/tinygltf/releases
2. Download `tiny_gltf.h` (single header file)
3. Place it in `CatboxEngine/Dependencies/`

### Option 2: Use vcpkg
```bash
vcpkg install tinygltf
```

### Option 3: Git Submodule
```bash
git submodule add https://github.com/syoyo/tinygltf.git CatboxEngine/Dependencies/tinygltf
```

## Required Files
- `tiny_gltf.h` - Single header library (similar to stb_image.h)

## Integration
The implementation in `Mesh.cpp` uses:
```cpp
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION  // Already defined elsewhere
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
```

## Supported Features
? Multi-material meshes  
? PBR materials (diffuse/baseColor, metallic, roughness, normal)  
? Embedded and external textures  
? Multiple meshes per file  
? Binary (.glb) and ASCII (.gltf) formats  

## Future Enhancements
- Animations
- Skinning/bones
- Morph targets
- Cameras and lights from GLTF scene
