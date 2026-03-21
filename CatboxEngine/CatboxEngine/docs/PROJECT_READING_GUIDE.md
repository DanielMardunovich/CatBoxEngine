# CatBoxEngine Project Reading Guide

## Goal
This guide is for learning the codebase quickly without getting lost in details.

---

## 1) Where to start (in order)

Read these first, in this exact sequence:

1. [`CatboxEngine/main.cpp`](../main.cpp)
2. [`CatboxEngine/core/Engine.h`](../core/Engine.h)
3. [`CatboxEngine/core/Engine.cpp`](../core/Engine.cpp)
4. [`CatboxEngine/core/Platform.h`](../core/Platform.h) + [`CatboxEngine/core/Platform.cpp`](../core/Platform.cpp)
5. [`CatboxEngine/core/UIManager.h`](../core/UIManager.h) + [`CatboxEngine/core/UIManager.cpp`](../core/UIManager.cpp)
6. [`CatboxEngine/resources/Entity.h`](../resources/Entity.h)
7. [`CatboxEngine/resources/EntityManager.h`](../resources/EntityManager.h) + [`CatboxEngine/resources/EntityManager.cpp`](../resources/EntityManager.cpp)
8. [`CatboxEngine/resources/Scene.h`](../resources/Scene.h) + [`CatboxEngine/resources/Scene.cpp`](../resources/Scene.cpp)
9. [`CatboxEngine/resources/SceneManager.h`](../resources/SceneManager.h) + [`CatboxEngine/resources/SceneManager.cpp`](../resources/SceneManager.cpp)
10. [`CatboxEngine/graphics/RenderPipeline.h`](../graphics/RenderPipeline.h) + [`CatboxEngine/graphics/RenderPipeline.cpp`](../graphics/RenderPipeline.cpp)
11. [`CatboxEngine/graphics/Mesh.h`](../graphics/Mesh.h) + [`CatboxEngine/graphics/Mesh.cpp`](../graphics/Mesh.cpp)
12. [`CatboxEngine/graphics/MeshManager.h`](../graphics/MeshManager.h) + [`CatboxEngine/graphics/MeshManager.cpp`](../graphics/MeshManager.cpp)
13. [`CatboxEngine/graphics/Shader.h`](../graphics/Shader.h) + [`CatboxEngine/graphics/Shader.cpp`](../graphics/Shader.cpp)
14. [`CatboxEngine/graphics/Light.h`](../graphics/Light.h) + [`CatboxEngine/graphics/LightManager.h`](../graphics/LightManager.h) + [`CatboxEngine/graphics/LightManager.cpp`](../graphics/LightManager.cpp)
15. [`CatboxEngine/shaders/VertexShader.vert`](../shaders/VertexShader.vert) and [`CatboxEngine/shaders/FragmentShader.frag`](../shaders/FragmentShader.frag)

If you only do one pass, do this list first.

---

## 2) Mental model of the engine

- `Engine` owns the frame loop (`Update` + `Render`) and mode switching (editor/play mode).
- `EntityManager` owns runtime entities.
- `SceneManager`/`Scene` handle persistence (load/save scene files) and scene activation.
- `MeshManager` owns mesh lifetime, loading (sync/async), and shared handles.
- `RenderPipeline` does rendering passes and submits draw calls.
- `UIManager` and inspectors provide in-editor tooling.
- Gameplay systems (`PlayerController`, `GoalSystem`, `EnemySystem`, etc.) run in play mode.

---

## 3) Runtime flow you should trace once

Follow this path in code:

1. Program starts in [`main.cpp`](../main.cpp).
2. [`Engine::app()`](../core/Engine.cpp) enters loop.
3. [`Engine::Initialize()`](../core/Engine.cpp) sets up window/GL, render pipeline, camera, ImGui, scene load.
4. Per frame:
   - [`Engine::Update()`](../core/Engine.cpp) handles input, play/editor behavior, systems, UI build.
   - [`Engine::Render()`](../core/Engine.cpp) clears frame, calls [`RenderPipeline::Render()`](../graphics/RenderPipeline.cpp), then UI render.
5. Shutdown:
   - autosave active scene,
   - cleanup,
   - memory leak report.

---

## 4) What to read by subsystem

### Core (`core/`)
- [`Engine.h`](../core/Engine.h) + [`Engine.cpp`](../core/Engine.cpp): orchestration and mode transitions.
- [`Platform.h`](../core/Platform.h) + [`Platform.cpp`](../core/Platform.cpp): window + file dialogs + drop callback.
- [`InputHandler.h`](../core/InputHandler.h) + [`InputHandler.cpp`](../core/InputHandler.cpp): camera/input + drag/drop model/texture behavior.
- [`Message.h`](../core/Message.h) + [`MessageQueue.h`](../core/MessageQueue.h): lightweight event bus.
- [`Time.h`](../core/Time.h) + [`Time.cpp`](../core/Time.cpp): delta time source.
- [`MemoryTracker.h`](../core/MemoryTracker.h) + [`MemoryTracker.cpp`](../core/MemoryTracker.cpp): allocation tracking.

### Resources (`resources/`)
- [`Entity.h`](../resources/Entity.h): central data model for rendering + gameplay tags + animation paths.
- [`EntityManager.h`](../resources/EntityManager.h) + [`EntityManager.cpp`](../resources/EntityManager.cpp): entity lifetime and mesh handle release behavior.
- [`Scene.h`](../resources/Scene.h) + [`Scene.cpp`](../resources/Scene.cpp): scene serialization format (`.scene`) and load/save mapping.
- [`SceneManager.h`](../resources/SceneManager.h) + [`SceneManager.cpp`](../resources/SceneManager.cpp): active-scene switching.
- [`Camera.h`](../resources/Camera.h) + [`Camera.cpp`](../resources/Camera.cpp), [`Transform.h`](../resources/Transform.h), [`Math/Vec3.h`](../resources/Math/Vec3.h), [`Math/Vec4.h`](../resources/Math/Vec4.h): foundational math/state.

### Graphics (`graphics/`)
- [`RenderPipeline.h`](../graphics/RenderPipeline.h) + [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp): shadow pass + geometry pass + skybox + overlays.
- [`Mesh.h`](../graphics/Mesh.h) + [`Mesh.cpp`](../graphics/Mesh.cpp): vertex/submesh/material/texture/morph/skeleton data.
- [`MeshManager.h`](../graphics/MeshManager.h) + [`MeshManager.cpp`](../graphics/MeshManager.cpp): handle-based mesh ownership and async completion callbacks.
- [`Shader.h`](../graphics/Shader.h) + [`Shader.cpp`](../graphics/Shader.cpp): uniform helpers and shader lifecycle.
- [`Light.h`](../graphics/Light.h) + [`LightManager.h`](../graphics/LightManager.h) + [`LightManager.cpp`](../graphics/LightManager.cpp): light data and shadow-map resources.
- [`Skybox.h`](../graphics/Skybox.h) + [`Skybox.cpp`](../graphics/Skybox.cpp), [`GraphicsSettings.h`](../graphics/GraphicsSettings.h) + [`GraphicsSettings.cpp`](../graphics/GraphicsSettings.cpp): environment rendering and runtime settings.

### Gameplay (`gameplay/`)
Read in this order:
1. [`PlayerController.h`](../gameplay/PlayerController.h) + [`PlayerController.cpp`](../gameplay/PlayerController.cpp)
2. [`AnimationSystem.h`](../gameplay/AnimationSystem.h) + [`AnimationSystem.cpp`](../gameplay/AnimationSystem.cpp)
3. [`CollisionSystem.h`](../gameplay/CollisionSystem.h) + [`CollisionSystem.cpp`](../gameplay/CollisionSystem.cpp)
4. [`GoalSystem.h`](../gameplay/GoalSystem.h) + [`GoalSystem.cpp`](../gameplay/GoalSystem.cpp)
5. [`TeleporterSystem.h`](../gameplay/TeleporterSystem.h) + [`TeleporterSystem.cpp`](../gameplay/TeleporterSystem.cpp)
6. [`EnemySystem.h`](../gameplay/EnemySystem.h) + [`EnemySystem.cpp`](../gameplay/EnemySystem.cpp)
7. [`TerrainSystem.h`](../gameplay/TerrainSystem.h) + [`TerrainSystem.cpp`](../gameplay/TerrainSystem.cpp)
8. [`RecordTimeSystem.h`](../gameplay/RecordTimeSystem.h) + [`RecordTimeSystem.cpp`](../gameplay/RecordTimeSystem.cpp)

### UI (`ui/Inspectors/`)
Read after core/resources/graphics so inspector actions make sense:
- [`EntityManagerInspector.h`](../ui/Inspectors/EntityManagerInspector.h) + [`EntityManagerInspector.cpp`](../ui/Inspectors/EntityManagerInspector.cpp)
- [`PlayerInspector.h`](../ui/Inspectors/PlayerInspector.h) + [`PlayerInspector.cpp`](../ui/Inspectors/PlayerInspector.cpp)
- [`LightInspector.h`](../ui/Inspectors/LightInspector.h) + [`LightInspector.cpp`](../ui/Inspectors/LightInspector.cpp)
- [`GraphicsSettingsInspector.h`](../ui/Inspectors/GraphicsSettingsInspector.h) + [`GraphicsSettingsInspector.cpp`](../ui/Inspectors/GraphicsSettingsInspector.cpp)
- [`StatsInspector.h`](../ui/Inspectors/StatsInspector.h) + [`StatsInspector.cpp`](../ui/Inspectors/StatsInspector.cpp)
- [`LevelSelectMenu.h`](../ui/Inspectors/LevelSelectMenu.h) + [`LevelSelectMenu.cpp`](../ui/Inspectors/LevelSelectMenu.cpp)
- [`CameraInspector.h`](../ui/Inspectors/CameraInspector.h) + [`CameraInspector.cpp`](../ui/Inspectors/CameraInspector.cpp)

---

## 5) Important data contracts

1. `Entity::MeshHandle` is runtime-owned by `MeshManager`; `Entity::MeshPath` is persisted.
2. Scene loading rebinds meshes (`LoadMeshSync`) and can regenerate terrain (`[terrain]`).
3. Texture overrides can be on entity level, while base materials/textures can come from mesh/submesh.
4. Skinning path: `Mesh::MeshSkeleton` + `Entity::BoneMatrices` uploaded in `RenderPipeline`.
5. Play mode toggles behavior for input, controller systems, and overlays.

---

## 6) Recommended 3-pass reading strategy

### Pass 1: Structure pass (60-90 min)
Read only headers (`.h`) for:
- `core/`, `resources/`, `graphics/`, `gameplay/`.
Goal: identify ownership, lifetime, and main API surface.

### Pass 2: Runtime pass (2-3 hrs)
Read implementation (`.cpp`) for:
- [`Engine`](../core/Engine.cpp), [`RenderPipeline`](../graphics/RenderPipeline.cpp), [`Scene`](../resources/Scene.cpp), [`MeshManager`](../graphics/MeshManager.cpp), [`EntityManager`](../resources/EntityManager.cpp), [`PlayerController`](../gameplay/PlayerController.cpp).
Goal: understand frame flow and scene/load/render interplay.

### Pass 3: Feature pass (ongoing)
Pick one feature and trace end-to-end:
- Scene load/save
- Drag/drop asset import
- Play mode + goal timing
- Enemy patrol
- Terrain generation
- Animation/skeleton path

---

## 7) Best “first contributions” areas

Start here for safe modifications:
1. [`ui/Inspectors/*`](../ui/Inspectors/) small editor UI changes.
2. [`graphics/GraphicsSettings.h`](../graphics/GraphicsSettings.h) + [`graphics/GraphicsSettings.cpp`](../graphics/GraphicsSettings.cpp) toggles and rendering settings.
3. [`gameplay/*System.*`](../gameplay/) isolated gameplay behavior tweaks.
4. [`resources/Scene.h`](../resources/Scene.h) + [`resources/Scene.cpp`](../resources/Scene.cpp) serialization additions (careful with backward compatibility).

Harder/high-risk areas:
- [`RenderPipeline.h`](../graphics/RenderPipeline.h) + [`RenderPipeline.cpp`](../graphics/RenderPipeline.cpp), [`Mesh.h`](../graphics/Mesh.h) + [`Mesh.cpp`](../graphics/Mesh.cpp), [`MeshManager.h`](../graphics/MeshManager.h) + [`MeshManager.cpp`](../graphics/MeshManager.cpp), shader interfaces.

---

## 8) Quick checklist while reading

- Track who owns memory/resources (`MeshManager`, `LightManager`, OpenGL handles).
- Track editor mode vs play mode branches.
- Track where scene state is captured/restored.
- Track where data is runtime-only vs serialized to `.scene`.
- Track thread boundaries (`LoadMeshAsync` + main-thread `PollCompleted`).

---

## 9) Suggested next document to create

After this guide, create:
- [`docs/ARCHITECTURE_NOTES.md`](./ARCHITECTURE_NOTES.md)

Use sections:
- Frame lifecycle
- Resource ownership
- Scene format fields
- Gameplay system interactions
- Rendering pipeline uniforms/passes

This will become your project-specific reference as the engine evolves.
