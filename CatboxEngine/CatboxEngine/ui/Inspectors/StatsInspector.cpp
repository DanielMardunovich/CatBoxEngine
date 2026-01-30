#include "StatsInspector.h"
#include "../../resources/EntityManager.h"
#include "../../resources/SceneManager.h"
#include "../../graphics/MeshManager.h"
#include "../../core/MemoryTracker.h"
#include "imgui.h"
#include <iostream>
#include <iomanip>

namespace
{
    // Memory conversion
    constexpr float BYTES_TO_MB = 1.0f / (1024.0f * 1024.0f);
    constexpr float MS_TO_SECONDS = 1000.0f;
    
    // UI constants
    constexpr int DECIMAL_PRECISION = 2;
}

void StatsInspector::Draw(float deltaTime, EntityManager& entityManager)
{
    ImGui::Begin("Statistics");

    DrawTimingStats(deltaTime);
    
    ImGui::Separator();
    
    DrawMemoryStats(entityManager);

    ImGui::End();
}

void StatsInspector::DrawTimingStats(float deltaTime)
{
    ImGui::Text("Performance");
    ImGui::Spacing();
    
    ImGui::Text("Delta Time: %.4f ms", deltaTime * MS_TO_SECONDS);
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
}

void StatsInspector::DrawMemoryStats(EntityManager& entityManager)
{
    ImGui::Text("Memory");
    ImGui::Spacing();

    // Mesh memory
    auto& meshMgr = MeshManager::Instance();
    const float meshCPU = meshMgr.GetTotalCPUMemory() * BYTES_TO_MB;
    const float meshGPU = meshMgr.GetTotalGPUMemory() * BYTES_TO_MB;
    
    ImGui::Text("Meshes: %zu", meshMgr.GetMeshCount());
    ImGui::Text("CPU: %.2f MB", meshCPU);
    ImGui::Text("GPU: %.2f MB", meshGPU);

    ImGui::Spacing();

#if TRACK_MEMORY
    auto& memTracker = MemoryTracker::Instance();
    ImGui::Text("Tracked: %.2f MB", memTracker.GetCurrentUsage() * BYTES_TO_MB);
    ImGui::Text("Allocations: %zu", memTracker.GetActiveAllocations());

    ImGui::Spacing();

    if (ImGui::Button("Print Report"))
    {
        PrintMemoryReport(entityManager);
    }
    ImGui::SameLine();
    if (ImGui::Button("Check Leaks"))
    {
        memTracker.CheckForLeaks();
    }
#else
    if (ImGui::Button("Print Report"))
    {
        PrintMemoryReport(entityManager);
    }
    ImGui::TextDisabled("(Build in DEBUG for detailed tracking)");
#endif
}

void StatsInspector::PrintMemoryReport(EntityManager& entityManager)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "    COMPREHENSIVE MEMORY REPORT" << std::endl;
    std::cout << "========================================" << std::endl;

#if TRACK_MEMORY
    // Tracked allocations (debug only)
    auto& memTracker = MemoryTracker::Instance();
    memTracker.PrintMemoryReport();
#endif

    // Mesh memory
    auto& meshMgr = MeshManager::Instance();
    const float meshCPU = meshMgr.GetTotalCPUMemory() * BYTES_TO_MB;
    const float meshGPU = meshMgr.GetTotalGPUMemory() * BYTES_TO_MB;

    std::cout << "\n=== MESH MEMORY ===" << std::endl;
    std::cout << "Mesh Count:     " << meshMgr.GetMeshCount() << std::endl;
    std::cout << "CPU Memory:     " << std::fixed << std::setprecision(DECIMAL_PRECISION) << meshCPU << " MB" << std::endl;
    std::cout << "GPU Memory:     " << std::fixed << std::setprecision(DECIMAL_PRECISION) << meshGPU << " MB" << std::endl;
    std::cout << "Total Memory:   " << std::fixed << std::setprecision(DECIMAL_PRECISION) << (meshCPU + meshGPU) << " MB" << std::endl;
    std::cout << "===================\n" << std::endl;

    // Scene memory
    auto& sceneMgr = SceneManager::Instance();
    std::cout << "=== SCENE MEMORY ===" << std::endl;
    std::cout << "Scene Count:    " << sceneMgr.GetSceneCount() << std::endl;
    auto* activeScene = sceneMgr.GetActiveScene();
    if (activeScene)
    {
        std::cout << "Active Scene:   " << activeScene->GetName() << std::endl;
        std::cout << "Entities:       " << activeScene->GetEntityCount() << std::endl;
    }
    std::cout << "====================\n" << std::endl;

#if !TRACK_MEMORY
    std::cout << "Note: Build in DEBUG mode for detailed allocation tracking" << std::endl;
#endif
    std::cout << "========================================\n" << std::endl;
}
