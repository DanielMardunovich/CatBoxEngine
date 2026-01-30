#include "StatsInspector.h"
#include "../../resources/EntityManager.h"
#include "../../resources/SceneManager.h"
#include "../../graphics/MeshManager.h"
#include "../../core/MemoryTracker.h"
#include "imgui.h"
#include <iostream>
#include <iomanip>

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
    ImGui::Text("Delta Time: %.4f ms", deltaTime * 1000.0f);
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
}

void StatsInspector::DrawMemoryStats(EntityManager& entityManager)
{
    ImGui::Text("Memory");

    // Mesh memory (always available)
    auto& meshMgr = MeshManager::Instance();
    const float meshCPU = meshMgr.GetTotalCPUMemory() / (1024.0f * 1024.0f);
    const float meshGPU = meshMgr.GetTotalGPUMemory() / (1024.0f * 1024.0f);
    
    ImGui::Text("Meshes: %zu", meshMgr.GetMeshCount());
    ImGui::Text("CPU: %.2f MB | GPU: %.2f MB", meshCPU, meshGPU);

#if TRACK_MEMORY
    auto& memTracker = MemoryTracker::Instance();
    ImGui::Text("Tracked: %.2f MB", memTracker.GetCurrentUsage() / (1024.0f * 1024.0f));
    ImGui::Text("Allocations: %zu", memTracker.GetActiveAllocations());

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
    const float meshCPU = meshMgr.GetTotalCPUMemory() / (1024.0f * 1024.0f);
    const float meshGPU = meshMgr.GetTotalGPUMemory() / (1024.0f * 1024.0f);

    std::cout << "\n=== MESH MEMORY ===" << std::endl;
    std::cout << "Mesh Count:     " << meshMgr.GetMeshCount() << std::endl;
    std::cout << "CPU Memory:     " << std::fixed << std::setprecision(2) << meshCPU << " MB" << std::endl;
    std::cout << "GPU Memory:     " << std::fixed << std::setprecision(2) << meshGPU << " MB" << std::endl;
    std::cout << "Total Memory:   " << std::fixed << std::setprecision(2) << (meshCPU + meshGPU) << " MB" << std::endl;
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
