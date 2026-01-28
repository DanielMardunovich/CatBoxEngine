#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include "Mesh.h"

using MeshHandle = uint32_t;

class MeshManager
{
public:
    MeshManager();
    ~MeshManager();

    static MeshManager& Instance();

    // synchronous load (blocks until loaded). returns 0 on failure, otherwise handle > 0
    MeshHandle LoadMeshSync(const std::string& path);
    // asynchronous load: returns a handle immediately; mesh may not be available yet
    MeshHandle LoadMeshAsync(const std::string& path);

    // get mesh pointer for a handle (returns nullptr if not loaded or invalid)
    Mesh* GetMesh(MeshHandle h);

    // release a handle (decrement refcount); when refcount reaches 0 the mesh may be unloaded
    void Release(MeshHandle h);

    // get handle for shared cube
    MeshHandle GetSharedCubeHandle();

private:
    struct Entry
    {
        std::string path;
        Mesh mesh;
        std::atomic<int> refcount{0};
        std::atomic<bool> loaded{false};
    };

    std::mutex m_mutex;
    std::unordered_map<std::string, MeshHandle> m_pathToHandle;
    std::unordered_map<MeshHandle, std::shared_ptr<Entry>> m_entries;
    std::atomic<MeshHandle> m_nextHandle{1};

    MeshHandle CreateEntryForPath(const std::string& path);
    static Mesh CreateCubeMesh();
};
