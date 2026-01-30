#pragma once

class EntityManager;

class StatsInspector
{
public:
    StatsInspector() = default;
    ~StatsInspector() = default;

    // Disable copy, enable move
    StatsInspector(const StatsInspector&) = delete;
    StatsInspector& operator=(const StatsInspector&) = delete;
    StatsInspector(StatsInspector&&) noexcept = default;
    StatsInspector& operator=(StatsInspector&&) noexcept = default;

    void Draw(float deltaTime, EntityManager& entityManager);

private:
    void DrawTimingStats(float deltaTime);
    void DrawMemoryStats(EntityManager& entityManager);
    void PrintMemoryReport(EntityManager& entityManager);
};
