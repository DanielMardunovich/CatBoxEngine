# Light Persistence in Scenes - Complete! ?

## Feature: Lights Now Saved/Loaded with Scenes

**Status:** ? Fully Implemented

### What Was Added

Lights are now fully integrated with the scene system. When you save a scene, all lights (with all their properties) are saved and restored when the scene is loaded.

## How It Works

### Scene Structure (Updated)
```cpp
Scene
??? Entities[]        // Game objects
??? Lights[]          // All scene lights (NEW!)
??? Camera            // View settings
??? Environment       // Ambient, background
```

### What Gets Saved Per Light

**All Light Properties:**
```
? Name
? Type (Directional/Point/Spot)
? Position
? Direction
? Color
? Intensity
? Attenuation (Constant, Linear, Quadratic)
? Spotlight Cone (Inner/Outer Cutoff)
? Shadow Settings:
   - CastsShadows (on/off)
   - ShadowMapSize (512/1024/2048/4096)
   - ShadowBias
   - ShadowOrthoSize (directional)
   - ShadowNearPlane
   - ShadowFarPlane
   - ShadowFOV (point/spot)
? Enabled (on/off)
```

### Scene File Format (Example)

```ini
[Scene]
Name=My Lit Scene
Version=1.0

[Camera]
Position=0,5,10
Yaw=-90
Pitch=0
FOV=60

[Environment]
Ambient=0.02,0.02,0.02
Background=0.4,0.3,0.2

[Lights]
Count=2

[Light0]
Name=Sun
Type=0
Position=0,0,0
Direction=0.5,-0.7,0.3
Color=1,0.95,0.8
Intensity=1
CastsShadows=1
ShadowMapSize=1024
ShadowBias=0.005
ShadowOrthoSize=20
ShadowNearPlane=1
ShadowFarPlane=50
Enabled=1

[Light1]
Name=Fill Light
Type=1
Position=0,5,0
Direction=0,-1,0
Color=1,1,1
Intensity=1
Constant=1
Linear=0.09
Quadratic=0.032
CastsShadows=1
ShadowMapSize=1024
ShadowBias=0.005
ShadowFOV=120
ShadowNearPlane=0.1
ShadowFarPlane=50
Enabled=1

[Entities]
Count=2
# ... entities follow ...
```

## Usage

### Saving a Scene with Lights

```cpp
// 1. Setup your scene
Entity character = ...;
entityManager.AddEntity(character);

Light sun;
sun.Name = "Sun";
sun.Type = LightType::Directional;
sun.Direction = {0.5f, -0.7f, 0.3f};
sun.CastsShadows = true;
LightManager::Instance().AddLight(sun);

// 2. Save scene
SceneManager::Instance().SaveActiveScene("scenes/my_scene.scene");
```

**Saved Data:**
- ? All entities
- ? All lights (including shadow settings)
- ? Camera position/orientation
- ? Environment settings

### Loading a Scene with Lights

```cpp
// Load scene
SceneManager::Instance().LoadScene("scenes/my_scene.scene");

// Everything is restored:
// - Entities at saved positions
// - Lights with saved properties
// - Camera at saved position
// - Shadow maps automatically created if enabled
```

## Implementation Details

### OnLoad (Scene Activation)
```cpp
void Scene::OnLoad(EntityManager& entityManager)
{
    // 1. Clear current scene
    entityManager.Clear();
    LightManager::Instance().ClearLights();
    
    // 2. Load lights into LightManager
    for (const auto& light : m_lights)
    {
        LightManager::Instance().AddLight(light);
        // Shadow maps auto-created if CastsShadows=true
    }
    
    // 3. Load entities
    for (const auto& entity : m_entities)
    {
        entityManager.AddEntity(entity);
    }
}
```

### OnUnload (Scene Deactivation)
```cpp
void Scene::OnUnload(EntityManager& entityManager)
{
    // 1. Capture current entities
    m_entities = entityManager.GetAll();
    
    // 2. Capture current lights
    m_lights = LightManager::Instance().GetAllLights();
    
    // 3. Clear managers
    entityManager.Clear();
    LightManager::Instance().ClearLights();
}
```

### SaveToFile (Serialization)
```cpp
void Scene::SaveToFile(const std::string& path)
{
    // Write scene metadata
    out << "[Scene]\n";
    out << "Name=" << m_name << "\n";
    
    // Write camera
    out << "[Camera]\n";
    // ... camera data ...
    
    // Write lights
    out << "[Lights]\n";
    out << "Count=" << m_lights.size() << "\n";
    
    for (size_t i = 0; i < m_lights.size(); ++i)
    {
        out << "[Light" << i << "]\n";
        out << "Name=" << light.Name << "\n";
        out << "Type=" << (int)light.Type << "\n";
        // ... all light properties ...
    }
    
    // Write entities
    out << "[Entities]\n";
    // ... entity data ...
}
```

### LoadFromFile (Deserialization)
```cpp
void Scene::LoadFromFile(const std::string& path)
{
    // Clear existing data
    m_entities.clear();
    m_lights.clear();
    
    // Parse file
    while (reading)
    {
        if (section == "[Light0]" or "[Light1]" etc.)
        {
            // Build light from saved data
            Light light;
            light.Name = readValue("Name");
            light.Type = (LightType)readInt("Type");
            light.Position = readVec3("Position");
            // ... read all properties ...
            
            m_lights.push_back(light);
        }
        else if (section == "[Entity0]" etc.)
        {
            // Build entity
            // ...
        }
    }
}
```

## Automatic Shadow Map Creation

When a light is loaded with `CastsShadows=true`, the shadow map is automatically created:

```cpp
size_t LightManager::AddLight(const Light& light)
{
    m_lights.push_back(light);
    
    // Auto-create shadow map if needed
    if (light.CastsShadows)
    {
        CreateShadowMap(m_lights.back());
        // Creates FBO and depth texture
    }
    
    return m_lights.size() - 1;
}
```

## Light Type Mapping

```cpp
enum class LightType
{
    Directional = 0,  // Saved as "Type=0"
    Point = 1,        // Saved as "Type=1"
    Spot = 2          // Saved as "Type=2"
};
```

## Testing Scenarios

### Test 1: Save and Load Scene
```
1. Create new scene
2. Add 2-3 lights with different types
3. Configure shadow settings
4. Add some entities
5. Save scene
6. Close engine
7. Load scene
Expected: ? All lights restored with exact settings
```

### Test 2: Multiple Scenes
```
Scene A: 1 directional light (Sun)
Scene B: 3 point lights (Indoor)

Switch A ? B: ? Sun disappears, point lights appear
Switch B ? A: ? Point lights disappear, Sun appears
```

### Test 3: Shadow Settings Preserved
```
1. Setup light with:
   - CastsShadows=true
   - ShadowMapSize=2048
   - ShadowBias=0.007
2. Save scene
3. Load scene
Expected: ? Shadow settings exactly restored
```

### Test 4: Light Modifications
```
1. Load scene
2. Change light color
3. Save scene (automatic on scene switch)
4. Load scene again
Expected: ? Modified color saved
```

## Console Output

### Saving Scene
```
Scene saved: scenes/my_scene.scene
  Entities: 5
  Lights: 2
  Camera: Saved
```

### Loading Scene
```
Scene loaded: scenes/my_scene.scene (5 entities, 2 lights)
Shadow map created for Sun
Shadow map created for Fill Light
```

## API Reference

### Scene Methods
```cpp
// Lights
void AddLight(const Light& light);
void RemoveLight(size_t index);
void ClearLights();
const std::vector<Light>& GetLights() const;
size_t GetLightCount() const;
```

### LightManager Methods
```cpp
// NEW: Clear all lights
void ClearLights();

// Existing
size_t AddLight(const Light& light);
void RemoveLight(size_t index);
```

### SceneManager Methods
```cpp
// Loads scene (including lights)
bool LoadScene(const std::string& path);

// Saves current scene (including lights)
bool SaveActiveScene(const std::string& path);
```

## Benefits

### Before (Without Light Persistence)
```
? Lights reset to default every scene
? Had to manually recreate lighting setup
? Lost shadow configurations
? Inconsistent lighting between sessions
```

### After (With Light Persistence)
```
? Lights saved with scene
? Shadow settings preserved
? Lighting consistent across sessions
? Easy to create different lighting setups per scene
```

## Advanced Usage

### Creating Lighting Presets
```cpp
// Save different lighting setups
SaveScene("scenes/outdoor_day.scene");    // Bright sun
SaveScene("scenes/outdoor_night.scene");  // Moonlight
SaveScene("scenes/indoor_warm.scene");    // Warm lighting
SaveScene("scenes/indoor_cool.scene");    // Cool lighting
```

### Dynamic Time of Day
```cpp
// Load appropriate lighting for time
if (isDay)
    LoadScene("scenes/level1_day.scene");
else if (isDusk)
    LoadScene("scenes/level1_dusk.scene");
else
    LoadScene("scenes/level1_night.scene");
```

## Troubleshooting

### Issue: Lights Not Appearing After Load
**Cause:** Scene file missing [Lights] section or corrupt

**Check:**
```
1. Open scene file in text editor
2. Verify [Lights] section exists
3. Check Count= matches number of [Light0], [Light1] sections
```

### Issue: Shadow Maps Not Created
**Cause:** CastsShadows=0 in file or loading error

**Fix:**
```
1. Check scene file: CastsShadows=1
2. Verify ShadowMapSize is valid (512/1024/2048/4096)
3. Check console for "Shadow map created" message
```

### Issue: Lights Have Wrong Settings
**Cause:** Old scene file format or manual edits

**Fix:**
```
1. Load scene in engine
2. Adjust lights in UI
3. Save scene again (overwrites with correct format)
```

## Migration from Old Format

Old scenes had single `[Light]` section:
```ini
[Light]
Direction=0.5,-0.7,0.3
Color=1,1,1
Intensity=1
```

New scenes have multiple lights:
```ini
[Lights]
Count=1

[Light0]
Name=Sun
Type=0
Direction=0.5,-0.7,0.3
# ... all properties ...
```

**Old scenes will still load**, but only the single light will be converted.

## Future Enhancements

### Planned
- [ ] Light animation keyframes
- [ ] Light groups/layers
- [ ] IES light profiles
- [ ] Light templates/presets
- [ ] Visual light editor in 3D view

### Performance
- [ ] Light culling (disable off-screen lights)
- [ ] Light LOD (reduce quality for distant lights)
- [ ] Shadow map caching (static lights)

## Summary

? **Lights fully saved with scenes**  
? **All properties preserved** (including shadows)  
? **Automatic shadow map creation** on load  
? **Multi-light support** (unlimited lights per scene)  
? **Backward compatible** with old scene format  
? **Easy to use** (automatic with save/load)  

Your lighting setups are now persistent! ???
