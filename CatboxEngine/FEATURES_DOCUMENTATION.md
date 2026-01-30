# Catbox Engine - Feature Implementation Documentation

This document outlines all implemented features and their locations in the codebase.

---

## 1. ? OBJ Loader - UVs and Normals Support

### Implementation Location
**File**: `CatboxEngine\graphics\Mesh.cpp`

### How It Works
- **Reading UVs**: Lines 156-160 - Parses `vt` (texture coordinate) lines from OBJ files
  ```cpp
  else if (prefix == "vt")
  {
      float u, v; ss >> u >> v;
      texcoords.emplace_back(u, v);
  }
  ```

- **Reading Normals**: Lines 161-165 - Parses `vn` (vertex normal) lines from OBJ files
  ```cpp
  else if (prefix == "vn")
  {
      float x, y, z; ss >> x >> y >> z;
      normals.emplace_back(x, y, z);
  }
  ```

- **Buffering to OpenGL**: Lines 1200-1230 in `Upload()` function
  - **Attribute 0**: Position (vec3)
  - **Attribute 1**: Normal (vec3) - Line 1212
  - **Attribute 2**: UV (vec2) - Line 1219
  - **Attribute 3**: Tangent (vec3) - Line 1226

### Features
- ? Supports triangle fans (polygons with arbitrary vertex counts)
- ? Handles negative indices (relative indexing)
- ? Automatically computes normals if not provided in OBJ file (Lines 295-330)
- ? Computes tangents for normal mapping (Lines 335-370)
- ? UV coordinate flipping for proper texture mapping (Line 239)

---

## 2. ? Diffuse/Albedo and Specular Maps

### A. Loading Textures
**File**: `CatboxEngine\graphics\Mesh.cpp`

**Functions**:
- `LoadTexture()` - Loads diffuse/albedo maps (Lines 1250-1257)
- `LoadSpecularTexture()` - Loads specular maps (Lines 1266-1273)
- `LoadNormalTexture()` - Loads normal maps (Lines 1282-1289)
- `LoadTextureFromFile()` - Helper function with graphics settings integration (Lines 1135-1159)

**MTL File Support**: Lines 75-140 - Reads material properties from .mtl files
- Diffuse texture: `map_Kd`
- Specular texture: `map_Ks`
- Normal map: `map_Bump`, `bump`, `map_bump`, or `norm`

### B. Per-Entity Texture Override UI
**File**: `CatboxEngine\ui\Inspectors\EntityManagerInspector.cpp`

**Function**: `DrawTextureOverride()` (Lines 308-390)

**Features**:
- Set diffuse, specular, and normal map overrides per entity
- Remove overrides to use mesh defaults
- Visual feedback (green "Override Active" text)
- Automatic texture loading with graphics settings
- File browser integration

**How to Use**:
1. Select an entity in Entity Inspector
2. Scroll to "Textures" section
3. Click "Set Override" for desired texture type
4. Browse and select texture file
5. Texture applies immediately

### C. Shader Usage
**File**: `CatboxEngine\shaders\FragmentShader.frag`

**Diffuse/Albedo** (Lines 169-183):
```glsl
vec3 albedo = u_DiffuseColor;
if (u_HasDiffuseMap)
{
    vec4 texColor = texture(u_DiffuseMap, TexCoord);
    albedo = texColor.rgb;
}
```

**Specular** (Lines 186-191):
```glsl
vec3 specularColor = u_SpecularColor;
if (u_HasSpecularMap)
{
    float specularIntensity = texture(u_SpecularMap, TexCoord).r;
    specularColor *= specularIntensity;
}
```

### D. Rendering
**File**: `CatboxEngine\graphics\RenderPipeline.cpp`

**Function**: `RenderEntities()` - Sets up textures and material uniforms
- Checks for entity texture override first, then mesh texture
- Binds diffuse, specular, and normal maps to texture units 0, 1, 2
- Sets shader uniforms for `u_HasDiffuseMap`, `u_HasSpecularMap`, `u_HasNormalMap`

---

## 3. ? MipMap Settings (Scene-Wide)

### A. Core Settings System
**Files**: 
- `CatboxEngine\graphics\GraphicsSettings.h`
- `CatboxEngine\graphics\GraphicsSettings.cpp`

**Class**: `GraphicsSettings` (Singleton)

**Settings**:
```cpp
bool EnableMipmaps = true;               // Enable/disable mipmaps globally
TextureFilterMode MinFilter = Trilinear; // Min filter mode
TextureFilterMode MagFilter = Linear;    // Mag filter mode
float AnisotropicFiltering = 16.0f;      // 1-16x anisotropic filtering
int MaxMipmapLevel = 1000;               // Max mipmap detail level
```

**Filter Modes**:
- `Nearest` - Pixelated look
- `Linear` - Smooth filtering
- `Bilinear` - Good quality with mipmaps
- `Trilinear` - Best quality (default)

**Function**: `ApplyToTexture(unsigned int textureID)` - Applies settings to a texture

### B. UI Controls
**Files**:
- `CatboxEngine\ui\Inspectors\GraphicsSettingsInspector.h`
- `CatboxEngine\ui\Inspectors\GraphicsSettingsInspector.cpp`

**Window**: "Graphics Settings"

**Controls**:
- ? Enable Mipmaps checkbox
- Min Filter dropdown (Nearest/Linear/Bilinear/Trilinear)
- Mag Filter dropdown (Nearest/Linear)
- Anisotropic Filtering slider (1x-16x)
- Max Mipmap Level slider (0-10)

**Features**:
- Real-time tooltips explaining each setting
- Visual feedback when settings change
- Settings summary display
- Note: Changes apply to newly loaded textures

### C. Integration
**Applied in**:
- `Mesh.cpp` - `LoadTextureFromFile()` (Lines 1148-1155)
- `EntityManagerInspector.cpp` - `LoadTextureWithSettings()` (Lines 392-418)

**Accessed via**: `GraphicsSettings::Instance()`

---

## 4. ? Phong Shader with Multiple Lights

### Implementation
**File**: `CatboxEngine\shaders\FragmentShader.frag`

### Lighting Model: **Blinn-Phong** (Advanced Phong)

**Maximum Lights**: 8 simultaneous lights (`MAX_LIGHTS = 8` - Line 4)

### Light Types Supported
1. **Directional Light** (type=0) - Sun/moon, no distance falloff
2. **Point Light** (type=1) - Omni-directional with distance attenuation
3. **Spot Light** (type=2) - Cone-shaped with distance and cone attenuation

### Lighting Components

**A. Ambient** (Line 237):
```glsl
vec3 ambient = 0.05 * albedo;  // Global illumination approximation
```

**B. Diffuse** (Lines 150-152):
```glsl
float diff = max(dot(normal, lightDir), 0.0);
vec3 diffuse = light.color * light.intensity * diff * albedo * attenuation;
```
- Lambert diffuse model
- Respects light color, intensity, and attenuation

**C. Specular - Blinn-Phong** (Lines 154-157):
```glsl
vec3 halfwayDir = normalize(lightDir + viewDir);
float spec = pow(max(dot(normal, halfwayDir), 0.0), u_Shininess);
vec3 specularContrib = light.color * light.intensity * spec * specular * attenuation;
```
- Uses halfway vector for better performance and quality
- Shininess controls highlight size (1-256)

### Light Structure (Lines 32-55)
```glsl
struct Light {
    int type;              // 0=Directional, 1=Point, 2=Spot
    vec3 position;         // World position
    vec3 direction;        // Light direction
    vec3 color;           // Light color (RGB)
    float intensity;      // Light intensity multiplier
    
    // Attenuation (Point and Spot)
    float constant;       // Constant attenuation
    float linear;         // Linear attenuation
    float quadratic;      // Quadratic attenuation
    
    // Spot light properties
    float innerCutoff;    // Inner cone angle
    float outerCutoff;    // Outer cone angle
    
    // Shadow properties
    bool castsShadows;           // Enable shadow mapping
    sampler2D shadowMap;         // Shadow depth map
    mat4 lightSpaceMatrix;       // Transform to light space
    float shadowBias;            // Shadow acne prevention
    
    bool enabled;         // Light on/off toggle
};
```

### Lighting Calculation
**Function**: `CalculateLight()` (Lines 104-164)

**Process**:
1. Calculate light direction based on type
2. Compute attenuation (distance and/or cone)
3. Calculate shadow factor (if enabled)
4. Compute diffuse lighting (Lambert)
5. Compute specular lighting (Blinn-Phong)
6. Apply shadow attenuation
7. Return final contribution

**Main Loop** (Lines 241-244):
```glsl
vec3 lighting = ambient;
for (int i = 0; i < u_NumLights && i < MAX_LIGHTS; ++i)
{
    lighting += CalculateLight(i, u_Lights[i], N, V, albedo, specularColor);
}
```

### Advanced Features
- ? **Normal Mapping** (Lines 194-230) - Per-pixel normal perturbation using tangent space
- ? **Attenuation** - Realistic light falloff with distance
- ? **Spot Light Cone** - Smooth falloff at cone edges
- ? **Material Properties** - Per-entity shininess and alpha

---

## 5. ? Light Configuration UI

### Implementation
**Files**:
- `CatboxEngine\ui\Inspectors\LightInspector.h`
- `CatboxEngine\ui\Inspectors\LightInspector.cpp`

### Window: "Light Inspector"

### A. Light Management
**Create New Light**:
- Button: "Create New Light"
- Automatically adds light to scene
- Default settings: Directional light, white color, intensity 1.0

**Delete Light**:
- "Delete" button for each light in list
- Confirmation required
- Removes light from active lights

### B. Light List (Lines 23-85)
- Scrollable list showing all lights
- Color-coded by type:
  - ?? Directional (yellow)
  - ?? Point (white)
  - ?? Spot (cyan)
- Shows enabled/disabled status
- Click to select and edit

### C. Light Properties Editor (Lines 87-250)

**Common Properties**:
- **Name**: Editable text field
- **Enabled**: Checkbox to toggle light on/off
- **Type**: Dropdown (Directional/Point/Spot)
- **Color**: RGB color picker
- **Intensity**: Slider (0.0 - 10.0)

**Position** (Point & Spot only):
- XYZ position drag controls
- World space coordinates

**Direction** (Directional & Spot):
- XYZ direction vector
- Automatically normalized

**Attenuation** (Point & Spot):
- Constant: Base attenuation
- Linear: Distance-based falloff
- Quadratic: Distance-squared falloff

**Spot Light Cone**:
- Inner Cutoff: Core spotlight angle (degrees)
- Outer Cutoff: Edge falloff angle (degrees)

**Shadow Properties**:
- Casts Shadows: Checkbox
- Shadow Bias: Prevents shadow acne (0.0001 - 0.01)

### D. Visual Helpers
- Color preview swatch for light color
- Real-time preview of light type icon
- Tooltips on all properties
- Warning when max lights (8) reached

### Backend
**File**: `CatboxEngine\graphics\LightManager.h/.cpp`

**Class**: `LightManager` (Singleton)

**Functions**:
- `CreateLight()` - Adds new light
- `DeleteLight(int id)` - Removes light
- `GetLight(int id)` - Retrieves light data
- `GetAllLights()` - Returns all active lights
- `GetActiveCount()` - Number of enabled lights

---

## 6. ? Shadow Mapping (Implemented)

### Implementation
**File**: `CatboxEngine\shaders\FragmentShader.frag`

### A. Shadow Calculation Function
**Function**: `CalculateShadow()` (Lines 62-101)

**Algorithm**: **PCF (Percentage Closer Filtering)**
- Samples 2x2 grid around shadow map texel
- Averages 4 samples for softer shadows
- Balances performance and quality

**Process**:
1. Transform fragment position to light space
2. Perform perspective divide
3. Transform to [0,1] texture coordinate range
4. Calculate dynamic bias based on surface angle
5. Sample shadow map in 2x2 pattern
6. Compare depth values
7. Return shadow factor (0.0 = fully lit, 1.0 = fully shadowed)

**Bias Calculation** (Line 83):
```glsl
float bias = max(light.shadowBias * (1.0 - dot(normal, lightDir)), light.shadowBias * 0.1);
```
- Prevents "shadow acne" artifacts
- Slope-based bias adjustment
- Configurable per light

**PCF Sampling** (Lines 86-98):
```glsl
for(int x = 0; x <= 1; ++x)
{
    for(int y = 0; y <= 1; ++y)
    {
        float pcfDepth = texture(light.shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
}
shadow /= 4.0;  // Average of 4 samples
```

### B. Shadow Integration
**In Lighting Calculation** (Lines 144-161):
1. Calculate shadow factor for current light
2. Soften shadows for stylized look (Line 148):
   ```glsl
   shadow = clamp(shadow * 0.4, 0.0, 1.0);
   ```
3. Apply shadow to diffuse and specular:
   ```glsl
   diffuse *= (1.0 - shadow);
   specularContrib *= (1.0 - shadow);
   ```

### C. Per-Light Shadow Support
**Vertex Shader Output** (Line 10):
```glsl
out vec4 FragPosLightSpace[8];  // Light space position for each light
```
- Supports shadow mapping for all 8 lights
- Each light has its own shadow map and transform

### D. Shadow Map Properties
**Per Light**:
- `bool castsShadows` - Enable/disable shadows for this light
- `sampler2D shadowMap` - Depth texture from light's perspective
- `mat4 lightSpaceMatrix` - Transforms world space to light space
- `float shadowBias` - Configurable bias (UI: 0.0001 - 0.01)

### Features
- ? PCF for soft shadow edges
- ? Perspective-correct shadows
- ? Dynamic bias based on surface angle
- ? Per-light shadow control
- ? Configurable shadow bias via UI
- ? Handles shadow map frustum boundaries

### Quality Settings
- **Sample Pattern**: 2x2 (4 samples)
- **Shadow Softness**: 40% (stylized)
- **Bias Range**: 0.0001 - 0.01 (configurable)

---

## Additional Features

### 7. ? Scene Management
**Files**: `CatboxEngine\resources\SceneManager.h/.cpp`

**Features**:
- Create, load, save, and unload scenes
- Active scene management
- Entity serialization
- Auto-save on shutdown to `Scenes/autosave.scene`

**UI**: `CatboxEngine\core\UIManager.cpp` - `DrawSceneManager()` function

### 8. ? Entity Management
**Files**: `CatboxEngine\resources\EntityManager.h/.cpp`

**Features**:
- Add, remove, and manage entities
- Transform controls (position, rotation, scale)
- Mesh assignment
- Material properties (shininess, alpha)
- Texture overrides per entity

### 9. ? Camera System
**Files**: `CatboxEngine\resources\Camera.h/.cpp`

**Features**:
- FPS-style camera controls (WASD + mouse)
- Configurable FOV, near/far planes
- Mouse sensitivity control
- Frustum culling support

**UI**: `CatboxEngine\ui\Inspectors\CameraInspector.h/.cpp`

### 10. ? Model Loading
**Formats Supported**:
- **OBJ** - Wavefront OBJ with MTL material files
- **GLTF/GLB** - GLTF 2.0 with embedded textures

**Features**:
- Multi-material support (SubMeshes)
- Automatic normal/tangent generation
- Texture fallback patterns
- Morph target (blend shape) support for GLTF

### 11. ? Memory Tracking
**File**: `CatboxEngine\core\MemoryTracker.h/.cpp`

**Features** (DEBUG builds only):
- Track allocations and deallocations
- Memory leak detection
- Usage reports
- Per-allocation tracking

**UI**: `CatboxEngine\ui\Inspectors\StatsInspector.h/.cpp`

---

## Architecture Summary

### Design Patterns
- **Singleton**: Managers (Mesh, Light, Scene, Graphics Settings)
- **Observer**: Message queue for events
- **Factory**: Entity/Light creation
- **Strategy**: Texture filtering modes
- **Coordinator**: UIManager

### Code Quality
- ? Modern C++ (C++17)
- ? Const correctness
- ? Smart memory management
- ? Modular design
- ? Comprehensive error handling
- ? Clean separation of concerns

---

## Build Configuration

**Platform**: Windows (Visual Studio)  
**Graphics API**: OpenGL 4.4+  
**Dependencies**:
- GLFW (windowing)
- GLAD (OpenGL loader)
- GLM (math)
- ImGui (UI)
- stb_image (texture loading)
- tiny_gltf (GLTF loading)

---

## Quick Reference

| Feature | Main File | UI File |
|---------|-----------|---------|
| OBJ Loading | `Mesh.cpp` (Lines 20-700) | N/A |
| Textures | `Mesh.cpp` (Lines 1135-1289) | `EntityManagerInspector.cpp` (Lines 290-420) |
| MipMaps | `GraphicsSettings.h/.cpp` | `GraphicsSettingsInspector.h/.cpp` |
| Phong Lighting | `FragmentShader.frag` (Lines 104-248) | N/A |
| Light Config | `LightManager.h/.cpp` | `LightInspector.h/.cpp` |
| Shadow Mapping | `FragmentShader.frag` (Lines 62-101) | `LightInspector.cpp` (shadow bias) |

---

**Documentation Created**: December 2024  
**Engine Version**: Catbox Engine v1.0  
**Author**: Daniel Mardunovich
