# GLTF Support Implementation Guide

## Overview
Your engine now supports GLTF (.gltf and .glb) format in addition to OBJ files. GLTF is the modern standard for 3D asset exchange.

## Setup Instructions

### Step 1: Download tiny_gltf.h

**Option A: Manual Download**
1. Visit https://github.com/syoyo/tinygltf/releases
2. Download `tiny_gltf.h` (single header file)
3. Place it in `CatboxEngine/Dependencies/`

**Option B: Direct Download**
```bash
# From your project root
curl -L https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h -o CatboxEngine/Dependencies/tiny_gltf.h
```

### Step 2: Enable GLTF Support

In `CatboxEngine/graphics/Mesh.cpp`, uncomment these lines (around line 16):

```cpp
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "../Dependencies/tiny_gltf.h"
```

### Step 3: Rebuild

```bash
# Your build command
msbuild CatboxEngine.sln
# or
cmake --build .
```

## Usage

### Loading GLTF Files

```cpp
// Automatically detects format by extension
MeshHandle h = MeshManager::Instance().LoadMeshSync("model.gltf");  // ASCII
MeshHandle h = MeshManager::Instance().LoadMeshSync("model.glb");   // Binary

// Through UI
// Use "Browse..." button - now supports .gltf and .glb files
```

### Console Output

When loading, you'll see:
```
Loading GLTF: models/character.gltf
  Meshes: 3
  Materials: 5
  Textures: 8
  Loaded texture: models/textures/body_diffuse.png
  Loaded normal map: models/textures/body_normal.png
GLTF loaded: 12450 vertices, 5 submeshes
```

## Supported Features

### Geometry
? Multiple meshes per file  
? Vertex positions, normals, UVs, tangents  
? Indexed geometry (optimized)  
? Triangulated meshes  

### Materials
? PBR Metallic-Roughness workflow  
? Base color (diffuse)  
? Normal maps  
? Multi-material models (SubMeshes)  
? Material properties (color, alpha)  

### Textures
? External textures (PNG, JPG)  
? Embedded textures (in GLB)  
? Multiple texture coordinates  
? Automatic mipmap generation  

### Formats
? ASCII GLTF (.gltf + separate textures)  
? Binary GLB (.glb with embedded data)  

## Not Yet Supported

?? Animations  
?? Skinning/Rigging  
?? Morph targets  
?? Cameras/Lights  
?? PBR metallic/roughness textures  
?? Occlusion maps  
?? Emissive materials  

## Exporting to GLTF

### Blender
1. File ? Export ? glTF 2.0 (.glb/.gltf)
2. Settings:
   - Format: GLB or GLTF Separate
   - Include: Selected Objects (or Visible Objects)
   - Geometry: Apply Modifiers ?
   - Materials: Export ?
   - Compression: None (for now)
3. Export

### Maya
1. File ? Export All ? gltfExport
2. Or use https://github.com/WonderMediaProductions/gltf-exporter

### 3ds Max
1. Use this plugin: https://github.com/BabylonJS/Exporters

## Comparison: GLTF vs OBJ

| Feature | GLTF | OBJ |
|---------|------|-----|
| Multi-material | ? Native | ? With MTL |
| Textures | ? Embedded or External | External only |
| PBR Materials | ? Full support | ? Basic only |
| Binary Format | ? GLB | ? Text only |
| Animations | ? Supported | ? Not supported |
| File Size | ? Smaller (binary) | Larger (text) |
| Industry Standard | ? Modern | Legacy |

## Troubleshooting

### "GLTF support not compiled"
- You need to uncomment the `#define TINYGLTF_IMPLEMENTATION` lines in Mesh.cpp
- Make sure `tiny_gltf.h` is in your Dependencies folder

### Textures not loading
- Check console for texture paths
- Ensure textures are in the same directory as the GLTF file
- For GLB files, textures should be embedded

### Compile errors
```cpp
// Make sure these are defined BEFORE including tiny_gltf.h:
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
```

### "Multiple definition" errors
- Only define `TINYGLTF_IMPLEMENTATION` in ONE .cpp file (Mesh.cpp)
- Other files should just `#include "tiny_gltf.h"` without the define

## Performance Notes

- **Binary GLB is faster** than ASCII GLTF (less parsing)
- **Embedded textures** in GLB load faster (no file I/O)
- **Multiple meshes** share the same vertex buffer (memory efficient)
- Each **submesh** requires its own draw call

## Advanced: Custom Extensions

tinygltf supports GLTF extensions. To add custom data:

```cpp
// In LoadFromGLTF, after loading model:
if (model.extensionsUsed.count("KHR_materials_pbrSpecularGlossiness"))
{
    // Handle specular-glossiness workflow
}
```

## Migration from OBJ

You don't need to change anything! The engine auto-detects file format:

```cpp
// Both work:
LoadMesh("model.obj");    // OBJ loader
LoadMesh("model.gltf");   // GLTF loader
LoadMesh("model.glb");    // GLTF binary loader
```

## Next Steps

1. Download `tiny_gltf.h`
2. Uncomment the includes in `Mesh.cpp`
3. Rebuild
4. Try loading a GLTF file!

## Resources

- **Khronos GLTF Spec**: https://www.khronos.org/gltf/
- **tinygltf GitHub**: https://github.com/syoyo/tinygltf
- **GLTF Sample Models**: https://github.com/KhronosGroup/glTF-Sample-Models
- **Online GLTF Viewer**: https://gltf-viewer.donmccurdy.com/

Happy modeling! ??
