# Scene Entity Synchronization Guide

## Overview
Scenes now automatically sync their entities with the EntityManager. When you switch scenes, entities are loaded/unloaded automatically, and saving captures the current state.

## How It Works

### Scene Switching Flow

```
Active Scene A          Switch to Scene B          Active Scene B
???????????????        ?????????????????         ???????????????
EntityManager:          1. Capture A entities      EntityManager:
 - Entity 1              ? Save to Scene A          - Entity 4
 - Entity 2             2. Clear EntityManager      - Entity 5
 - Entity 3             3. Load B entities           
                         ? From Scene B storage
```

### Step-by-Step

1. **OnUnload (Scene A)**:
   ```cpp
   // Captures current entities from EntityManager to Scene A
   scene->CaptureFromEntityManager(entityManager);
   // Clears EntityManager
   entityManager.Clear();
   ```

2. **OnLoad (Scene B)**:
   ```cpp
   // Clears EntityManager (already done)
   entityManager.Clear();
   // Loads Scene B's entities into EntityManager
   for (entity in scene->m_entities)
       entityManager.AddEntity(entity);
   ```

## Usage Examples

### Creating and Populating a Scene

```cpp
auto& sceneMgr = SceneManager::Instance();

// Create new scene
SceneID level1 = sceneMgr.CreateScene("Level 1");
sceneMgr.SetActiveScene(level1, entityManager);

// Add entities (they go into EntityManager)
Entity player;
player.name = "Player";
player.Transform.Position = {0, 0, 0};
entityManager.AddEntity(player, false);

Entity enemy;
enemy.name = "Enemy";
enemy.Transform.Position = {5, 0, 0};
entityManager.AddEntity(enemy, false);

// Entities are now in EntityManager
// When you switch scenes, they'll be captured to Scene
```

### Saving a Scene

**UI Method:**
1. Right-click scene in Scene Manager
2. Click "Save..."
3. Choose location

**Code Method:**
```cpp
// Saves current EntityManager state to scene, then writes to disk
sceneMgr.SaveScene(sceneID, "scenes/level1.scene", entityManager);
```

**What Happens:**
```cpp
// 1. Captures entities from EntityManager
scene->CaptureFromEntityManager(entityManager);

// 2. Saves to file
scene->SaveToFile(path);
```

### Loading a Scene

**UI Method:**
1. Click "Load Scene..." button
2. Select .scene file
3. Scene loads and becomes active

**Code Method:**
```cpp
SceneID id = sceneMgr.LoadScene("scenes/level1.scene");
sceneMgr.SetActiveScene(id, entityManager);
```

**What Happens:**
```cpp
// 1. Loads scene from file (entities in scene storage)
scene->LoadFromFile(path);

// 2. When activated, loads entities into EntityManager
scene->OnLoad(entityManager);
// ? entityManager now has all entities from file
```

### Switching Between Scenes

```cpp
// You have multiple scenes with entities
SceneID menu = sceneMgr.CreateScene("Menu");
SceneID game = sceneMgr.CreateScene("Game");

// Switch to menu
sceneMgr.SetActiveScene(menu, entityManager);
// EntityManager now has menu entities

// Add a button
Entity button;
button.name = "Play Button";
entityManager.AddEntity(button, false);

// Switch to game
sceneMgr.SetActiveScene(game, entityManager);
// Button is saved to Menu scene
// Game entities are loaded into EntityManager
```

## Console Output

### Scene Creation
```
Scene created: Level 1 (ID: 1)
```

### Scene Switching
```
Unloading scene: Menu
  Captured 3 entities from EntityManager
Active scene changed to: Game (ID: 2)
Loading scene: Game (0 entities)
  Loaded 0 entities into scene
```

### Scene Saving
```
Captured current entities to scene before saving
Scene saved: scenes/level1.scene
```

### Scene Loading
```
Scene loaded: scenes/level1.scene (5 entities)
Active scene changed to: Level 1 (ID: 3)
Loading scene: Level 1 (5 entities)
  Loaded 5 entities into scene
```

## Scene Manager UI

### Scene List Display
Each scene shows entity count:
```
Main Menu (3)   ? 3 entities in this scene
Level 1 (12)    ? 12 entities in this scene
Level 2 (8)     ? 8 entities in this scene
```

### Active Scene Info
```
Active Scene: Level 1
Entities: 12 (in scene) / 12 (in manager)
Loaded: Yes
```

- **(in scene)**: Entities stored in Scene object
- **(in manager)**: Entities currently in EntityManager
- These should match when scene is active

## Important Notes

### ? Entities Are Automatically Saved

When you **switch scenes**, entities are automatically captured:
```cpp
// Switching triggers auto-capture
sceneMgr.SetActiveScene(newSceneID, entityManager);
// Old scene entities are saved before switch
```

### ? Saving Captures Current State

When you **save a scene**, it captures EntityManager first:
```cpp
sceneMgr.SaveScene(sceneID, path, entityManager);
// Always gets latest entity state before saving
```

### ? No Manual Sync Needed

You **don't need** to manually sync entities:
```cpp
// ? Don't do this
scene->CaptureFromEntityManager(entityManager);
scene->SaveToFile(path);

// ? Just do this
sceneMgr.SaveScene(sceneID, path, entityManager);
```

### ?? EntityManager is Scene-Specific

Entities in EntityManager **belong to active scene**:
```cpp
// Scene A is active
entityManager.AddEntity(entity1);  // Goes to Scene A

sceneMgr.SetActiveScene(sceneB, entityManager);
// entity1 is saved to Scene A
// EntityManager now has Scene B entities

entityManager.AddEntity(entity2);  // Goes to Scene B
```

## Testing the System

### Test 1: Create and Save

```cpp
// 1. Create scene
SceneID test = sceneMgr.CreateScene("Test");
sceneMgr.SetActiveScene(test, entityManager);

// 2. Add entities
for (int i = 0; i < 5; i++) {
    Entity e;
    e.name = "Entity " + std::to_string(i);
    entityManager.AddEntity(e, false);
}

// 3. Save
sceneMgr.SaveScene(test, "test.scene", entityManager);

// 4. Check file - should have 5 entities
```

### Test 2: Load and Verify

```cpp
// 1. Load scene
SceneID loaded = sceneMgr.LoadScene("test.scene");
sceneMgr.SetActiveScene(loaded, entityManager);

// 2. Verify
assert(entityManager.Size() == 5);
// EntityManager should have 5 entities from file
```

### Test 3: Scene Switching

```cpp
// 1. Create two scenes with entities
SceneID sceneA = sceneMgr.CreateScene("A");
sceneMgr.SetActiveScene(sceneA, entityManager);
Entity eA; eA.name = "A Entity";
entityManager.AddEntity(eA, false);

SceneID sceneB = sceneMgr.CreateScene("B");
sceneMgr.SetActiveScene(sceneB, entityManager);
Entity eB; eB.name = "B Entity";
entityManager.AddEntity(eB, false);

// 2. Switch back to A
sceneMgr.SetActiveScene(sceneA, entityManager);

// 3. Verify only A entities present
assert(entityManager.Size() == 1);
assert(entityManager.GetAll()[0].name == "A Entity");
```

## Workflow Example

### Level Design Workflow

1. **Create Level Scene**
   - Scene Manager ? "Create Scene"
   - Name: "Forest Level"

2. **Populate Level**
   - Browse ? Load tree models
   - Spawn ? Create trees, rocks, enemies
   - All entities go into EntityManager

3. **Save Level**
   - Scene Manager ? Right-click "Forest Level"
   - Save... ? "scenes/forest.scene"
   - All entities captured and saved

4. **Test Another Level**
   - Create "Desert Level"
   - Switch to it (Forest entities saved automatically)
   - Add desert entities

5. **Switch Back**
   - Click "Forest Level" in Scene Manager
   - All forest entities reload
   - EntityManager shows correct entities

## Debugging Tips

### Check Entity Count

```cpp
auto* scene = sceneMgr.GetActiveScene();
std::cout << "Scene entities: " << scene->GetEntityCount() << std::endl;
std::cout << "Manager entities: " << entityManager.Size() << std::endl;
// These should match for active scene
```

### Verify Save/Load

```cpp
// Before save
size_t count = scene->GetEntityCount();

// Save
sceneMgr.SaveScene(id, "test.scene", entityManager);

// Load new instance
SceneID newID = sceneMgr.LoadScene("test.scene");
Scene* loaded = sceneMgr.GetScene(newID);

// Should match
assert(loaded->GetEntityCount() == count);
```

### Console Logging

Enable detailed logging in Scene.cpp OnLoad/OnUnload:
```
Loading scene: Level 1 (12 entities)
  Loaded 12 entities into scene  ? Verify count
Unloading scene: Level 1
  Captured 12 entities from EntityManager  ? Verify capture
```

## Summary

? **Automatic Sync**: Entities sync automatically when switching scenes  
? **Auto-Capture**: Saving a scene always captures current EntityManager state  
? **Clean Switching**: Old entities unloaded, new entities loaded  
? **UI Integration**: Scene Manager shows entity counts  
? **File Persistence**: Scenes save/load all entities correctly  

Your scene system now properly manages entity lifecycles! ??
