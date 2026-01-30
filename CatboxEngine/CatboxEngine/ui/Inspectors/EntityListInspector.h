#pragma once

class EntityManager;

class EntityListInspector
{
public:
    EntityListInspector() = default;
    ~EntityListInspector() = default;

    // Disable copy, enable move
    EntityListInspector(const EntityListInspector&) = delete;
    EntityListInspector& operator=(const EntityListInspector&) = delete;
    EntityListInspector(EntityListInspector&&) noexcept = default;
    EntityListInspector& operator=(EntityListInspector&&) noexcept = default;

    void Draw(EntityManager& entityManager, int& selectedIndex);
};
