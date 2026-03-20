# Catbox Engine – Feature Location Guide + Architecture Notes

This document maps each requested feature to where it lives in the codebase and explains how it works.

## 1) Cube rendered in window with perspective

**Where**
- [`Engine.cpp`](../core/Engine.cpp) (`Render()`, around lines 225-250)
- [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp) (`Render()`, around lines 40-60)
- [`Camera.cpp`](../resources/Camera.cpp) (`GetProjectionMatrix()`, lines 102-105)
- [`MeshManager.cpp`](../graphics/MeshManager.cpp) (`CreateCubeMesh()`, lines 241-284)
- [`EntityManager.cpp`](../resources/EntityManager.cpp) (`AddEntity()`, lines 12-18)

**How it works**
- The engine frame calls `m_renderPipeline.Render(...)`.
- `RenderPipeline` asks the camera for `view` and `projection` matrices and multiplies them for perspective rendering.
- `Camera::GetProjectionMatrix()` uses `glm::perspective(...)`.
- Entities without a mesh get the shared cube from `MeshManager::GetSharedCubeHandle()`.
- That shared cube geometry is built in `CreateCubeMesh()` and drawn during `GeometryPass()`.

## 2) Cube is textured

**Where**
- [`MeshManager.cpp`](../graphics/MeshManager.cpp) (`CreateCubeMesh()`, UVs at lines 247-273)
- [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp) (texture binding in `GeometryPass()`, around lines 245-320)
- [`Mesh.cpp`](../graphics/Mesh.cpp) (`LoadTextureFromFile()`, lines 1134-1161)

**How it works**
- The cube includes UV coordinates in its vertex data.
- During rendering, the pipeline checks if diffuse/specular/normal textures are present and binds them to texture units.
- Textures are loaded with STB (`stbi_load`) and uploaded via OpenGL.

## 3) UI Framework setup

**Where**
- [`Engine.cpp`](../core/Engine.cpp) (`InitImGui()`, lines 438-457)
- [`UIManager.cpp`](../core/UIManager.cpp) (`NewFrame()`, `Draw()`, `Render()`, lines 49-84 and 200-204)

**How it works**
- `Engine::InitImGui()` creates ImGui context, sets style, and initializes GLFW/OpenGL backends.
- Each frame:
  - `UIManager::NewFrame()` starts an ImGui frame.
  - `UIManager::Draw()` builds editor windows/inspectors.
  - `UIManager::Render()` submits ImGui draw data.

## 4) Implement `Entity` and `EntityManager` classes

**Where**
- [`Entity.h`](../resources/Entity.h)
- [`EntityManager.h`](../resources/EntityManager.h)
- [`EntityManager.cpp`](../resources/EntityManager.cpp)

**How it works**
- `Entity` is the data model for scene objects (transform, mesh handle/path, texture overrides, gameplay tags).
- `EntityManager` owns `std::vector<Entity>` and provides add/remove/find operations.
- On add/remove, it also coordinates with `MeshManager` and posts messages to `MessageQueue`.

## 5) Manipulate entity name/model/texture/position/rotation in UI

**Where**
- [`EntityManagerInspector.cpp`](../ui/Inspectors/EntityManagerInspector.cpp)
  - `DrawEntityInfo()` (name/tags)
  - `DrawEntityTransform()` (position/rotation/scale)
  - `DrawEntityMesh()` (change/remove mesh)
  - `DrawEntityTextures()` and `DrawTextureOverride()` (texture overrides)
- Key ranges: lines ~307-529 and 531-638

**How it works**
- Name is edited with `ImGui::InputText`.
- Position/rotation/scale use `ImGui::DragFloat3`.
- Mesh changes use a file dialog and `MeshManager::LoadMeshSync(...)`.
- Texture overrides are loaded and assigned per entity channel (diffuse/specular/normal).

## 6) Render all entities

**Where**
- [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp) (`GeometryPass()`, lines 199-332)

**How it works**
- `GeometryPass()` loops over `entityManager.GetAll()`.
- For each entity, it resolves the mesh handle, builds model transform matrix, sets shader uniforms, binds textures, and issues draw calls.
- Supports both single-material meshes and multi-submesh materials.

## 7) OBJ loader (vertices + faces)

**Where**
- [`Mesh.cpp`](../graphics/Mesh.cpp) (`LoadFromOBJ()`, starts line 26)
- Face parsing/triangulation around lines 169-268

**How it works**
- Reads `v`, `vt`, `vn`, `f` records from OBJ text.
- Supports polygon faces and triangulates via fan method.
- Caches unique `(position, uv, normal)` combinations into final vertex/index buffers.
- Also supports material groups (`mtllib`, `usemtl`) and texture map lookup from MTL.

## 8) Mesh manager + mesh caching

**Where**
- [`MeshManager.h`](../graphics/MeshManager.h)
- [`MeshManager.cpp`](../graphics/MeshManager.cpp)
  - Path cache in `CreateEntryForPath()` lines 18-37
  - Sync/async loading lines 46-138

**How it works**
- Uses `m_pathToHandle` to reuse already-loaded meshes by path.
- Uses per-entry refcount to manage lifetime.
- Supports sync and async load paths.
- Includes a shared cube handle singleton path (`"__shared_cube"`).

## 9) Game engine architecture document

### High-level runtime architecture

**Core loop**
- [`Engine.cpp`](../core/Engine.cpp)
  - `app()` runs frame loop: `Time::Update()` → `Update()` → `Render()`.

**Update phase responsibilities**
- Input/gameplay update and play-mode logic in `Engine::Update()`.
- UI frame construction via `UIManager`.
- Async mesh completion polling via `MeshManager::PollCompleted()`.
- Event dispatch via `MessageQueue::ProcessMessages()`.

**Render phase responsibilities**
- `Engine::Render()` clears frame and calls `RenderPipeline::Render()`.
- `RenderPipeline` handles shadow pass, geometry pass, skybox pass, debug visuals.
- UI is rendered through ImGui backend at end of frame.

### Main subsystems

- **Resource layer**: `Entity`, `EntityManager`, `Scene`, `SceneManager`, `Camera`
- **Graphics layer**: `Mesh`, `MeshManager`, `RenderPipeline`, shaders, lights
- **Gameplay layer**: player, collisions, enemy, teleporter, goal, terrain systems
- **Core services**: `MessageQueue`, `MemoryTracker`, `Time`, `Platform`, `UIManager`

### Data flow summary

1. Scene/entity data is edited in UI and stored in managers.
2. Mesh handles resolve through `MeshManager` cache.
3. `RenderPipeline` consumes entity + camera data and submits GPU draw calls.
4. Events/messages are posted by systems and processed centrally once per frame.

## 10) Message class + subclasses

**Where**
- [`Message.h`](../core/Message.h)

**How it works**
- `Message` is an abstract base with `GetType()` and `GetName()`.
- Concrete message types include entity, mesh, texture, and file-drop events.
- Payload fields are embedded per subtype (e.g., path, handle, entity index).

## 11) Message queue

**Where**
- [`MessageQueue.h`](../core/MessageQueue.h)
- Usage examples in:
  - [`EntityManager.cpp`](../resources/EntityManager.cpp) (post entity created)
  - [`EntityManager.h`](../resources/EntityManager.h) (post entity destroyed)
  - [`MeshManager.cpp`](../graphics/MeshManager.cpp) (post mesh loaded/failed)
  - [`Engine.cpp`](../core/Engine.cpp) (`SetupMessageSubscriptions()`, `Update()`)

**How it works**
- Thread-safe queue stores `shared_ptr<Message>`.
- Subscribers register callbacks per `MessageType`.
- `ProcessMessages()` swaps queue locally and dispatches callbacks.

## 12) Memory checking functions

**Where**
- [`MemoryTracker.h`](../core/MemoryTracker.h)
- [`MemoryTracker.cpp`](../core/MemoryTracker.cpp)
- [`Engine.cpp`](../core/Engine.cpp) (`app()` + destructor)

**How it works**
- `MemoryTracker` records allocations/deallocations and tracks totals/current usage.
- Can print reports and leak details.
- `Engine` prints memory state at startup/shutdown and checks leaks at teardown.

## 13) Camera class + UI configuration (including FOV)

**Where**
- [`Camera.h`](../resources/Camera.h) and [`Camera.cpp`](../resources/Camera.cpp)
- [`CameraInspector.cpp`](../ui/Inspectors/CameraInspector.cpp)

**How it works**
- Camera stores transform + perspective params (`FOV`, `Near`, `Far`, `Aspect`).
- `GetViewMatrix()` uses `glm::lookAt`, `GetProjectionMatrix()` uses `glm::perspective`.
- `CameraInspector` exposes FOV, clip planes, sensitivity, speed, and position controls in ImGui.

## 14) Texture loader choice (`stb`)

**Where**
- [`Mesh.cpp`](../graphics/Mesh.cpp)
  - STB defines near top (lines 15-20)
  - Generic texture load helper `LoadTextureFromFile()` (lines 1134-1161)

**How it works**
- Uses `stbi_load(...)` to decode image files into pixel data.
- Uploads pixel buffer to OpenGL texture object.
- Applies wrapping/filter/mipmap settings (via `GraphicsSettings`).

---

## Quick map by requirement

- Cube + perspective: [`Engine.cpp`](../core/Engine.cpp), [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp), [`Camera.cpp`](../resources/Camera.cpp), [`MeshManager.cpp`](../graphics/MeshManager.cpp)
- Cube textured: [`MeshManager.cpp`](../graphics/MeshManager.cpp), [`Mesh.cpp`](../graphics/Mesh.cpp), [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp)
- UI framework: [`Engine.cpp`](../core/Engine.cpp) (`InitImGui`), [`UIManager.h`](../core/UIManager.h), [`UIManager.cpp`](../core/UIManager.cpp)
- Entity + manager: [`Entity.h`](../resources/Entity.h), [`EntityManager.h`](../resources/EntityManager.h), [`EntityManager.cpp`](../resources/EntityManager.cpp)
- UI entity editing: [`EntityManagerInspector.cpp`](../ui/Inspectors/EntityManagerInspector.cpp)
- Render all entities: [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp) (`GeometryPass`)
- OBJ loader: [`Mesh.cpp`](../graphics/Mesh.cpp) (`LoadFromOBJ`)
- Mesh cache: [`MeshManager.h`](../graphics/MeshManager.h), [`MeshManager.cpp`](../graphics/MeshManager.cpp)
- Architecture: this section + [`Engine.cpp`](../core/Engine.cpp) / [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp)
- Message types: [`Message.h`](../core/Message.h)
- Message queue: [`MessageQueue.h`](../core/MessageQueue.h)
- Memory checks: [`MemoryTracker.h`](../core/MemoryTracker.h), [`MemoryTracker.cpp`](../core/MemoryTracker.cpp)
- Camera + FOV UI: [`Camera.h`](../resources/Camera.h), [`Camera.cpp`](../resources/Camera.cpp), [`CameraInspector.cpp`](../ui/Inspectors/CameraInspector.cpp)
- STB texture loading: [`Mesh.cpp`](../graphics/Mesh.cpp)
