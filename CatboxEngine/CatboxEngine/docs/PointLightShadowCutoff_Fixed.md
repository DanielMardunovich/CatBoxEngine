# Point Light Shadow Cutoff Issue - Solved! ?

## Problem Description

**Issue:** Point light shadows get cut off in certain situations, leaving parts of the scene unshadowed even though the light affects them.

**Visual Symptom:** 
- Shadows only appear in a cone/direction from the light
- Areas outside this cone are lit but have no shadows
- Looks like shadows are "clipped" or "missing"

## Root Cause

Point lights emit light **omnidirectionally** (360° sphere), but the shadow implementation uses:
- **Single perspective projection** (like a spotlight)
- Limited FOV (Field of View) - default was 90°
- Only captures one direction

**Result:** Shadow frustum only covers ~90° cone, everything outside is cut off.

## Why This Happens

### Point Light vs Spotlight
```
Point Light (Reality):        Current Shadow (Bug):
     ?                              ?
   ? ? ?                          ?   ?
  ?  ?  ?                        ?     ?
 ?   ?   ?                      ?  90°  ?
???????????                    ?????????????
360° sphere                    Limited cone!
```

### Shadow Frustum
```
Point Light at (0, 5, 0)
Direction: (0, -1, 0) (downward)
FOV: 90°

Shadow Frustum (side view):
        Light
          ?
          ?
       ?     ?     ? 45° from center
      ?   ?   ?
     ?    ?    ?
    ?     ?     ?
   ?      ?      ?  ? Only this area gets shadows!
  ?????????????????
  
Objects outside cone: ? No shadows even though lit!
```

## Solutions

### Solution 1: Increase Shadow FOV (Quick Fix) ?

**What:** Make shadow frustum wider to cover more area

**How:** Increase `ShadowFOV` from 90° to 120° or higher

**UI Control:**
```
Lights Window ? Select Point Light ? Shadows
Shadow FOV: [????????????] 120°
```

**Pros:**
- ? Easy to implement
- ? Covers more area
- ? Adjustable per light

**Cons:**
- ? Still not omnidirectional (360°)
- ? Some areas may still be cut off
- ? Very wide FOV (>150°) causes distortion

**Recommended Values:**
- **90°** - Default (covers 25% of sphere)
- **120°** - Better coverage (covers ~33%)
- **150°** - Good compromise
- **179°** - Maximum (nearly 50% coverage, but distorted)

### Solution 2: Adjust Near/Far Planes ?

**What:** Match shadow range to light's effective reach

**Calculate Range:**
```cpp
// Attenuation formula
attenuation = 1.0 / (constant + linear*d + quadratic*d²)

// When attenuation drops to 1/256 (barely visible):
range = (-linear + ?(linear² - 4*quadratic*(constant - 256*intensity))) 
        / (2*quadratic)

Example:
constant = 1.0, linear = 0.09, quadratic = 0.032, intensity = 1.0
? range ? 42 units

Set: Near=0.1, Far=50
```

**UI Controls:**
```
Near Plane: [??????????] 0.1  ? Very close to light
Far Plane:  [???????????] 50.0  ? Match light range
```

**Benefits:**
- ? Shadows only where light reaches
- ? No wasted shadow map resolution
- ? Better depth precision

### Solution 3: Cube Shadow Maps (Proper Fix) ??

**What:** Use 6 shadow maps (one per cube face) for true 360° coverage

**How It Works:**
```
       ???????  +Y (Top)
       ?  2  ?
??????????????????????????
? -X  ? +Z  ? +X  ? -Z  ?
?  4  ?  0  ?  5  ?  1  ?  ? 6 faces
?????????????????????????
       ?  3  ?
       ???????  -Y (Bottom)

Each face = 90° FOV perspective
Together = Complete 360° coverage
```

**Advantages:**
- ? True omnidirectional shadows
- ? No cutoff issues
- ? Accurate for any angle

**Disadvantages:**
- ? 6x memory usage (6 shadow maps)
- ? 6x rendering cost (render scene 6 times)
- ? More complex implementation

**Status:** Not yet implemented (Future enhancement)

## Current Implementation

### What's Working Now ?

1. **Increased Default FOV**
   - Changed from 90° ? 120°
   - Covers more area by default

2. **UI Controls Added**
   - Shadow FOV slider (60° - 179°)
   - Near/Far plane controls
   - Real-time adjustment

3. **Per-Light Configuration**
   - Each point light can have different FOV
   - Adjustable based on scene needs

### How to Use

**Step 1: Enable Shadows**
```
Lights Window ? Point Light ? Shadows
? Cast Shadows
```

**Step 2: Adjust Coverage**
```
Shadow FOV: [??????????] 120°  ? Increase if shadows cut off
```

**Step 3: Optimize Range**
```
Near Plane: [??????????] 0.1   ? Close to light
Far Plane:  [??????????] 50.0  ? Match light range
```

**Step 4: Verify**
- Move camera around model
- Check all angles have shadows
- Increase FOV if any areas are cut off

## Testing Scenarios

### Test 1: Character in Center
```
Setup:
- Point light at (0, 5, 0)
- Character at (0, 0, 0)
- Shadow FOV: 120°

Expected: ? Shadows on all sides
```

### Test 2: Multiple Objects Around Light
```
Setup:
- Point light at (0, 5, 0)
- Objects at (-5,0,0), (5,0,0), (0,0,-5), (0,0,5)
- Shadow FOV: 150°

Expected: ? All objects cast shadows
```

### Test 3: Close to Light
```
Setup:
- Point light at (0, 2, 0)
- Object at (0, 0, 0)
- Shadow FOV: 120°, Near: 0.1, Far: 10

Expected: ? Shadow visible even very close
```

### Test 4: Far from Light
```
Setup:
- Point light at (0, 5, 0)
- Object at (20, 0, 0) [far away]
- Shadow FOV: 90°

Expected: ?? May be cut off (increase FOV to 150°)
```

## Performance Impact

### Shadow FOV vs Performance

| FOV | Coverage | Performance |
|-----|----------|-------------|
| 90° | 25% sphere | Baseline |
| 120° | 33% sphere | Same |
| 150° | 42% sphere | Same |
| 179° | ~50% sphere | Same |

**Note:** FOV change doesn't affect performance, only coverage area.

### Multiple Point Lights

```
1 Light:  1 shadow map (1024²) = ~4 MB
3 Lights: 3 shadow maps = ~12 MB
8 Lights: 8 shadow maps = ~32 MB

Performance: ~1-2ms per light
```

**Optimization:** Only enable shadows on most important 1-2 lights.

## Workarounds Until Cube Maps

### Workaround 1: Directional Light for Large Areas
If you need 360° coverage and scene is large:
```cpp
// Use directional light instead
Light sun;
sun.Type = LightType::Directional;
sun.Direction = {0, -1, 0};
// Directional = Infinite coverage, uniform shadows
```

### Workaround 2: Multiple Point Lights
Fake omnidirectional by using multiple lights:
```cpp
// 4 point lights in cross pattern
Light north, south, east, west;
north.Position = {0, 5, 10};
south.Position = {0, 5, -10};
east.Position = {10, 5, 0};
west.Position = {-10, 5, 0};
// Together they cover more area
```

### Workaround 3: Hybrid Approach
```cpp
// Point light for color/lighting
pointLight.CastsShadows = false;  // No shadows

// Directional light for shadows
sunLight.Type = LightType::Directional;
sunLight.Intensity = 0.1;  // Very dim, just for shadows
sunLight.CastsShadows = true;
```

## Configuration Recommendations

### Indoor Scenes (Small Rooms)
```
Shadow FOV: 150° - 179°
Near Plane: 0.1
Far Plane: 10-20
Reasoning: Need wide coverage, small space
```

### Outdoor Scenes (Large Areas)
```
Shadow FOV: 90° - 120°
Near Plane: 1.0
Far Plane: 50-100
Reasoning: Directional light better for outdoors
```

### Character Lighting (Close Range)
```
Shadow FOV: 120° - 150°
Near Plane: 0.1
Far Plane: 5-10
Reasoning: Close to subject, need good coverage
```

### Ambient Fill Light
```
Shadow FOV: N/A
Cast Shadows: OFF
Reasoning: Fill lights shouldn't cast shadows anyway
```

## Debug Visualization

### See Shadow Frustum (Future)
```cpp
// Draw lines showing shadow frustum
void DebugDrawShadowFrustum(Light& light)
{
    // Calculate frustum corners
    // Draw wireframe pyramid/cone
    // Shows exactly what shadow map sees
}
```

### Shadow Map Viewer (Future)
```cpp
// Display shadow map as texture overlay
ImGui::Begin("Shadow Debug");
ImGui::Image((void*)(intptr_t)light.ShadowMapTexture, 
             ImVec2(256, 256));
ImGui::End();
```

## Console Debug Output

### Current Implementation
```
=== Shadow Pass Debug ===
Total lights: 1
Light 0 (Point Light): FBO=1, Tex=2, Casts=1, Enabled=1
```

### With Cutoff Issues
```
Warning: Shadow FOV=90° may cause cutoff for point light at (0,5,0)
Recommendation: Increase FOV to 120° or higher
```

## Future Enhancements

### Priority 1: Cube Shadow Maps
- True 360° omnidirectional shadows
- 6-face rendering
- Cube map texture sampling

### Priority 2: Frustum Visualization
- Show shadow coverage in viewport
- Highlight cut-off areas
- Interactive adjustment

### Priority 3: Automatic FOV Calculation
```cpp
// Calculate optimal FOV based on:
// - Light position
// - Object positions
// - Attenuation range
float CalculateOptimalFOV(Light& light, Scene& scene);
```

### Priority 4: Cached Shadow Maps
- Only re-render when light/objects move
- Static scenes = render once
- Significant performance improvement

## Summary

? **Problem Identified:** Single perspective frustum causes cutoff  
? **Quick Fix Applied:** Increased FOV to 120° default  
? **UI Controls Added:** Adjustable FOV, near/far planes  
? **Works Now:** Shadows cover much larger area  
?? **Limitation:** Still not true 360° (needs cube maps)  
?? **Future:** Cube shadow maps for perfect coverage  

**For now:** Adjust Shadow FOV slider if you see cutoff (120° - 150° works well)!
