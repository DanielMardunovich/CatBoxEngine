# GLTF Quick Start

## ? Quick Setup (3 steps)

### 1. Download tiny_gltf.h
```bash
# Place in CatboxEngine/Dependencies/
https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h
```

### 2. Enable in Mesh.cpp (line ~16)
Uncomment these lines:
```cpp
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "../Dependencies/tiny_gltf.h"
```

### 3. Rebuild
```bash
Build ? Rebuild Solution
```

## ? Done!

Now you can load:
- `.gltf` - ASCII format with external textures
- `.glb` - Binary format with embedded textures

## ?? Using GLTF

### UI
- Click "Browse..."
- Filter now shows: "3D Models (*.obj;*.gltf;*.glb)"
- Select your GLTF file
- Click "Spawn"

### Code
```cpp
MeshHandle h = MeshManager::Instance().LoadMeshSync("model.gltf");
```

## ?? Exporting from Blender

1. File ? Export ? glTF 2.0
2. Choose format:
   - **GLB** (recommended) - Single binary file
   - **GLTF Separate** - Multiple files with textures
3. Export!

## ?? Console Output

Success:
```
Loading GLTF: character.gltf
  Meshes: 2
  Materials: 3
  Textures: 5
GLTF loaded: 5420 vertices, 3 submeshes
```

Error (if not set up):
```
GLTF support not compiled. Add tiny_gltf.h...
```

## ?? Features

? Multi-material models  
? PBR materials  
? Normal maps  
? Binary (GLB) and ASCII (GLTF)  
? Embedded textures  

## ?? Full Docs

See: `docs/GLTF_Implementation_Guide.md`
