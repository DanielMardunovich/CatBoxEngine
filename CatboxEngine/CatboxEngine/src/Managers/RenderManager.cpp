#include "RenderManager.h"
#include "../Renderable.h"

RenderManager& RenderManager::Get()
{
    static RenderManager instance;
    return instance;
}

void RenderManager::AddRenderable(Renderable* aRenderable)
{
    std::lock_guard<std::mutex> lock(mtx);
    renderables.push_back(aRenderable);
}

void RenderManager::RenderAll(Shader& aShader)
{
    std::lock_guard<std::mutex> lock(mtx);
    for (auto* r : renderables)
    {
        if (r) r->Render(aShader);
    }
}

RenderManager::~RenderManager()
{
    std::lock_guard<std::mutex> lock(mtx);
    for (auto* r : renderables)
        delete r;
    renderables.clear();
}