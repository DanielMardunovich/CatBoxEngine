# GLTF Texture Troubleshooting Guide

## Problem: Model loads but no textures appear

### Solution Applied
The GLTF loader now properly handles both:
- **Embedded textures** (in GLB files)
- **External textures** (in GLTF files with separate image files)

## Check Console Output

When loading a GLTF model, you should see detailed output:

```
Loading GLTF: model.gltf
  Meshes: 1
  Materials: 2
  Textures: 3
  Images: 3
  Processing mesh 'Character' with 2 primitives
    Primitive 0 - Material: Body
    Loading baseColor texture (index=0)...
    Loading embedded texture...
    ? Texture loaded successfully (ID=5, 2048x2048)
    Loading normal texture (index=1)...
    Loading embedded texture...
    ? Texture loaded successfully (ID=6, 2048x2048)
GLTF loaded: 5420 vertices, 2 submeshes
```

## Common Issues & Fixes

### 1. "Failed to load texture"
**Cause:** External texture file not found  
**Check:**
- Is the texture file in the same directory as the GLTF?
- Does the file path match exactly (case-sensitive on Linux)?

**Solution:**
```cpp
// GLTF file structure:
model.gltf
textures/
  diffuse.png
  normal.png
```

### 2. Textures appear black
**Cause:** Texture loaded but not bound properly in shader  
**Check:** Look for these in console:
```
? Texture loaded successfully (ID=X, widthxheight)
```
If you see this, the texture IS loaded. Problem is in rendering.

**Debug:** Check your shader has these uniforms:
```glsl
uniform sampler2D u_DiffuseMap;
uniform bool u_HasDiffuseMap;
```

### 3. Only some materials have textures
**Cause:** Normal behavior for multi-material models  
**Each submesh** has its own material and textures.

### 4. GLB files vs GLTF files

**GLB (Binary):**
- ? All data embedded (geometry + textures)
- ? Single file
- ? Faster loading
- ? Harder to edit

**GLTF (ASCII):**
- ? Human-readable
- ? Easy to edit
- ? Requires external texture files
- ? Multiple files to manage

## Testing Checklist

1. **Console shows texture loaded?**
   - ? Yes ? Problem is in rendering/shaders
   - ? No ? Problem is in loading

2. **GLB or GLTF?**
   - GLB: Textures should be embedded
   - GLTF: Check texture files exist

3. **Material has baseColorTexture?**
   ```
   # In GLTF file, check materials:
   "materials": [{
     "pbrMetallicRoughness": {
       "baseColorTexture": { "index": 0 }  // ? Should exist
     }
   }]
   ```

4. **Check OpenGL errors**
   Add this after texture loading:
   ```cpp
   GLenum err;
   while ((err = glGetError()) != GL_NO_ERROR) {
       std::cerr << "OpenGL error: " << err << std::endl;
   }
   ```

## Debug Commands

### Enable verbose output
Already enabled! Check console for:
- "Loading embedded texture..." ? GLB embedded data
- "Loading external texture: path" ? External file
- "? Texture loaded successfully" ? Worked!

### Check texture ID is valid
```cpp
if (sub.DiffuseTexture != 0) {
    std::cout << "Diffuse texture ID: " << sub.DiffuseTexture << std::endl;
}
```

### Verify texture is bound during render
In `Engine.cpp`, add debug:
```cpp
if (sub.HasDiffuseTexture)
{
    std::cout << "Binding texture " << sub.DiffuseTexture << std::endl;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sub.DiffuseTexture);
}
```

## Export Settings

### Blender GLTF Export
Recommended settings:
- ? Include: Selected Objects
- ? Transform: +Y Up
- ? Geometry: Apply Modifiers
- ? Materials: Export
- ? Images: Automatic (for GLB) or JPEG/PNG (for GLTF)
- ? Compression: None

### Texture Resolution
Large textures (4K+) might fail:
- Try 2K (2048x2048) max
- Use power-of-2 sizes (1024, 2048, 4096)

## Still Not Working?

Share this console output:
1. The full GLTF loading output
2. Any error messages
3. Whether GLB or GLTF
4. Blender export settings used

The new verbose logging will help diagnose the exact issue!
