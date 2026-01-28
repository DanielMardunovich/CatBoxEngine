#include "MeshManager.h"
#include "Mesh.h"
#include "../core/MessageQueue.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <memory>

MeshManager::MeshManager() {}
MeshManager::~MeshManager() {}

MeshManager& MeshManager::Instance()
{
    static MeshManager inst;
    return inst;
}

MeshHandle MeshManager::CreateEntryForPath(const std::string& path)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_pathToHandle.find(path);
    if (it != m_pathToHandle.end())
    {
        // existing entry: increment refcount before returning
        auto eh = m_entries.find(it->second);
        if (eh != m_entries.end()) eh->second->refcount++;
        return it->second;
    }

    MeshHandle h = m_nextHandle.fetch_add(1);
    auto e = std::make_shared<Entry>();
    e->path = path;
    e->refcount = 1;
    m_entries[h] = e;
    m_pathToHandle[path] = h;
    return h;
}

void MeshManager::AddRef(MeshHandle h)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(h);
    if (it != m_entries.end()) it->second->refcount++;
}

MeshHandle MeshManager::LoadMeshSync(const std::string& path)
{
    MeshHandle h = CreateEntryForPath(path);
    auto e = m_entries[h];
    if (!e->loaded)
    {
        Mesh m;
        bool ok = false;
        auto extpos = path.find_last_of('.');
        if (extpos != std::string::npos)
        {
            std::string ext = path.substr(extpos+1);
            for (auto &c : ext) c = (char)tolower(c);
            if (ext == "obj") ok = m.LoadFromOBJ(path);
            else if (ext == "gltf" || ext == "glb") ok = m.LoadFromGLTF(path);
            else
            {
                std::cerr << "Unsupported file format: " << ext << std::endl;
                ok = m.LoadFromOBJ(path); // Fallback
            }
        }
        else ok = m.LoadFromOBJ(path);

        if (!ok)
        {
            // Post failure message
            auto msg = std::make_shared<MeshLoadFailedMessage>(path, "Failed to load mesh");
            MessageQueue::Instance().Post(msg);
            return 0;
        }
        m.Upload();
        e->mesh = std::move(m);
        e->loaded = true;
        
        // Post success message
        auto msg = std::make_shared<MeshLoadedMessage>(path, h);
        MessageQueue::Instance().Post(msg);
    }
    return h;
}

MeshHandle MeshManager::LoadMeshAsync(const std::string& path)
{
    MeshHandle h = CreateEntryForPath(path);
    auto e = m_entries[h];
    if (e->loaded) return h;

    // spawn background thread to load
    std::thread([this, h, path]() {
        Mesh m;
        bool ok = false;
        auto extpos = path.find_last_of('.');
        if (extpos != std::string::npos)
        {
            std::string ext = path.substr(extpos+1);
            for (auto &c : ext) c = (char)tolower(c);
            if (ext == "obj") ok = m.LoadFromOBJ(path);
            else if (ext == "gltf" || ext == "glb") ok = m.LoadFromGLTF(path);
            else
            {
                std::cerr << "Unsupported file format: " << ext << std::endl;
                ok = m.LoadFromOBJ(path);
            }
        }
        else ok = m.LoadFromOBJ(path);

        if (!ok)
        {
            // Post failure message
            auto msg = std::make_shared<MeshLoadFailedMessage>(path, "Failed to load mesh");
            MessageQueue::Instance().Post(msg);
            return;
        }
        m.Upload();
        auto e = m_entries[h];
        e->mesh = std::move(m);
        e->loaded = true;
        // notify main thread by pushing into queue
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_completed.push(h);
        }
        
        // Post success message
        auto msg = std::make_shared<MeshLoadedMessage>(path, h);
        MessageQueue::Instance().Post(msg);
    }).detach();

    return h;
}

Mesh* MeshManager::GetMesh(MeshHandle h)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(h);
    if (it == m_entries.end()) return nullptr;
    if (!it->second->loaded) return nullptr;
    return &it->second->mesh;
}

void MeshManager::Release(MeshHandle h)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_entries.find(h);
    if (it == m_entries.end()) return;
    if (--it->second->refcount <= 0)
    {
        m_pathToHandle.erase(it->second->path);
        m_entries.erase(it);
    }
}

void MeshManager::RegisterLoadCallback(MeshHandle h, std::function<void(MeshHandle)> cb)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_callbacks[h].push_back(cb);
}

void MeshManager::PollCompleted()
{
    std::queue<MeshHandle> q;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        std::swap(q, m_completed);
    }
    while (!q.empty())
    {
        MeshHandle h = q.front(); q.pop();
        std::vector<std::function<void(MeshHandle)>> cbs;
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            auto it = m_callbacks.find(h);
            if (it != m_callbacks.end()) cbs = it->second;
        }
        for (auto &cb : cbs) cb(h);
    }
}

MeshHandle MeshManager::GetSharedCubeHandle()
{
    static MeshHandle sharedHandle = 0;
    if (sharedHandle != 0)
    {
        // increment refcount for each caller
        std::lock_guard<std::mutex> lk(m_mutex);
        auto it = m_entries.find(sharedHandle);
        if (it != m_entries.end()) it->second->refcount++;
        return sharedHandle;
    }
    // create a shared cube entry and synchronously load
    std::string key = "__shared_cube";
    MeshHandle h = CreateEntryForPath(key);
    auto e = m_entries[h];
    if (!e->loaded)
    {
        e->mesh = CreateCubeMesh();
        e->mesh.Upload();
        e->loaded = true;
    }
    sharedHandle = h;
    return sharedHandle;
}

// simple cube creator
Mesh MeshManager::CreateCubeMesh()
{
    Mesh mesh;
    // reuse previous implementation in-line
    std::vector<Vertex> verts;
    verts.reserve(24);
    auto push = [&](float px, float py, float pz, float nx, float ny, float nz, float u, float v){
        Vertex vv; vv.Position = {px,py,pz}; vv.Normal = {nx,ny,nz}; vv.UV = {u,v,0}; vv.Tangent = {0,0,0}; verts.push_back(vv);
    };
    push(-0.5f,-0.5f,0.5f, 0,0,1, 0,0);
    push(0.5f,-0.5f,0.5f, 0,0,1, 1,0);
    push(0.5f,0.5f,0.5f, 0,0,1, 1,1);
    push(-0.5f,0.5f,0.5f, 0,0,1, 0,1);
    push(0.5f,-0.5f,-0.5f, 0,0,-1, 0,0);
    push(-0.5f,-0.5f,-0.5f, 0,0,-1, 1,0);
    push(-0.5f,0.5f,-0.5f, 0,0,-1, 1,1);
    push(0.5f,0.5f,-0.5f, 0,0,-1, 0,1);
    push(-0.5f,-0.5f,-0.5f, -1,0,0, 0,0);
    push(-0.5f,-0.5f,0.5f, -1,0,0, 1,0);
    push(-0.5f,0.5f,0.5f, -1,0,0, 1,1);
    push(-0.5f,0.5f,-0.5f, -1,0,0, 0,1);
    push(0.5f,-0.5f,0.5f, 1,0,0, 0,0);
    push(0.5f,-0.5f,-0.5f, 1,0,0, 1,0);
    push(0.5f,0.5f,-0.5f, 1,0,0, 1,1);
    push(0.5f,0.5f,0.5f, 1,0,0, 0,1);
    push(-0.5f,0.5f,0.5f, 0,1,0, 0,0);
    push(0.5f,0.5f,0.5f, 0,1,0, 1,0);
    push(0.5f,0.5f,-0.5f, 0,1,0, 1,1);
    push(-0.5f,0.5f,-0.5f, 0,1,0, 0,1);
    push(-0.5f,-0.5f,-0.5f, 0,-1,0, 0,0);
    push(0.5f,-0.5f,-0.5f, 0,-1,0, 1,0);
    push(0.5f,-0.5f,0.5f, 0,-1,0, 1,1);
    push(-0.5f,-0.5f,0.5f, 0,-1,0, 0,1);
    mesh.Vertices = verts;
    mesh.Indices = std::vector<uint32_t>{
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        8,9,10, 10,11,8,
        12,13,14, 14,15,12,
        16,17,18, 18,19,16,
        20,21,22, 22,23,20
    };
    return mesh;
}
