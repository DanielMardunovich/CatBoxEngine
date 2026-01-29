# Shadow System Debug Guide

## Issue: Shadows Not Appearing

### Current Status from Screenshot:
- ? Point Light exists
- ? "Cast Shadows" is enabled
- ? Shadow map resolution: 1024
- ? **No shadows visible on model**

## Debug Steps

### 1. Verify Shadow Map Creation

**Check Console Output:**
```
Expected: [Silent or error if FBO incomplete]
Look for: "Shadow framebuffer incomplete"
```

**Verify in Code:**
The light should have:
- `ShadowMapFBO != 0`
- `ShadowMapTexture != 0`

### 2. Verify Shadow Pass Execution

**In Engine::Render():**
```cpp
// Should call this BEFORE main rendering
RenderShadowMaps();
```

**Check:**
- Is `RenderShadowMaps()` being called?
- Is viewport being restored after shadow pass?
- Is framebuffer being unbound (back to 0)?

### 3. Check Shader Uniforms

**Fragment Shader Should Receive:**
```glsl
uniform int u_NumLights;                    // Should be 1
uniform bool u_Lights[0].castsShadows;      // Should be true
uniform sampler2D u_Lights[0].shadowMap;    // Shadow texture
uniform mat4 u_LightSpaceMatrices[0];       // Light view-proj matrix
```

**In Engine.cpp, verify:**
```cpp
// Are these being set?
myShader.SetBool((base + "castsShadows").c_str(), light.CastsShadows && light.Enabled);
myShader.SetInt((base + "shadowMap").c_str(), 3 + i);
myShader.SetMat4(("u_LightSpaceMatrices[" + std::to_string(i) + "]").c_str(), light.LightSpaceMatrix);
```

### 4. Common Issues

#### Issue A: Light Direction Not Normalized
**Problem:** Point light has direction `(-0.768, -0.640, 0.000)` which is NOT normalized
**Length:** sqrt(0.768² + 0.640² + 0²) = sqrt(0.589 + 0.410) = 0.999 ? (Actually close enough)

#### Issue B: Shadow Map Not Rendering Anything
**Possible Causes:**
1. Entities not in shadow frustum
2. Shadow shader not bound
3. Depth test disabled

#### Issue C: Fragment Shader Not Sampling Shadow
**Check:** Is `CalculateShadow()` actually being called?

### 5. Quick Fixes to Try

#### Fix 1: Add Debug Visualization
**Show shadow map as overlay:**
```cpp
// In Engine.cpp, add after main rendering:
if (ImGui::Begin("Shadow Debug"))
{
    auto& lights = LightManager::Instance().GetAllLights();
    if (!lights.empty() && lights[0].ShadowMapTexture != 0)
    {
        ImGui::Image((void*)(intptr_t)lights[0].ShadowMapTexture, 
                     ImVec2(256, 256), 
                     ImVec2(0, 1), ImVec2(1, 0)); // Flip Y
    }
}
ImGui::End();
```

#### Fix 2: Verify Shadow Pass Is Running
**Add temporary debug output:**
```cpp
void Engine::RenderShadowMaps()
{
    std::cout << "=== Shadow Pass Start ===" << std::endl;
    
    auto& lights = lightMgr.GetAllLights();
    std::cout << "Lights to process: " << lights.size() << std::endl;
    
    for (size_t i = 0; i < lights.size(); ++i)
    {
        auto& light = lights[i];
        
        if (!light.CastsShadows)
        {
            std::cout << "Light " << i << ": Shadows disabled" << std::endl;
            continue;
        }
        
        if (!light.Enabled)
        {
            std::cout << "Light " << i << ": Light disabled" << std::endl;
            continue;
        }
        
        if (light.ShadowMapFBO == 0)
        {
            std::cout << "Light " << i << ": No shadow FBO!" << std::endl;
            continue;
        }
        
        std::cout << "Light " << i << ": Rendering shadows..." << std::endl;
        
        // ... rest of shadow rendering ...
    }
    
    std::cout << "=== Shadow Pass End ===" << std::endl;
}
```

#### Fix 3: Verify Fragment Shader Is Correct
**Check that FragmentShader.frag has:**
```glsl
// Should calculate shadow for each light
for (int i = 0; i < u_NumLights && i < MAX_LIGHTS; ++i)
{
    lighting += CalculateLight(i, u_Lights[i], N, V, albedo, specularColor);
}
```

**And CalculateLight() calls:**
```glsl
float shadow = CalculateShadow(lightIndex, light, normal, lightDir);
```

### 6. Most Likely Issues

Based on your setup, the most likely problems are:

**Priority 1: Shadow Map Not Being Created**
- Check if `ShadowMapFBO` is actually non-zero
- Check console for "Shadow framebuffer incomplete"

**Priority 2: Entities Not in Shadow Frustum**
- Point light at (1.1, 1.2, 0) might not "see" the model
- Shadow FOV (90°) might not cover the scene
- Try moving light higher: (0, 5, 0)

**Priority 3: Shadow Shader Not Executing**
- Verify `RenderShadowMaps()` is called
- Check if any entities are rendered in shadow pass

## Immediate Actions

### Action 1: Add Debug Output
```cpp
// In Engine::Render(), right after RenderShadowMaps():
auto& lights = LightManager::Instance().GetAllLights();
if (!lights.empty())
{
    std::cout << "Light 0: FBO=" << lights[0].ShadowMapFBO 
              << ", Tex=" << lights[0].ShadowMapTexture 
              << ", CastsShadows=" << lights[0].CastsShadows << std::endl;
}
```

### Action 2: Verify Light Position
**Current:** (1.1, 1.2, 0)  
**Try:** (0, 5, 0) - directly above model

**Why:** Point light shadows use perspective projection. If light is too close or at wrong angle, shadow frustum might not include the model.

### Action 3: Check if Shadow Map Has Content
**Add after shadow pass:**
```cpp
// Read back shadow map to verify it has data
if (lights[0].ShadowMapFBO != 0)
{
    glBindFramebuffer(GL_FRAMEBUFFER, lights[0].ShadowMapFBO);
    float depth;
    glReadPixels(512, 512, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    std::cout << "Shadow map center depth: " << depth << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
```

**Expected:** Depth should be < 1.0 if something was rendered  
**If 1.0:** Nothing rendered to shadow map (entities not in frustum or shadow pass not working)

## Expected Console Output (Working)

```
=== Shadow Pass Start ===
Lights to process: 1
Light 0: Rendering shadows...
Light 0: FBO=1, Tex=2, CastsShadows=1
Shadow map center depth: 0.3524
=== Shadow Pass End ===
```

## Expected Console Output (Broken)

```
=== Shadow Pass Start ===
Lights to process: 1
Light 0: No shadow FBO!
=== Shadow Pass End ===
```
OR
```
Light 0: FBO=1, Tex=2, CastsShadows=1
Shadow map center depth: 1.0
(Nothing rendered to shadow map)
```

## Next Steps

1. **Add debug output** (Action 1)
2. **Run the engine**
3. **Check console** for shadow pass output
4. **Report back** what you see
5. **We'll fix** based on the output

The issue is definitely one of:
- Shadow map not created (FBO = 0)
- Shadow pass not running
- Entities not in shadow frustum
- Shader not sampling shadows

With the debug output, we can pinpoint which one! ??
