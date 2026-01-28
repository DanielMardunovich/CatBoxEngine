# Multi-Material Mesh Support

## Overview
The mesh system now supports models with multiple materials through a SubMesh architecture. Each material gets its own geometry, textures, and rendering properties.

## Architecture

### SubMesh Structure
```cpp
struct SubMesh
{
    std::vector<uint32_t> Indices;  // Indices for this material
    uint32_t BaseVertex = 0;
    std::string MaterialName;
    
    // Material properties
    Vec3 DiffuseColor;
    Vec3 SpecularColor;
    float Shininess;
    float Alpha;
    
    // Textures (per material)
    unsigned int DiffuseTexture;
    unsigned int SpecularTexture;
    unsigned int NormalTexture;
    
    // OpenGL buffer
    uint32_t EBO;
};
```

### Mesh Structure
```cpp
struct Mesh
{
    std::vector<Vertex> Vertices;           // Shared by all submeshes
    std::vector<SubMesh> SubMeshes;         // One per material
    
    // Legacy single-material support
    std::vector<uint32_t> Indices;          // Used if SubMeshes.empty()
    Vec3 DiffuseColor;                      // etc.
};
```

## How It Works

### 1. OBJ Loading
When loading an OBJ file with multiple materials:

```obj
mtllib materials.mtl
usemtl Material1
f 1/1/1 2/2/2 3/3/3
usemtl Material2
f 4/4/4 5/5/5 6/6/6
```

The loader:
1. Tracks each `usemtl` directive
2. Creates a MaterialGroup for each material
3. Assigns faces to the appropriate group
4. Creates SubMeshes from MaterialGroups

### 2. Material Application
For each SubMesh:
- Loads material properties from MTL file
- Loads textures (diffuse, specular, normal)
- Creates separate OpenGL EBO for indices
- Stores all data in SubMesh structure

### 3. Rendering
The renderer checks if `mesh->SubMeshes.empty()`:

**Multi-Material Path:**
```cpp
for (const auto& sub : mesh->SubMeshes)
{
    // Set this submesh's material properties
    shader.setVec3("u_DiffuseColor", sub.DiffuseColor);
    shader.SetTexture("u_DiffuseMap", sub.DiffuseTexture);
    // ... etc
    
    // Draw this submesh
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
    glDrawElements(GL_TRIANGLES, sub.Indices.size(), ...);
}
```

**Legacy Path:**
```cpp
// Single material - uses mesh-level properties
shader.setVec3("u_DiffuseColor", mesh->DiffuseColor);
mesh->Draw();
```

## Benefits

? **Full Material Support** - Each material gets its own textures and properties  
? **Backward Compatible** - Single-material models work exactly as before  
? **Memory Efficient** - Vertices shared across all materials  
? **Flexible** - Easy to extend with more material types  

## Console Output

When loading a multi-material model:
```
Loaded MTL file: model.mtl
MTL parsing complete. Found 3 diffuse maps, 2 specular maps, 1 normal maps
Creating 3 submeshes for multi-material model
  SubMesh 'Material1' loaded diffuse: textures/mat1_diff.png
  SubMesh 'Material1' loaded normal: textures/mat1_norm.png
  SubMesh 'Material2' loaded diffuse: textures/mat2_diff.png
  SubMesh 'Material3' loaded diffuse: textures/mat3_diff.png
```

## Performance Notes

- **Shared Vertex Buffer**: All submeshes use the same vertex data (one VBO)
- **Multiple Draw Calls**: Each submesh requires its own draw call
- **Texture Binding**: Textures are bound per-submesh (may cause state changes)
- **Optimization**: Consider batching submeshes with identical materials

## Limitations

- Currently no material instancing (duplicate materials create duplicate data)
- No support for material libraries across multiple meshes
- Texture atlas not implemented (each submesh binds textures individually)

## Future Enhancements

1. **Material Library** - Share materials across multiple meshes
2. **Material Instancing** - Reuse identical materials
3. **Texture Atlas** - Combine textures to reduce binding overhead
4. **Draw Call Batching** - Group submeshes with same material
5. **LOD System** - Different detail levels per material

## Example Usage

```cpp
// Load a multi-material model
MeshHandle h = MeshManager::Instance().LoadMeshSync("character.obj");
Mesh* mesh = MeshManager::Instance().GetMesh(h);

if (!mesh->SubMeshes.empty())
{
    std::cout << "Model has " << mesh->SubMeshes.size() << " materials:" << std::endl;
    for (const auto& sub : mesh->SubMeshes)
    {
        std::cout << "  - " << sub.MaterialName << std::endl;
        std::cout << "    Diffuse: " << sub.DiffuseTexturePath << std::endl;
    }
}
```

## Migration Guide

### Old Code (Single Material)
```cpp
mesh->LoadTexture("texture.png");
mesh->DiffuseColor = {1, 0, 0};
```

### New Code (Multi-Material Compatible)
```cpp
if (mesh->SubMeshes.empty())
{
    // Legacy single material
    mesh->LoadTexture("texture.png");
    mesh->DiffuseColor = {1, 0, 0};
}
else
{
    // Access specific submesh
    mesh->SubMeshes[0].DiffuseColor = {1, 0, 0};
}
```

The system automatically detects whether a model is single or multi-material and handles it appropriately!
