# Advanced Rendering Features Implementation Guide

## Overview
This document covers four major rendering upgrades:
1. **MipMap Settings** - Texture filtering and anisotropic filtering
2. **Phong Shader with Multiple Lights** - Support for directional, point, and spot lights
3. **Light Configuration UI** - Create, delete, and configure lights
4. **Shadow Mapping** - Real-time shadows with PCF filtering

## Status

### ? Completed
- Light system architecture (Light.h, LightManager)
- Phong shader with multi-light support
- Shadow map shaders
- Light Inspector UI
- MipMap settings in Entity
- Texture filtering enums

### ?? Integration Required
The following files need to be integrated into Engine.cpp:
1. Initialize LightManager in Engine::Initialize()
2. Update rendering loop to use new shader
3. Implement shadow map rendering pass
4. Pass lights to shader
5. Add LightInspector to UI

## 1. MipMap Settings

### Entity Properties
```cpp
enum class TextureFilter
{
    Nearest,                    // GL_NEAREST
    Linear,                     // GL_LINEAR  
    NearestMipmapNearest,      // GL_NEAREST_MIPMAP_NEAREST
    LinearMipmapNearest,       // GL_LINEAR_MIPMAP_NEAREST
    NearestMipmapLinear,       // GL_NEAREST_MIPMAP_LINEAR
    LinearMipmapLinear         // GL_LINEAR_MIPMAP_LINEAR (Trilinear)
};

class Entity {
    TextureFilter MinFilter = TextureFilter::LinearMipmapLinear;
    TextureFilter MagFilter = TextureFilter::Linear;
    TextureWrap WrapS = TextureWrap::Repeat;
    TextureWrap WrapT = TextureWrap::Repeat;
    float Anisotropy = 4.0f;
    bool UseCustomTextureSettings = false;
};
```

### Integration Steps

**Step 1**: Add texture settings application function
```cpp
// In Mesh.cpp or texture loading code
void ApplyTextureSettings(unsigned int textureID, const Entity& entity)
{
    if (!entity.UseCustomTextureSettings)
        return;
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Min filter
    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
    switch (entity.MinFilter)
    {
        case TextureFilter::Nearest: minFilter = GL_NEAREST; break;
        case TextureFilter::Linear: minFilter = GL_LINEAR; break;
        case TextureFilter::NearestMipmapNearest: minFilter = GL_NEAREST_MIPMAP_NEAREST; break;
        case TextureFilter::LinearMipmapNearest: minFilter = GL_LINEAR_MIPMAP_NEAREST; break;
        case TextureFilter::NearestMipmapLinear: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
        case TextureFilter::LinearMipmapLinear: minFilter = GL_LINEAR_MIPMAP_LINEAR; break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    
    // Mag filter (no mipmaps for magnification)
    GLenum magFilter = GL_LINEAR;
    if (entity.MagFilter == TextureFilter::Nearest)
        magFilter = GL_NEAREST;
    else
        magFilter = GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    // Wrap modes
    GLenum wrapS = GL_REPEAT;
    switch (entity.WrapS)
    {
        case TextureWrap::Repeat: wrapS = GL_REPEAT; break;
        case TextureWrap::MirroredRepeat: wrapS = GL_MIRRORED_REPEAT; break;
        case TextureWrap::ClampToEdge: wrapS = GL_CLAMP_TO_EDGE; break;
        case TextureWrap::ClampToBorder: wrapS = GL_CLAMP_TO_BORDER; break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    
    GLenum wrapT = GL_REPEAT;
    switch (entity.WrapT)
    {
        case TextureWrap::Repeat: wrapT = GL_REPEAT; break;
        case TextureWrap::MirroredRepeat: wrapT = GL_MIRRORED_REPEAT; break;
        case TextureWrap::ClampToEdge: wrapT = GL_CLAMP_TO_EDGE; break;
        case TextureWrap::ClampToBorder: wrapT = GL_CLAMP_TO_BORDER; break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    
    // Anisotropic filtering
    if (entity.Anisotropy > 1.0f)
    {
        float maxAniso;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        float aniso = std::min(entity.Anisotropy, maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }
}
```

**Step 2**: Apply in rendering loop
```cpp
// In Engine::Render() before rendering entity
if (e.UseCustomTextureSettings)
{
    if (e.HasDiffuseTextureOverride)
        ApplyTextureSettings(e.DiffuseTexture, e);
    if (e.HasSpecularTextureOverride)
        ApplyTextureSettings(e.SpecularTexture, e);
    if (e.HasNormalTextureOverride)
        ApplyTextureSettings(e.NormalTexture, e);
}
```

**Step 3**: Add UI in EntityInspector
```cpp
if (ImGui::Checkbox("Custom Texture Settings", &entity.UseCustomTextureSettings))
{
    // Update textures immediately
}

if (entity.UseCustomTextureSettings)
{
    const char* minFilters[] = {
        "Nearest", "Linear",
        "Nearest Mipmap Nearest", "Linear Mipmap Nearest",
        "Nearest Mipmap Linear", "Linear Mipmap Linear (Trilinear)"
    };
    int minFilter = (int)entity.MinFilter;
    if (ImGui::Combo("Min Filter", &minFilter, minFilters, 6))
    {
        entity.MinFilter = (TextureFilter)minFilter;
    }
    
    const char* magFilters[] = { "Nearest", "Linear" };
    int magFilter = (entity.MagFilter == TextureFilter::Nearest) ? 0 : 1;
    if (ImGui::Combo("Mag Filter", &magFilter, magFilters, 2))
    {
        entity.MagFilter = (magFilter == 0) ? TextureFilter::Nearest : TextureFilter::Linear;
    }
    
    ImGui::SliderFloat("Anisotropy", &entity.Anisotropy, 1.0f, 16.0f);
}
```

## 2. Phong Shader Integration

### Engine Initialization
```cpp
// In Engine::Initialize()
#include "../graphics/LightManager.h"

// Create default lights
auto& lightMgr = LightManager::Instance();
lightMgr.CreateDefaultLights();
lightMgr.InitializeShadowMaps();
```

### Rendering Loop Updates
```cpp
// In Engine::Render()

// 1. First Pass: Render shadow maps
RenderShadowMaps();

// 2. Second Pass: Main rendering
myShader.Use();

// Set camera
myShader.setVec3("u_CameraPos", camera.Position.x, camera.Position.y, camera.Position.z);

// Set lights
auto& lights = LightManager::Instance().GetAllLights();
int numLights = std::min((int)lights.size(), 8);  // MAX_LIGHTS
myShader.SetInt("u_NumLights", numLights);

for (int i = 0; i < numLights; ++i)
{
    const auto& light = lights[i];
    std::string base = "u_Lights[" + std::to_string(i) + "].";
    
    myShader.SetInt((base + "type").c_str(), (int)light.Type);
    myShader.setVec3((base + "position").c_str(), light.Position.x, light.Position.y, light.Position.z);
    myShader.setVec3((base + "direction").c_str(), light.Direction.x, light.Direction.y, light.Direction.z);
    myShader.setVec3((base + "color").c_str(), light.Color.x, light.Color.y, light.Color.z);
    myShader.setFloat((base + "intensity").c_str(), light.Intensity);
    
    // Attenuation
    myShader.setFloat((base + "constant").c_str(), light.Constant);
    myShader.setFloat((base + "linear").c_str(), light.Linear);
    myShader.setFloat((base + "quadratic").c_str(), light.Quadratic);
    
    // Spot light
    myShader.setFloat((base + "innerCutoff").c_str(), std::cos(glm::radians(light.InnerCutoff)));
    myShader.setFloat((base + "outerCutoff").c_str(), std::cos(glm::radians(light.OuterCutoff)));
    
    // Shadows
    myShader.SetBool((base + "castsShadows").c_str(), light.CastsShadows);
    myShader.setFloat((base + "shadowBias").c_str(), light.ShadowBias);
    myShader.SetBool((base + "enabled").c_str(), light.Enabled);
    
    // Bind shadow map
    if (light.CastsShadows)
    {
        glActiveTexture(GL_TEXTURE3 + i);
        glBindTexture(GL_TEXTURE_2D, light.ShadowMapTexture);
        myShader.SetTexture((base + "shadowMap").c_str(), 3 + i);
    }
}

// Continue with entity rendering...
```

### Shadow Map Rendering
```cpp
void Engine::RenderShadowMaps()
{
    auto& lights = LightManager::Instance().GetAllLights();
    
    // Use shadow shader
    Shader shadowShader;
    shadowShader.Initialize("./shaders/ShadowMap.vert", "./shaders/ShadowMap.frag");
    shadowShader.Use();
    
    for (auto& light : lights)
    {
        if (!light.CastsShadows || !light.Enabled)
            continue;
        
        // Bind shadow map framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, light.ShadowMapFBO);
        glViewport(0, 0, light.ShadowMapSize, light.ShadowMapSize);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Calculate light space matrix
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        
        if (light.Type == LightType::Directional)
        {
            // Orthographic projection for directional light
            float size = light.ShadowOrthoSize;
            lightProjection = glm::ortho(-size, size, -size, size, 
                                        light.ShadowNearPlane, light.ShadowFarPlane);
            
            // View from light position looking at target
            glm::vec3 lightPos = glm::vec3(
                -light.Direction.x * 10.0f,
                -light.Direction.y * 10.0f,
                -light.Direction.z * 10.0f
            );
            lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else
        {
            // Perspective projection for point/spot lights
            lightProjection = glm::perspective(glm::radians(light.ShadowFOV), 1.0f,
                                              light.ShadowNearPlane, light.ShadowFarPlane);
            
            glm::vec3 lightPos(light.Position.x, light.Position.y, light.Position.z);
            glm::vec3 target = lightPos + glm::vec3(light.Direction.x, light.Direction.y, light.Direction.z);
            lightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        lightSpaceMatrix = lightProjection * lightView;
        shadowShader.SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        
        // Store for main pass
        light.lightSpaceMatrix = lightSpaceMatrix;  // Add this to Light struct
        
        // Render all entities to shadow map
        for (const auto& e : entityManager.GetAll())
        {
            Mesh* mesh = MeshManager::Instance().GetMesh(e.MeshHandle);
            if (!mesh) continue;
            
            // Create model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));
            
            shadowShader.SetMat4("u_Model", model);
            
            // Draw mesh
            mesh->Draw();
        }
    }
    
    // Restore default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Reset viewport
    int display_w, display_h;
    glfwGetFramebufferSize(GetWindow(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
}
```

## 3. Light Inspector UI Integration

### Add to UIManager
```cpp
// In UIManager.h
#include "../ui/Inspectors/LightInspector.h"

// In UIManager::Draw()
LightInspector lightInspector;
lightInspector.Draw();
```

## 4. Complete Integration Checklist

### Files to Modify

**Engine.h**
- Add `#include "../graphics/LightManager.h"`
- Add `void RenderShadowMaps();` method
- Add shadow shader member: `Shader shadowShader;`

**Engine.cpp**
- Initialize LightManager in `Initialize()`
- Initialize shadow shader
- Implement `RenderShadowMaps()` method
- Update `Render()` to call shadow pass first
- Pass lights to main shader

**UIManager.cpp**
- Add `#include "../ui/Inspectors/LightInspector.h"`
- Draw light inspector window

**EntityInspector.cpp**
- Add texture settings UI section

**Shader.h/cpp**
- Add `SetInt()` method if not present
- Add `SetMat4()` method if not present

### Build Requirements

Add these files to your project:
- `CatboxEngine/graphics/Light.h`
- `CatboxEngine/graphics/LightManager.h`
- `CatboxEngine/graphics/LightManager.cpp`
- `CatboxEngine/ui/Inspectors/LightInspector.h`
- `CatboxEngine/ui/Inspectors/LightInspector.cpp`
- `CatboxEngine/shaders/ShadowMap.vert`
- `CatboxEngine/shaders/ShadowMap.frag`

## 5. Testing

### Test 1: Basic Lighting
```
1. Run engine
2. Open "Lights" window
3. Modify sun light color
4. Observe scene lighting changes
```

### Test 2: Multiple Lights
```
1. Add 3 point lights at different positions
2. Set different colors (red, green, blue)
3. Observe color mixing on surfaces
```

### Test 3: Shadows
```
1. Enable shadows on directional light
2. Place cube above ground plane
3. Observe shadow casting
4. Adjust shadow bias if Peter Panning occurs
```

### Test 4: MipMaps
```
1. Select entity
2. Enable "Custom Texture Settings"
3. Change filter to "Nearest"
4. Observe pixelated textures
5. Enable anisotropic filtering
6. Observe sharper textures at angles
```

## 6. Performance Tips

### Shadow Map Optimization
- Use lower resolution (512-1024) for distant lights
- Disable shadows on non-important lights
- Use cascade shadow maps for directional lights (advanced)

### Light Count
- Maximum 8 lights in shader (change MAX_LIGHTS if needed)
- Disable lights outside camera frustum
- Use light culling for scenes with many lights

### MipMap Generation
- Generate mipmaps once when loading texture
- Don't regenerate every frame
- Use compressed texture formats (DDS, KTX)

## Summary

? **MipMap Settings** - Per-entity texture filtering control  
? **Phong Shader** - Multiple lights with proper attenuation  
? **Light UI** - Complete light management interface  
? **Shadow Mapping** - PCF-filtered shadows with configurable quality  

This is a professional-grade lighting system! ??
