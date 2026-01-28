# Scene Persistence Fix

## Problem
Entities spawned in a scene would disappear when the engine was closed and restarted.

## Root Cause
**MeshHandles are not persistent.** When you save a scene with `MeshHandle=5`, that number becomes invalid when the engine restarts because:
1. MeshManager starts fresh with no loaded meshes
2. Handle IDs are generated sequentially, so ID 5 might refer to a different mesh (or nothing at all)

## Solution
Save **mesh paths** instead of handles, then reload meshes when loading scenes.

## What Was Changed

### 1. Entity Now Stores MeshPath
```cpp
class Entity
{
    MeshHandle MeshHandle = 0;     // Runtime handle
    std::string MeshPath = "";      // Persistent path
};
```

### 2. Scene Files Now Save Paths
**Old Format (Broken):**
```ini
[Entity0]
Name=Player
MeshHandle=5    ? Invalid after restart
```

**New Format (Working):**
```ini
[Entity0]
Name=Player
MeshPath=C:\Models\player.gltf    ? Reloads mesh on load
```

### 3. Scene Loading Reloads Meshes
```cpp
// When loading scene
if (key == "MeshPath") 
{
    currentEntity.MeshPath = value;
    // Load mesh and get NEW handle
    if (!value.empty())
    {
        currentEntity.MeshHandle = MeshManager::Instance().LoadMeshSync(value);
    }
}
```

### 4. Auto-Save on Exit
```cpp
Engine::~Engine()
{
    // Auto-save active scene
    sceneMgr.SaveScene(activeSceneID, "autosave.scene", entityManager);
}
```

### 5. Auto-Load on Startup
```cpp
Engine::Initialize()
{
    // Try to load autosave
    if (file exists "autosave.scene")
    {
        SceneID id = sceneMgr.LoadScene("autosave.scene");
        sceneMgr.SetActiveScene(id, entityManager);
    }
}
```

## How It Works Now

### Workflow: Creating Entities

```
1. User spawns entity with model "C:\Models\player.gltf"
   ?? MeshManager loads mesh ? Returns handle 5
   ?? Entity.MeshHandle = 5
   ?? Entity.MeshPath = "C:\Models\player.gltf"

2. User closes engine
   ?? Engine destructor runs
   ?? Active scene auto-saved to "autosave.scene"
   ?? Scene file contains: MeshPath=C:\Models\player.gltf

3. User starts engine
   ?? Engine::Initialize() runs
   ?? Loads "autosave.scene"
   ?? Reads: MeshPath=C:\Models\player.gltf
   ?? Calls: MeshManager::LoadMeshSync("C:\Models\player.gltf")
   ?? Returns NEW handle (maybe 3 this time)
   ?? Entity.MeshHandle = 3
   ?? Entity renders correctly!
```

## Console Output

### On Engine Start
```
Loading autosave scene...
Scene loaded: autosave.scene (5 entities)
Active scene changed to: Default Scene (ID: 1)
Loading scene: Default Scene (5 entities)
  Loaded 5 entities into scene
Loading GLTF: C:\Models\player.gltf
  Meshes: 1, Materials: 2, Textures: 3
GLTF loaded: 787 vertices, 2 submeshes
```

### On Engine Close
```
Auto-saving active scene: Default Scene
Captured current entities to scene before saving
Scene saved: autosave.scene

=== Engine Shutdown ===
? No memory leaks detected!
```

## Testing

### Test 1: Basic Persistence

```
1. Start engine
2. Spawn entity with model (e.g., Browse ? player.gltf)
3. Close engine
4. Start engine again
5. ? Entity should still be visible
```

### Test 2: Multiple Entities

```
1. Start engine
2. Spawn 5 different models
3. Close engine
4. Start engine again
5. ? All 5 entities should be visible
```

### Test 3: Scene Switching

```
1. Create "Scene A", add 3 entities
2. Create "Scene B", add 2 entities
3. Save both scenes
4. Close engine
5. Start engine, load "Scene A"
6. ? Should see 3 entities from Scene A
7. Load "Scene B"
8. ? Should see 2 entities from Scene B
```

## Special Cases

### Shared Cube
```ini
[Entity0]
MeshPath=[cube]    ? Special marker for shared cube
```

When loaded:
```cpp
if (value == "[cube]")
{
    currentEntity.MeshHandle = MeshManager::Instance().GetSharedCubeHandle();
}
```

### Invalid Paths
If a mesh path in the scene file is invalid:
```
Loading GLTF: C:\Models\missing.gltf
GLTF Error: File not found
Failed to load mesh: C:\Models\missing.gltf
```

Entity will have `MeshHandle = 0` and won't render (but won't crash).

### Legacy Scene Files
Old scene files with `MeshHandle=5` are still supported:
```cpp
// Legacy support
else if (key == "MeshHandle" && currentEntity.MeshPath.empty())
{
    currentEntity.MeshHandle = std::stoull(value);
}
```

But they likely won't work correctly. Re-save scenes in new format.

## Autosave File Location

`autosave.scene` is created in the **working directory** (where the executable runs from).

**Example:**
```
CatBoxEngine/
??? CatboxEngine.exe
??? autosave.scene      ? Auto-generated
??? shaders/
??? scenes/
    ??? level1.scene    ? Manual saves
    ??? level2.scene
```

## Benefits

? **Entities persist** across engine restarts  
? **Automatic workflow** - no manual save required  
? **Scene portability** - can share scene files with paths  
? **Mesh caching** - MeshManager still caches by path  
? **Backward compatible** - old scenes still load (but may break)  

## Limitations

### Absolute Paths
Scene files save **absolute paths**:
```ini
MeshPath=C:\Users\Daniel\Documents\CatBoxEngine\models\player.gltf
```

This means scenes aren't portable between machines. To fix, use **relative paths**:
```cpp
// TODO: Convert absolute path to relative
std::string MakeRelativePath(const std::string& path);
```

### Large Scenes
Loading scenes with many entities will reload all meshes:
```
Scene with 100 entities ? Loads 100 meshes on startup
```

This can be slow. Future optimization: Async mesh loading with progress bar.

## Future Improvements

### Relative Paths
```cpp
// Save relative to project root
MeshPath=models/player.gltf    // Instead of C:\...\models\player.gltf
```

### Async Loading
```cpp
// Load scene in background
sceneMgr.LoadSceneAsync("level1.scene", [](SceneID id) {
    sceneMgr.SetActiveScene(id, entityManager);
});
```

### Progress Bar
```cpp
// Show loading progress
OnSceneLoad([](float progress) {
    ShowLoadingBar(progress);  // 0.0 to 1.0
});
```

### Smart Autosave
```cpp
// Only autosave if scene was modified
if (scene->IsDirty())
{
    sceneMgr.SaveScene(id, "autosave.scene", entityManager);
}
```

## Summary

Your entities now persist correctly! When you:
1. **Spawn entities** ? MeshPath is stored
2. **Close engine** ? Scene auto-saves with paths
3. **Start engine** ? Autosave loads, meshes reload
4. **Entities render** ? Everything works! ??

The key insight: **Store what to load, not the loaded result.**
