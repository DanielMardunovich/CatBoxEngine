#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <queue>
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
    // register a callback to be invoked on the main thread when the given handle finishes loading
    void RegisterLoadCallback(MeshHandle h, std::function<void(MeshHandle)> cb);
    // poll completed loads and invoke callbacks (call from main thread)
    void PollCompleted();

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
    // refcounting
    void AddRef(MeshHandle h);
    // async completion queue and callbacks
    std::queue<MeshHandle> m_completed;
    std::unordered_map<MeshHandle, std::vector<std::function<void(MeshHandle)>>> m_callbacks;
};
