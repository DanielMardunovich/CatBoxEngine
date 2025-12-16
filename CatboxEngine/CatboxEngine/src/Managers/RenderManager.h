#pragma once
#include <vector>
#include <mutex>
#include "../Renderable.h"
#include "../Shader.h"
#include "../Camera.h"

class RenderManager
{
public:
    static RenderManager& Get();

    void AddRenderable(Renderable* aRenderable);
    void RenderAll(Shader& aShader);
    void SetCamera(const Camera& camera);
	const Camera& GetCamera() const;

    // Remove a previously added renderable. If found it will be deleted and removed from the list.
    // Safe to call with nullptr (no-op).
    void RemoveRenderable(Renderable* aRenderable);

private:
    RenderManager() = default;
    ~RenderManager();

    std::vector<Renderable*> renderables;
    std::mutex mtx;
    Camera m_Camera;
};