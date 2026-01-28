# Light Spawning and Customization UI

## Overview
The light spawning system allows you to create, customize, and delete lights directly from the UI, similar to entity management. This provides a complete workflow for setting up scene lighting.

## Features

? **Spawn Lights** - Create Point, Directional, and Spot lights with one click  
? **Light List** - View all lights in a scrollable list with type indicators  
? **Select & Edit** - Click to select a light and edit all its properties  
? **Delete Lights** - Remove lights from the scene  
? **Live Preview** - See changes in real-time as you adjust properties  
? **Type Icons** - Visual indicators (? Directional, ?? Point, ?? Spot)  

## Using the Lights Window

### Opening the Lights Window
The **Lights** window appears automatically in your UI alongside the Entity Inspector and Scene Manager.

### Spawning Lights

**Step 1**: Set spawn position
```
Spawn Position: [X] [Y] [Z]
Default: 0, 5, 0
```

**Step 2**: Click a spawn button
- **Spawn Point Light** - Creates omnidirectional point light
- **Spawn Directional Light** - Creates sun-like directional light
- **Spawn Spot Light** - Creates cone-shaped spotlight

**Result**: New light appears in the Light List

### Light List

The light list shows all active lights with:
- **Type Icon** - ? (Directional), ?? (Point), ?? (Spot)
- **Name** - Light name (e.g., "Point Light 0")
- **Status** - Shows "(Disabled)" if light is off
- **Delete Button** - Red "Delete" button on the right

**Selecting a Light**:
- Click on any light in the list
- Selected light is highlighted
- Properties appear below the list

### Editing Light Properties

When a light is selected, you can edit:

#### Basic Properties
```
Name: [Text Input]
Enabled: [?] Checkbox
Type: [Dropdown] Directional / Point / Spot
```

#### Position (Point & Spot Lights)
```
Position: [X] [Y] [Z]
Drag to adjust
```

#### Direction (Directional & Spot Lights)
```
Direction: [X] [Y] [Z]
Auto-normalized after editing
```

#### Color & Intensity
```
Color: [Color Picker] RGB
Intensity: [Slider] 0.0 - 10.0
```

#### Attenuation (Point & Spot Lights)
**Collapsible Section**: "Attenuation"

```
Constant: [Slider] 0.0 - 10.0
Linear: [Slider] 0.0 - 1.0
Quadratic: [Slider] 0.0 - 1.0

Effective Range: XX.X units (calculated)
```

**Range Formula**:
```
Range = (-Linear + ?(Linear² - 4×Quadratic×(Constant - 256×Intensity))) / (2×Quadratic)
```

#### Spotlight Cone (Spot Lights Only)
**Collapsible Section**: "Spotlight Cone"

```
Inner Cutoff: [Slider] 0° - 90°
Outer Cutoff: [Slider] 0° - 90°
```

**Note**: Outer cutoff is automatically clamped to be >= inner cutoff

#### Shadow Settings
**Collapsible Section**: "Shadows"

```
Cast Shadows: [?] Checkbox
Resolution: [Dropdown] 512 / 1024 / 2048 / 4096
Shadow Bias: [Slider] 0.0001 - 0.01

[For Directional Lights]
Ortho Size: [Slider] 1.0 - 100.0
Near Plane: [Slider] 0.1 - 10.0
Far Plane: [Slider] 10.0 - 200.0
```

## Workflow Examples

### Example 1: Create Sun Light
```
1. Click "Spawn Directional Light"
2. Select "Directional Light 0" in list
3. Rename to "Sun"
4. Set Direction: 0.5, -0.7, 0.3
5. Set Color: 1.0, 0.95, 0.8 (warm)
6. Set Intensity: 1.2
7. Enable "Cast Shadows"
8. Set Shadow Resolution: 2048
```

### Example 2: Create Room Light
```
1. Set Spawn Position: 0, 3, 0
2. Click "Spawn Point Light"
3. Select "Point Light 0"
4. Rename to "Ceiling Light"
5. Set Color: 1.0, 0.9, 0.7 (warm white)
6. Set Intensity: 2.0
7. Adjust Attenuation:
   - Constant: 1.0
   - Linear: 0.09
   - Quadratic: 0.032
```

### Example 3: Create Flashlight
```
1. Set Spawn Position: 0, 1.5, 0
2. Click "Spawn Spot Light"
3. Select "Spot Light 0"
4. Rename to "Flashlight"
5. Set Direction: 0, 0, -1 (forward)
6. Set Color: 1.0, 1.0, 1.0
7. Set Intensity: 3.0
8. Adjust Cone:
   - Inner Cutoff: 12.5°
   - Outer Cutoff: 17.5°
```

### Example 4: Multi-Colored Scene
```
1. Spawn 3 point lights at different positions
2. Light 1: Red (1, 0, 0) at (-5, 2, 0)
3. Light 2: Green (0, 1, 0) at (5, 2, 0)
4. Light 3: Blue (0, 0, 1) at (0, 2, 5)
5. Set all intensities to 1.5
6. Observe color mixing on surfaces
```

## UI Layout

```
?? Lights ??????????????????????????????????
? Active Lights: 3                         ?
? ?????????????????????????????????????????
? Spawn Position: [0] [5] [0]             ?
? [Spawn Point] [Spawn Dir] [Spawn Spot]  ?
? ?????????????????????????????????????????
? Light List:                              ?
? ????????????????????????????????????    ?
? ? ? Sun                    [Delete]?    ?
? ? ?? Point Light 0         [Delete]?    ?
? ? ?? Spot Light 0 (Disabled)[Delete]?   ?
? ????????????????????????????????????    ?
? ?????????????????????????????????????????
? Selected: Sun                            ?
? ?????????????????????????????????????????
? Name: [Sun                          ]    ?
? Enabled: [?]                             ?
? Type: [Directional ?]                    ?
? Direction: [0.5] [-0.7] [0.3]           ?
? Color: [?? RGB Picker               ]   ?
? Intensity: [?????•??????] 1.2           ?
?                                          ?
? ? Shadows                                ?
?   Cast Shadows: [?]                     ?
?   Resolution: [2048 ?]                   ?
?   Shadow Bias: [????•?] 0.0050          ?
?   Ortho Size: [????????•] 20.0          ?
????????????????????????????????????????????
```

## Console Output

### Spawning Lights
```
Light added: Point Light 0 (Index: 0)
Shadow map created: 1024x1024
```

### Deleting Lights
```
Light removed at index: 1
```

### Changing Properties
```
[When shadows are enabled]
Shadow map created: 2048x2048

[When shadows are disabled]
Deleted shadow map for light: Point Light 0
```

## Tips & Best Practices

### Performance
- **Start with few lights** (1-3) for best performance
- **Disable shadows** on non-important lights
- **Use lower resolution** (512-1024) for moving lights
- **Point lights are cheaper** than spot lights
- **Limit active lights** to what's visible

### Visual Quality
- **Use warm colors** for indoor scenes (0.9, 0.8, 0.7)
- **Use cool colors** for outdoor/night (0.8, 0.9, 1.0)
- **Combine multiple colors** for dynamic scenes
- **Adjust intensity** based on scene brightness
- **Use shadows** on main light only

### Attenuation Settings
```
Small Room (5-10 units):
  Constant: 1.0
  Linear: 0.35
  Quadratic: 0.44

Medium Room (10-20 units):
  Constant: 1.0
  Linear: 0.14
  Quadratic: 0.07

Large Room (20-50 units):
  Constant: 1.0
  Linear: 0.09
  Quadratic: 0.032

Very Large (50+ units):
  Constant: 1.0
  Linear: 0.045
  Quadratic: 0.0075
```

### Spotlight Settings
```
Flashlight:
  Inner: 12.5°, Outer: 17.5°

Desk Lamp:
  Inner: 20°, Outer: 30°

Stage Light:
  Inner: 15°, Outer: 25°

Searchlight:
  Inner: 5°, Outer: 10°
```

## Keyboard Shortcuts

*Currently no shortcuts, but could be added:*
```
Ctrl+L: Open Lights window
Ctrl+Shift+P: Spawn Point Light
Ctrl+Shift+D: Spawn Directional Light
Delete: Delete selected light
```

## Troubleshooting

### Issue: Light doesn't affect scene
**Causes:**
1. Light is disabled
2. Intensity is 0
3. Light is too far away (check attenuation)

**Solution:**
- Check "Enabled" checkbox
- Increase intensity
- Move light closer or adjust attenuation

### Issue: Light is too bright
**Solution:**
- Reduce intensity (try 0.5-2.0)
- Increase attenuation (higher linear/quadratic)

### Issue: Light is too dim
**Solution:**
- Increase intensity (try 2.0-5.0)
- Decrease attenuation (lower linear/quadratic)
- Check if light is blocked by geometry

### Issue: Shadows look wrong
**Causes:**
1. Shadow bias too low (shadow acne)
2. Shadow bias too high (Peter Panning)
3. Shadow map resolution too low

**Solutions:**
- **Shadow acne**: Increase bias to 0.005-0.01
- **Peter Panning**: Decrease bias to 0.001-0.003
- **Pixelated shadows**: Increase resolution to 2048 or 4096

### Issue: Spotlight cone not visible
**Causes:**
1. Inner/outer cutoff too small
2. Light direction wrong
3. Intensity too low

**Solution:**
- Increase outer cutoff to 30-45°
- Check direction points at scene
- Increase intensity

## Integration with Entities

### Move Light with Entity
```cpp
// In game code
Entity playerEntity = /* ... */;
Light* flashlight = LightManager::Instance().GetLight(0);

// Update light position to follow player
flashlight->Position = playerEntity.Transform.Position;
flashlight->Direction = playerEntity.GetForwardVector();
```

### Toggle Light on/off
```cpp
// Flashlight toggle
if (Input::KeyPressed(KEY_F))
{
    flashlight->Enabled = !flashlight->Enabled;
}
```

### Dynamic Light Color
```cpp
// Change light color based on game state
if (playerHealth < 30)
{
    light->Color = {1.0f, 0.0f, 0.0f};  // Red warning
}
```

## Summary

? **Easy to use** - Spawn lights with one click  
? **Full control** - Edit all properties in UI  
? **Real-time preview** - See changes immediately  
? **Professional workflow** - Similar to Unity/Unreal  
? **Type safety** - Icons show light type at a glance  
? **Performance aware** - Calculated range display  

Your lighting workflow is now production-ready! ??
