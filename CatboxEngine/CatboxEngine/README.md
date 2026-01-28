# CatBox Engine - Project Summary

## Overview
A 3D game engine built in C++ with OpenGL, featuring modern rendering capabilities, multi-material mesh support, and comprehensive model loading.

## Core Features

### ? Rendering System
- **OpenGL 4.4 Core Profile**
- **PBR-ready shaders** (diffuse, specular, normal mapping)
- **Multi-material rendering** with SubMesh architecture
- **Reference-counted mesh caching** via MeshManager
- **Thread-safe resource management**

### ? Model Loading
- **OBJ + MTL** with full material support
- **GLTF 2.0 / GLB** with PBR materials
- **Multi-material models** (one model, multiple textures)
- **Embedded & external textures**
- **Automatic fallbacks** for poorly exported models

### ? Message System
- **Event-driven architecture** with pub/sub pattern
- **Thread-safe message queue**
- **Built-in events**: Entity lifecycle, mesh loading, texture loading
- **Easily extensible** for custom events

### ? UI System
- **ImGui integration** for editor interface
- **Entity inspector** with transform controls
- **Camera inspector** with fly controls
- **Drag-and-drop** model/texture support
- **File browser** for model loading

## Architecture

```
Engine (Core Loop)
  ??? Platform (Window, Input, OS abstraction)
  ??? Renderer
  ?   ??? MeshManager (Caching, ref-counting)
  ?   ?   ??? Mesh (Geometry + Materials)
  ?   ?       ??? SubMesh[] (Multi-material)
  ?   ??? Shader (GLSL)
  ??? EntityManager (Scene graph)
  ?   ??? Entity[] (Transform + MeshHandle)
  ??? Camera (View/Projection, controls)
  ??? UIManager (ImGui interface)
  ??? MessageQueue (Event system)
```

## File Structure

```
CatboxEngine/
??? core/
?   ??? Engine.cpp/h          - Main engine loop
?   ??? Platform.cpp/h        - OS abstraction (Windows/Linux)
?   ??? UIManager.cpp/h       - ImGui interface
?   ??? Message.h             - Event message types
?   ??? MessageQueue.h        - Event system
??? graphics/
?   ??? Mesh.cpp/h            - Geometry & materials (OBJ/GLTF)
?   ??? MeshManager.cpp/h     - Resource caching
?   ??? Shader.cpp/h          - Shader management
?   ??? ...
??? resources/
?   ??? Entity.h              - Game object
?   ??? EntityManager.cpp/h   - Scene management
?   ??? Camera.cpp/h          - Camera system
?   ??? Math/Vec3.h           - Math types
??? shaders/
?   ??? VertexShader.vert     - Vertex shader
?   ??? FragmentShader.frag   - Fragment shader (PBR)
??? docs/
    ??? MessageSystem.md
    ??? MultiMaterialSupport.md
    ??? GLTF_Implementation_Guide.md
    ??? ...
```

## Key Systems

### MeshManager (Resource Management)
- **Reference counting**: Shared meshes, automatic cleanup
- **Path-based caching**: Same file = same mesh
- **Thread-safe**: Multiple threads can load meshes
- **Async loading**: Background loading with callbacks

### Multi-Material Support
- **SubMesh architecture**: Each material = SubMesh
- **Independent textures**: Diffuse, specular, normal per SubMesh
- **Efficient rendering**: Shared vertex buffer, separate EBOs
- **Legacy compatible**: Single-material models still work

### GLTF Loader
- **GLTF 2.0 & GLB**: ASCII and binary formats
- **PBR materials**: Base color, metallic, roughness
- **Multiple meshes**: Combines into single Mesh with SubMeshes
- **Embedded textures**: GLB files with internal images
- **External textures**: Separate PNG/JPG files
- **Fallback support**: Emissive ? diffuse for bad exports

## Usage Examples

### Loading a Model
```cpp
// Automatic format detection (.obj, .gltf, .glb)
MeshHandle h = MeshManager::Instance().LoadMeshSync("character.gltf");

// Create entity with model
Entity e;
e.MeshHandle = h;
e.Transform.Position = {0, 0, 0};
entityManager.AddEntity(e, false);
```

### Message System
```cpp
// Subscribe to events
MessageQueue::Instance().Subscribe(MessageType::MeshLoaded, 
    [](const Message& msg) {
        auto& m = static_cast<const MeshLoadedMessage&>(msg);
        std::cout << "Loaded: " << m.path << std::endl;
    });

// Post events
auto msg = std::make_shared<MeshLoadedMessage>(path, handle);
MessageQueue::Instance().Post(msg);
```

### Multi-Material Rendering
```cpp
if (!mesh->SubMeshes.empty())
{
    for (const auto& sub : mesh->SubMeshes)
    {
        // Bind submesh material
        shader.setVec3("u_DiffuseColor", sub.DiffuseColor);
        glBindTexture(GL_TEXTURE_2D, sub.DiffuseTexture);
        
        // Draw submesh
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
        glDrawElements(GL_TRIANGLES, sub.Indices.size(), ...);
    }
}
```

## Console Output (Clean Mode)

```
Loading GLTF: model.gltf
  Meshes: 2, Materials: 3, Textures: 5
  Loaded texture (external fallback): textures/body.png
GLTF loaded: 5420 vertices, 3 submeshes
Entity created: Model: model.gltf at index 0
```

## Performance Notes

- **Mesh caching**: Same model loaded once, shared by all entities
- **Reference counting**: Automatic cleanup when last entity deleted
- **SubMesh batching**: One vertex buffer, multiple index buffers
- **Texture sharing**: Same texture = same GPU memory

## Dependencies

- **OpenGL 4.4+** (via GLAD)
- **GLFW 3** (windowing, input)
- **GLM** (math library)
- **ImGui** (UI)
- **stb_image** (texture loading)
- **tinygltf** (GLTF loading)

## Building

```bash
# Visual Studio
Open CatboxEngine.sln
Build ? Rebuild Solution

# CMake
mkdir build && cd build
cmake ..
cmake --build .
```

## Future Enhancements

### Planned Features
- [ ] Animation system (skeletal, morph targets)
- [ ] Physics integration (collision, rigidbodies)
- [ ] Scene serialization (save/load)
- [ ] Material editor (visual shader editor)
- [ ] Shadow mapping
- [ ] Post-processing effects
- [ ] Asset browser (visual file picker)

### Optimization Opportunities
- [ ] Frustum culling
- [ ] LOD system
- [ ] Instanced rendering for duplicates
- [ ] Async texture loading
- [ ] Texture atlasing for SubMeshes
- [ ] Material instancing (deduplicate)

## Documentation

See `/docs` folder for detailed guides:
- `MessageSystem.md` - Event system
- `MultiMaterialSupport.md` - SubMesh architecture
- `GLTF_Implementation_Guide.md` - GLTF loader
- `GLTF_Texture_Troubleshooting.md` - Debug guide

## License

[Your License Here]

## Contributors

[Your Name]
