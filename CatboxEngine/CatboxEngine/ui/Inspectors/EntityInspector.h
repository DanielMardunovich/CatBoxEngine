#pragma once
#include "../../resources/Entity.h"
#include "../../core/Platform.h"

class EntityInspector
{
public:
    EntityInspector() = default;
    ~EntityInspector() = default;

    // Disable copy, enable move
    EntityInspector(const EntityInspector&) = delete;
    EntityInspector& operator=(const EntityInspector&) = delete;
    EntityInspector(EntityInspector&&) noexcept = default;
    EntityInspector& operator=(EntityInspector&&) noexcept = default;

    void Draw(Entity& entity);
};
