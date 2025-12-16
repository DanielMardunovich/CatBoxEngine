#include "RenderManager.h"
#include "../Renderable.h"
#include <algorithm>

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

void RenderManager::RemoveRenderable(Renderable* aRenderable)
{
    if (!aRenderable) return;

    std::lock_guard<std::mutex> lock(mtx);

    auto it = std::find(renderables.begin(), renderables.end(), aRenderable);
    if (it != renderables.end())
    {
        // take ownership and erase from container
        Renderable* r = *it;
        renderables.erase(it);
        delete r;
    }
    // if not found, nothing to do
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

void RenderManager::SetCamera(const Camera& camera)
{
    std::lock_guard<std::mutex> lock(mtx);
    m_Camera = camera;
}

const Camera& RenderManager::GetCamera() const
{
    return m_Camera;
}