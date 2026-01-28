#include "Scene.h"
#include "EntityManager.h"
#include "../graphics/MeshManager.h"
#include "../core/MessageQueue.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

Scene::Scene(const std::string& name)
    : m_name(name)
{
    // Set creation time
    auto now = std::chrono::system_clock::now();
    m_metadata.createdTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    m_metadata.modifiedTime = m_metadata.createdTime;
}

Scene::~Scene()
{
    if (m_isLoaded)
    {
        std::cerr << "Warning: Scene destroyed while still loaded!" << std::endl;
    }
}

void Scene::OnLoad(EntityManager& entityManager)
{
    if (m_isLoaded) return;
    
    std::cout << "Loading scene: " << m_name << " (" << m_entities.size() << " entities)" << std::endl;
    m_isLoaded = true;
    
    // Clear existing entities in manager
    entityManager.Clear();
    
    // Load all entities from this scene into the entity manager
    for (const auto& entity : m_entities)
    {
        entityManager.AddEntity(entity, false); // false = don't save to scene (we already have them)
    }
    
    std::cout << "  Loaded " << m_entities.size() << " entities into scene" << std::endl;
}

void Scene::OnUnload(EntityManager& entityManager)
{
    if (!m_isLoaded) return;
    
    std::cout << "Unloading scene: " << m_name << std::endl;
    
    // Capture current state before unloading
    CaptureFromEntityManager(entityManager);
    
    // Clear entity manager
    entityManager.Clear();
    
    m_isLoaded = false;
}

void Scene::CaptureFromEntityManager(EntityManager& entityManager)
{
    // Save current state of entities from EntityManager to Scene
    m_entities.clear();
    
    const auto& entities = entityManager.GetAll();
    for (const auto& entity : entities)
    {
        m_entities.push_back(entity);
    }
    
    // Update modified time
    auto now = std::chrono::system_clock::now();
    m_metadata.modifiedTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    std::cout << "  Captured " << m_entities.size() << " entities from EntityManager" << std::endl;
}

void Scene::Update(float deltaTime)
{
    // Update camera (note: Camera::Update needs GLFWwindow, so scene update is minimal)
    // Camera updates are handled by Engine directly
    
    // Scene-specific update logic can go here
    (void)deltaTime;
}

void Scene::AddEntity(const Entity& entity)
{
    m_entities.push_back(entity);
    
    auto now = std::chrono::system_clock::now();
    m_metadata.modifiedTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
}

void Scene::RemoveEntity(size_t index)
{
    if (index < m_entities.size())
    {
        m_entities.erase(m_entities.begin() + index);
        
        auto now = std::chrono::system_clock::now();
        m_metadata.modifiedTime = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
    }
}

Entity* Scene::GetEntity(size_t index)
{
    if (index < m_entities.size())
    {
        return &m_entities[index];
    }
    return nullptr;
}

void Scene::ClearEntities()
{
    m_entities.clear();
    
    auto now = std::chrono::system_clock::now();
    m_metadata.modifiedTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
}

bool Scene::SaveToFile(const std::string& path) const
{
    std::ofstream out(path);
    if (!out.is_open())
    {
        std::cerr << "Failed to save scene to: " << path << std::endl;
        return false;
    }
    
    out << "[Scene]" << std::endl;
    out << "Name=" << m_name << std::endl;
    out << "Version=" << m_metadata.version << std::endl;
    out << "Author=" << m_metadata.author << std::endl;
    out << "Description=" << m_metadata.description << std::endl;
    
    out << "\n[Camera]" << std::endl;
    out << "Position=" << m_camera.Position.x << "," << m_camera.Position.y << "," << m_camera.Position.z << std::endl;
    out << "Yaw=" << m_camera.Yaw << std::endl;
    out << "Pitch=" << m_camera.Pitch << std::endl;
    out << "FOV=" << m_camera.FOV << std::endl;
    out << "Near=" << m_camera.Near << std::endl;
    out << "Far=" << m_camera.Far << std::endl;
    
    out << "\n[Light]" << std::endl;
    out << "Direction=" << m_light.Direction.x << "," << m_light.Direction.y << "," << m_light.Direction.z << std::endl;
    out << "Color=" << m_light.Color.x << "," << m_light.Color.y << "," << m_light.Color.z << std::endl;
    out << "Intensity=" << m_light.Intensity << std::endl;
    
    out << "\n[Environment]" << std::endl;
    out << "Ambient=" << AmbientColor.x << "," << AmbientColor.y << "," << AmbientColor.z << std::endl;
    out << "Background=" << BackgroundColor.x << "," << BackgroundColor.y << "," << BackgroundColor.z << std::endl;
    
    out << "\n[Entities]" << std::endl;
    out << "Count=" << m_entities.size() << std::endl;
    
    for (size_t i = 0; i < m_entities.size(); ++i)
    {
        const auto& e = m_entities[i];
        out << "\n[Entity" << i << "]" << std::endl;
        out << "Name=" << e.name << std::endl;
        out << "Position=" << e.Transform.Position.x << "," << e.Transform.Position.y << "," << e.Transform.Position.z << std::endl;
        out << "Rotation=" << e.Transform.Rotation.x << "," << e.Transform.Rotation.y << "," << e.Transform.Rotation.z << std::endl;
        out << "Scale=" << e.Transform.Scale.x << "," << e.Transform.Scale.y << "," << e.Transform.Scale.z << std::endl;
        out << "MeshPath=" << e.MeshPath << std::endl;  // Save path instead of handle
    }
    
    out.close();
    std::cout << "Scene saved: " << path << std::endl;
    return true;
}

bool Scene::LoadFromFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        std::cerr << "Failed to load scene from: " << path << std::endl;
        return false;
    }
    
    ClearEntities();
    
    std::string line;
    std::string currentSection;
    Entity currentEntity;
    bool inEntity = false;
    
    auto parseVec3 = [](const std::string& str) -> Vec3 {
        Vec3 result{0, 0, 0};
        std::istringstream ss(str);
        char comma;
        ss >> result.x >> comma >> result.y >> comma >> result.z;
        return result;
    };
    
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '[')
        {
            if (inEntity)
            {
                AddEntity(currentEntity);
                currentEntity = Entity();
            }
            
            currentSection = line;
            inEntity = currentSection.find("[Entity") == 0;
            continue;
        }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        if (currentSection == "[Scene]")
        {
            if (key == "Name") m_name = value;
            else if (key == "Version") m_metadata.version = value;
            else if (key == "Author") m_metadata.author = value;
            else if (key == "Description") m_metadata.description = value;
        }
        else if (currentSection == "[Camera]")
        {
            if (key == "Position") m_camera.Position = parseVec3(value);
            else if (key == "Yaw") m_camera.Yaw = std::stof(value);
            else if (key == "Pitch") m_camera.Pitch = std::stof(value);
            else if (key == "FOV") m_camera.FOV = std::stof(value);
            else if (key == "Near") m_camera.Near = std::stof(value);
            else if (key == "Far") m_camera.Far = std::stof(value);
        }
        else if (currentSection == "[Light]")
        {
            if (key == "Direction") m_light.Direction = parseVec3(value);
            else if (key == "Color") m_light.Color = parseVec3(value);
            else if (key == "Intensity") m_light.Intensity = std::stof(value);
        }
        else if (currentSection == "[Environment]")
        {
            if (key == "Ambient") AmbientColor = parseVec3(value);
            else if (key == "Background") BackgroundColor = parseVec3(value);
        }
        else if (inEntity)
        {
            if (key == "Name") currentEntity.name = value;
            else if (key == "Position") currentEntity.Transform.Position = parseVec3(value);
            else if (key == "Rotation") currentEntity.Transform.Rotation = parseVec3(value);
            else if (key == "Scale") currentEntity.Transform.Scale = parseVec3(value);
            else if (key == "MeshPath") 
            {
                currentEntity.MeshPath = value;
                // Load mesh and get handle
                if (value == "[cube]")
                {
                    currentEntity.MeshHandle = MeshManager::Instance().GetSharedCubeHandle();
                }
                else if (!value.empty())
                {
                    currentEntity.MeshHandle = MeshManager::Instance().LoadMeshSync(value);
                }
            }
            // Legacy support: load by handle (deprecated)
            else if (key == "MeshHandle" && currentEntity.MeshPath.empty())
            {
                // Old format - try to load but it probably won't work
                currentEntity.MeshHandle = std::stoull(value);
            }
        }
    }
    
    if (inEntity)
    {
        AddEntity(currentEntity);
    }
    
    in.close();
    std::cout << "Scene loaded: " << path << " (" << m_entities.size() << " entities)" << std::endl;
    return true;
}

