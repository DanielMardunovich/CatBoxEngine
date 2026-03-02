#include "EntityManager.h"
#include "../graphics/MeshManager.h"
#include "../core/MessageQueue.h"
#include <memory>

int EntityManager::AddEntity(const Entity& e, bool /*useSharedCube*/)
{
    m_entities.push_back(e);
    int idx = (int)m_entities.size() - 1;

    Entity& ent = m_entities[idx];
    // if caller provided a MeshHandle in e.MeshHandle use that, otherwise create or assign a cube
    if (ent.MeshHandle == 0)
    {
        MeshHandle h = MeshManager::Instance().GetSharedCubeHandle();
        ent.MeshHandle = h;
        ent.MeshPath = "[cube]";
    }

    // Post entity created message
    auto msg = std::make_shared<EntityCreatedMessage>(idx, ent.name);
    MessageQueue::Instance().Post(msg);

    return idx;
}

