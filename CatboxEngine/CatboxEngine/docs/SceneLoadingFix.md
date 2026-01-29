# Scene Loading - Entity Rendering Fix

## Issue: Entities Not Rendering After Loading Scene

### Problem
When loading a saved scene, entities would not render even though they appeared in the entity list. This occurred after implementing frustum culling and shadow mapping.

### Root Cause

**Scene Loading Process (Before Fix):**
```cpp
void Scene::OnLoad(EntityManager& entityManager)
{
    for (const auto& entity : m_entities)
    {
        entityManager.AddEntity(entity, false);  // Just copy entity
    }
}
```

**Problems:**
1. **Stale Mesh Handles** - Mesh handles from saved scene might not be valid anymore
2. **Missing Bounds** - Meshes weren't guaranteed to have valid bounding boxes
3. **Unloaded Meshes** - Meshes might not be in memory even with valid handles
4. **Frustum Culling Issue** - Invalid bounds (FLT_MAX) would cause all entities to be culled

### Solution

**Updated Scene Loading Process:**
```cpp
void Scene::OnLoad(EntityManager& entityManager)
{
    for (auto& entity : m_entities)
    {
        // 1. Reload mesh from path
        if (!entity.MeshPath.empty())
        {
            MeshHandle newHandle = MeshManager::Instance().LoadMeshSync(entity.MeshPath);
            entity.MeshHandle = newHandle;
            
            // 2. Ensure bounds are calculated
            Mesh* mesh = MeshManager::Instance().GetMesh(newHandle);
            if (mesh)
            {
                bool validBounds = (mesh->BoundsMin.x != FLT_MAX);
                if (!validBounds)
                {
                    mesh->CalculateBounds();  // Calculate now!
                }
            }
        }
        
        entityManager.AddEntity(entity, false);
    }
}
```

### What the Fix Does

**Step 1: Reload Meshes**
- Uses `entity.MeshPath` to reload the mesh
- Gets new mesh handle from MeshManager
- Ensures mesh is in memory

**Step 2: Validate Bounds**
- Checks if bounding box is valid
- Calculates bounds if missing (FLT_MAX check)
- Prevents frustum culling from culling everything

**Step 3: Handle Special Cases**
- `[cube]` path ? Use shared cube handle
- Empty path ? Skip mesh loading
- Failed load ? Log error, continue with other entities

## Testing

### Before Fix
```
1. Spawn entities and save scene
2. Close engine
3. Load scene
? Result: Entity list shows entities, but nothing renders
```

### After Fix
```
1. Spawn entities and save scene
2. Close engine
3. Load scene
? Result: All entities render correctly
```

### Console Output (After Fix)
```
Loading scene: Default Scene (3 entities)
  Calculated bounds for mesh: models/cube.obj
  Calculated bounds for mesh: models/sphere.gltf
  Loaded 3 entities into scene
```

## Why Bounds Are Critical

### Frustum Culling Check (in Engine.cpp)
```cpp
bool validBounds = (mesh->BoundsMin.x != FLT_MAX) && 
                   (mesh->BoundsMax.x != -FLT_MAX);

if (validBounds)
{
    // Do frustum culling
    if (!camera.IsBoxInFrustum(worldMin, worldMax))
    {
        continue;  // Skip rendering
    }
}
```

**Without valid bounds:**
- `validBounds = false`
- Frustum culling is skipped ?
- Entity renders

**With invalid bounds (FLT_MAX):**
- `validBounds = true`
- Frustum culling happens
- Bounding box is infinite or invalid
- Entity is culled ?
- Nothing renders

### When Bounds Are Calculated

**Automatic (during loading):**
- `LoadFromOBJ()` ? calls `CalculateBounds()` ?
- `LoadFromGLTF()` ? calls `CalculateBounds()` ?

**Missing (causes the bug):**
- Loading from cache ? bounds might not be set
- Mesh already in memory ? bounds not recalculated
- Scene loading ? entity copied without reloading mesh

**Now Fixed:**
- `Scene::OnLoad()` ? checks and calculates bounds ?

## Additional Improvements

### 1. Handle Validation
```cpp
// Release old handle if different
if (entity.MeshHandle != 0 && entity.MeshHandle != newHandle)
{
    MeshManager::Instance().Release(entity.MeshHandle);
}
entity.MeshHandle = newHandle;
```

### 2. Error Logging
```cpp
if (newHandle == 0)
{
    std::cerr << "  Failed to reload mesh: " << entity.MeshPath << std::endl;
}
```

### 3. Shared Cube Support
```cpp
if (entity.MeshPath == "[cube]")
{
    entity.MeshHandle = MeshManager::Instance().GetSharedCubeHandle();
}
```

## Related Systems

### MeshManager
- Manages mesh loading and caching
- Returns mesh handles
- Provides shared cube for default geometry

### EntityManager
- Stores active entities
- Used for rendering loop
- Receives entities from Scene on load

### Frustum Culling
- Uses bounding boxes to cull off-screen entities
- Requires valid bounds to work correctly
- Skips culling if bounds are invalid

### Scene Persistence
- Saves entity data to .scene files
- Stores MeshPath for each entity
- Reloads meshes on scene load

## Debugging Tips

### Check if bounds are valid
```cpp
Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
if (mesh)
{
    std::cout << "Bounds: Min(" << mesh->BoundsMin.x << ", " 
              << mesh->BoundsMin.y << ", " << mesh->BoundsMin.z << ") "
              << "Max(" << mesh->BoundsMax.x << ", " 
              << mesh->BoundsMax.y << ", " << mesh->BoundsMax.z << ")" 
              << std::endl;
}
```

**Valid bounds:**
```
Bounds: Min(-1.0, -1.0, -1.0) Max(1.0, 1.0, 1.0)
```

**Invalid bounds (bug):**
```
Bounds: Min(3.40282e+38, 3.40282e+38, 3.40282e+38) Max(-3.40282e+38, -3.40282e+38, -3.40282e+38)
```

### Check if mesh is loaded
```cpp
if (entity.MeshHandle != 0)
{
    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
    if (mesh)
    {
        std::cout << "Mesh loaded: " << entity.MeshPath << std::endl;
    }
    else
    {
        std::cout << "Mesh handle invalid: " << entity.MeshHandle << std::endl;
    }
}
```

### Check frustum culling
```cpp
// Temporarily disable frustum culling to test
bool validBounds = false;  // Force skip culling
if (validBounds)
{
    // This won't run
}
// Entity will always render
```

## Summary

? **Scene loading now reloads meshes** properly  
? **Bounding boxes are validated** and calculated if needed  
? **Mesh handles are updated** to current session  
? **Frustum culling works** with valid bounds  
? **Entities render correctly** after scene load  

The fix ensures that all scene data is properly restored when loading, not just entity transforms and properties, but also the underlying mesh resources and their metadata.
