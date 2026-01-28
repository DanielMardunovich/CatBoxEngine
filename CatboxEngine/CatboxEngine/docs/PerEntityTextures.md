# Per-Entity Texture System - Diffuse, Specular, and Normal Maps

## Overview
The engine now supports per-entity texture overrides for diffuse (albedo), specular (shininess), and normal maps. Each entity can have its own textures that override the mesh's default textures.

## Features

? **Per-Entity Textures** - Each entity has independent texture overrides  
? **Diffuse/Albedo Maps** - Base color textures  
? **Specular Maps** - Shininess/reflectivity textures  
? **Normal Maps** - Surface detail without geometry  
? **Material Properties** - Per-entity shininess and alpha  
? **UI Controls** - Easy texture assignment in Inspector  
? **Scene Persistence** - Textures save/load with scenes  
? **Fallback System** - Uses mesh textures if no entity override  

## Entity Texture Properties

### Entity Structure
```cpp
class Entity
{
    // Texture overrides
    unsigned int DiffuseTexture = 0;
    std::string DiffuseTexturePath = "";
    bool HasDiffuseTextureOverride = false;
    
    unsigned int SpecularTexture = 0;
    std::string SpecularTexturePath = "";
    bool HasSpecularTextureOverride = false;
    
    unsigned int NormalTexture = 0;
    std::string NormalTexturePath = "";
    bool HasNormalTextureOverride = false;
    
    // Material properties
    float Shininess = 32.0f;  // 1-256
    float Alpha = 1.0f;        // 0-1
};
```

## Using the Texture System

### Method 1: UI Inspector (Easiest)

1. **Select an entity** in the Entity List
2. **Open Inspector** window
3. **Scroll to Textures section**
4. **Click "Set Override"** for desired texture type
5. **Choose texture file** (PNG, JPG, BMP, TGA)
6. **Texture loads automatically!**

### Method 2: Code

```cpp
Entity entity;
entity.name = "Textured Cube";

// Load diffuse texture
entity.DiffuseTexture = LoadTextureFromFile("textures/brick_diffuse.png");
entity.DiffuseTexturePath = "textures/brick_diffuse.png";
entity.HasDiffuseTextureOverride = true;

// Load specular texture
entity.SpecularTexture = LoadTextureFromFile("textures/brick_specular.png");
entity.SpecularTexturePath = "textures/brick_specular.png";
entity.HasSpecularTextureOverride = true;

// Load normal map
entity.NormalTexture = LoadTextureFromFile("textures/brick_normal.png");
entity.NormalTexturePath = "textures/brick_normal.png";
entity.HasNormalTextureOverride = true;

// Set material properties
entity.Shininess = 64.0f;  // Shiny surface
entity.Alpha = 1.0f;        // Fully opaque

entityManager.AddEntity(entity, false);
```

## Inspector UI

### Texture Section Layout

```
?? Inspector ???????????????????????????
? Name: My Entity                      ?
?                                      ?
? Transform                            ?
? Position: [0, 0, 0]                  ?
? Rotation: [0, 0, 0]                  ?
? Scale: [1, 1, 1]                     ?
?                                      ?
? Material Properties                  ?
? Shininess: [====•===] 64.0           ?
? Alpha:     [========•] 1.0           ?
?                                      ?
? Textures                             ?
? ??????????????????????????????       ?
? ? Diffuse: Override Active   ?       ?
? ?   textures/wood.png        ?       ?
? ? [Remove Override]          ?       ?
? ??????????????????????????????       ?
?                                      ?
? ??????????????????????????????       ?
? ? Specular: (using mesh default)?    ?
? ? [Set Override]             ?       ?
? ??????????????????????????????       ?
?                                      ?
? ??????????????????????????????       ?
? ? Normal: Override Active    ?       ?
? ?   textures/wood_normal.png ?       ?
? ? [Remove Override]          ?       ?
? ??????????????????????????????       ?
????????????????????????????????????????
```

### Visual Indicators

- **Green text** - Override is active
- **Gray text** - Using mesh default
- **[Set Override]** - Click to assign texture
- **[Remove Override]** - Click to clear and use mesh texture

## Texture Rendering Pipeline

### 1. Texture Priority
```
Entity Override > Mesh Default
```

If entity has override:
```cpp
unsigned int texID = entity.DiffuseTexture;  // Use entity texture
```

Otherwise:
```cpp
unsigned int texID = mesh->DiffuseTexture;   // Use mesh texture
```

### 2. Rendering Code
```cpp
// Check for entity texture override
bool hasDiffuse = entity.HasDiffuseTextureOverride ? 
                  true : mesh->HasDiffuseTexture;
unsigned int diffuseTex = entity.HasDiffuseTextureOverride ? 
                          entity.DiffuseTexture : mesh->DiffuseTexture;

shader.SetBool("u_HasDiffuseMap", hasDiffuse);
if (hasDiffuse)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTex);
    shader.SetTexture("u_DiffuseMap", 0);
}
```

### 3. Shader Integration

**Fragment Shader:**
```glsl
uniform sampler2D u_DiffuseMap;
uniform bool u_HasDiffuseMap;
uniform sampler2D u_SpecularMap;
uniform bool u_HasSpecularMap;
uniform sampler2D u_NormalMap;
uniform bool u_HasNormalMap;

void main()
{
    // Sample diffuse
    vec3 albedo = u_DiffuseColor;
    if (u_HasDiffuseMap)
        albedo *= texture(u_DiffuseMap, TexCoord).rgb;
    
    // Sample specular
    vec3 specularColor = u_SpecularColor;
    if (u_HasSpecularMap)
        specularColor *= texture(u_SpecularMap, TexCoord).r;
    
    // Sample normal map
    vec3 normal = normalize(FragNormal);
    if (u_HasNormalMap)
    {
        vec3 normalMap = texture(u_NormalMap, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;  // [0,1] -> [-1,1]
        
        // Transform to world space using TBN matrix
        vec3 T = normalize(FragTangent);
        vec3 B = normalize(cross(normal, T));
        mat3 TBN = mat3(T, B, normal);
        normal = normalize(TBN * normalMap);
    }
    
    // Lighting calculations...
}
```

## Material Properties

### Shininess
- **Range:** 1.0 - 256.0
- **Low (1-16):** Rough/matte surfaces (wood, concrete)
- **Medium (16-64):** Semi-gloss (plastic, painted metal)
- **High (64-256):** Polished/shiny (chrome, mirror)

```cpp
entity.Shininess = 32.0f;  // Moderate shininess
```

### Alpha (Transparency)
- **Range:** 0.0 - 1.0
- **0.0:** Fully transparent
- **0.5:** Half transparent
- **1.0:** Fully opaque

```cpp
entity.Alpha = 0.8f;  // Slightly transparent
```

## Scene Serialization

### Scene File Format
Entity textures are saved with their paths:

```ini
[Entity0]
Name=Textured Cube
Position=0,0,0
Rotation=0,0,0
Scale=1,1,1
MeshPath=[cube]
DiffuseTexturePath=C:\Textures\brick_diffuse.png
SpecularTexturePath=C:\Textures\brick_specular.png
NormalTexturePath=C:\Textures\brick_normal.png
Shininess=64.0
Alpha=1.0
```

### Auto-Reload
When loading a scene, textures are automatically reloaded:
```cpp
// Scene loading
if (!entity.DiffuseTexturePath.empty())
{
    entity.DiffuseTexture = LoadTextureFromFile(entity.DiffuseTexturePath);
    entity.HasDiffuseTextureOverride = true;
}
```

## Texture Types Explained

### 1. Diffuse/Albedo Map
**Purpose:** Base color of the surface

**Characteristics:**
- RGB color data
- No lighting information baked in
- Represents the "pure" color of the material

**Example:**
- Wood grain colors
- Brick color variations
- Metal base color

### 2. Specular Map
**Purpose:** Controls shininess per-pixel

**Characteristics:**
- Grayscale (uses RED channel)
- White = Shiny (full specular)
- Black = Matte (no specular)
- Can be colored for tinted reflections

**Example:**
- Wet spots on concrete (white in map)
- Scratches on metal (black in map)
- Polished areas (white) vs worn areas (black)

### 3. Normal Map
**Purpose:** Adds surface detail without geometry

**Characteristics:**
- RGB = XYZ normal direction
- Blue-ish appearance (Z+ is blue)
- Tangent space normals
- Requires tangents in mesh

**Example:**
- Brick grout depth
- Wood grain bumps
- Surface scratches and dents

## Common Workflows

### Workflow 1: Apply Texture to Single Entity

```
1. Spawn cube
2. Select cube in list
3. Inspector ? Textures ? Diffuse
4. Click "Set Override"
5. Select texture file
6. ? Cube now has unique texture
```

### Workflow 2: Share Mesh, Different Textures

```
1. Load model "Character.gltf"
2. Spawn 3 instances
3. Entity 1: Set diffuse to "red_team.png"
4. Entity 2: Set diffuse to "blue_team.png"
5. Entity 3: Set diffuse to "green_team.png"
6. ? Same mesh, different team colors!
```

### Workflow 3: Remove Override

```
1. Select entity with override
2. Inspector ? Textures ? Diffuse
3. Click "Remove Override"
4. ? Entity now uses mesh's default texture
```

## Supported Texture Formats

- **PNG** - Best for diffuse with alpha
- **JPG/JPEG** - Good for photos, no alpha
- **BMP** - Uncompressed, large files
- **TGA** - Good for all types

## Best Practices

### ? Do's

- **Use power-of-2 dimensions** (256x256, 512x512, 1024x1024)
- **Compress textures** for final builds
- **Use mipmaps** (automatic in engine)
- **Match resolution** to on-screen size
- **Use RGB for specular** if you want colored reflections
- **Test with default textures first** before overrides

### ? Don'ts

- **Don't use huge textures** (>4096x4096) for small objects
- **Don't mix different UV layouts** on same mesh
- **Don't forget to remove overrides** if not needed
- **Don't load textures every frame** (use caching)

## Performance

### Memory Usage

```cpp
// Per texture
size_t memoryPerTexture = width * height * 4 * 1.33;  // RGBA + mipmaps

// Example: 1024x1024 texture
// = 1024 * 1024 * 4 * 1.33 = 5.6 MB
```

### Entity Overhead

```cpp
// Per entity texture override
size_t entityOverhead = 
    sizeof(unsigned int) +  // Texture ID (4 bytes)
    sizeof(std::string) +   // Path (~32 bytes)
    sizeof(bool);           // Has override (1 byte)
// Total: ~37 bytes per texture override
```

### Rendering Impact

- **No override:** Zero cost (uses mesh texture)
- **With override:** 1 texture bind per entity (negligible)

## Troubleshooting

### Issue: Texture Not Showing

**Causes:**
1. File path invalid
2. Texture not loaded
3. Mesh missing UVs
4. Shader not sampling texture

**Debug:**
```cpp
std::cout << "Has override: " << entity.HasDiffuseTextureOverride << std::endl;
std::cout << "Texture ID: " << entity.DiffuseTexture << std::endl;
std::cout << "Path: " << entity.DiffuseTexturePath << std::endl;

// Check mesh UVs
mesh->PrintDebugInfo();
// Look for: UVs: 100%
```

### Issue: Wrong Texture Displayed

**Causes:**
1. Override pointing to wrong texture
2. Multiple entities sharing texture ID

**Solution:**
```cpp
// Clear all overrides first
entity.HasDiffuseTextureOverride = false;
entity.DiffuseTexture = 0;

// Then set new override
entity.DiffuseTexture = LoadTextureFromFile(newPath);
entity.HasDiffuseTextureOverride = true;
```

### Issue: Specular Map Not Working

**Causes:**
1. Specular map is colored (should be grayscale)
2. Shininess is too low/high
3. Lighting angle wrong

**Solution:**
```cpp
// Use grayscale specular map
// Convert colored image to grayscale in image editor

// Adjust shininess
entity.Shininess = 64.0f;  // Try different values

// Check lighting in shader
// Ensure light direction is correct
```

### Issue: Normal Map Looks Wrong

**Causes:**
1. Normal map not in tangent space
2. Mesh missing tangents
3. Normal map Y-axis flipped

**Solution:**
```cpp
// Check mesh has tangents
mesh->PrintDebugInfo();
// Look for: Tangents: 100%

// Flip Y in image editor (DirectX vs OpenGL convention)

// Verify tangent calculation in mesh loader
```

## Example: Complete Textured Entity

```cpp
// Create entity
Entity player;
player.name = "Player Character";
player.Transform.Position = {0, 0, 0};
player.Transform.Scale = {1, 1, 1};

// Load mesh
player.MeshHandle = MeshManager::Instance().LoadMeshSync("models/character.gltf");
player.MeshPath = "models/character.gltf";

// Apply custom textures
player.DiffuseTexture = LoadTextureFromFile("textures/player_diffuse.png");
player.DiffuseTexturePath = "textures/player_diffuse.png";
player.HasDiffuseTextureOverride = true;

player.SpecularTexture = LoadTextureFromFile("textures/player_specular.png");
player.SpecularTexturePath = "textures/player_specular.png";
player.HasSpecularTextureOverride = true;

player.NormalTexture = LoadTextureFromFile("textures/player_normal.png");
player.NormalTexturePath = "textures/player_normal.png";
player.HasNormalTextureOverride = true;

// Set material properties
player.Shininess = 48.0f;  // Semi-gloss armor
player.Alpha = 1.0f;        // Fully opaque

// Add to scene
entityManager.AddEntity(player, false);
```

## Summary

? **Per-Entity Control** - Each entity can have unique textures  
? **Easy UI** - Set textures with a few clicks  
? **Fallback System** - Uses mesh textures when no override  
? **Scene Persistence** - Textures save/load automatically  
? **Full PBR Support** - Diffuse, Specular, Normal maps  
? **Material Properties** - Shininess and alpha per entity  

Your texture system is production-ready! ??
