#pragma once
#include <vector>
#include <mutex>
#include "../Renderable.h"
#include "../Shader.h"

class RenderManager
{
public:
    static RenderManager& Get();

    void AddRenderable(Renderable* aRenderable);
    void RenderAll(Shader& aShader);

private:
    RenderManager() = default;
    ~RenderManager();

    std::vector<Renderable*> renderables;
    std::mutex mtx;
};