#include "Scene.h"
#include "EntityManager.h"
#include "../graphics/MeshManager.h"
#include "../graphics/Mesh.h"
#include "../graphics/LightManager.h"
#include "../gameplay/TerrainSystem.h"
#include "../core/MessageQueue.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <cfloat>
#include <algorithm>

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
    
    m_isLoaded = true;
    
    // Clear existing entities in manager
    entityManager.Clear();
    
    // Clear and reload lights into LightManager
    auto& lightMgr = LightManager::Instance();
    lightMgr.ClearLights();
    
    for (const auto& light : m_lights)
    {
        lightMgr.AddLight(light);
    }
    
    // Load all entities from this scene into the entity manager
    for (auto& entity : m_entities)
    {
        // Ensure mesh is loaded with correct handle
        if (!entity.MeshPath.empty() && entity.MeshPath != "[cube]" && entity.MeshPath != "[terrain]")
        {
            // Reload mesh to ensure it's in memory
            MeshHandle newHandle = MeshManager::Instance().LoadMeshSync(entity.MeshPath);
            if (newHandle != 0)
            {
                // Release old handle if different
                if (entity.MeshHandle != 0 && entity.MeshHandle != newHandle)
                {
                    MeshManager::Instance().Release(entity.MeshHandle);
                }
                entity.MeshHandle = newHandle;
                
                // Ensure bounds are calculated
                Mesh* mesh = MeshManager::Instance().GetMesh(newHandle);
                if (mesh)
                {
                    // Check if bounds are valid
                    bool validBounds = (mesh->BoundsMin.x != FLT_MAX) && (mesh->BoundsMax.x != -FLT_MAX);
                    if (!validBounds)
                    {
                        mesh->CalculateBounds();
                    }
                }
            }
            else
            {
                std::cerr << "  Failed to reload mesh: " << entity.MeshPath << std::endl;
            }
        }
        else if (entity.MeshPath == "[cube]")
        {
            // Use shared cube
            entity.MeshHandle = MeshManager::Instance().GetSharedCubeHandle();
        }
        else if (entity.MeshPath == "[terrain]")
        {
            // Terrain mesh is generated below after all entities are added
        }

        entityManager.AddEntity(entity, false); // false = don't save to scene (we already have them)
    }

    // Generate terrain meshes now that all entities are in the manager
    for (auto& e : entityManager.GetAll())
    {
        if (e.IsTerrain)
            TerrainSystem::GenerateTerrainMesh(e);
    }

    // Ensure new teleporter pairs spawned after loading don't reuse any loaded pair ID
    int maxPairID = -1;
    for (const auto& e : m_entities)
    {
        if (e.IsTeleporter && e.TeleporterPairID > maxPairID)
            maxPairID = e.TeleporterPairID;
    }
    if (maxPairID >= 0)
        entityManager.SyncTeleporterPairID(maxPairID);}

void Scene::OnUnload(EntityManager& entityManager)
{
    if (!m_isLoaded) return;
    
    // Capture current state before unloading
    CaptureFromEntityManager(entityManager);
    
    // Capture lights from LightManager
    auto& lightMgr = LightManager::Instance();
    m_lights = lightMgr.GetAllLights();
    
    // Clear entity manager and lights
    entityManager.Clear();
    lightMgr.ClearLights();
    
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
    
    // ALSO capture lights from LightManager
    auto& lightMgr = LightManager::Instance();
    m_lights = lightMgr.GetAllLights();
    
    // Update modified time
    auto now = std::chrono::system_clock::now();
    m_metadata.modifiedTime = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
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

void Scene::RemoveLight(size_t index)
{
    if (index < m_lights.size())
    {
        m_lights.erase(m_lights.begin() + index);
    }
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
    
    out << "\n[Environment]" << std::endl;
    out << "Ambient=" << AmbientColor.x << "," << AmbientColor.y << "," << AmbientColor.z << std::endl;
    out << "Background=" << BackgroundColor.x << "," << BackgroundColor.y << "," << BackgroundColor.z << std::endl;
    
    // Save lights
    out << "\n[Lights]" << std::endl;
    out << "Count=" << m_lights.size() << std::endl;
    
    for (size_t i = 0; i < m_lights.size(); ++i)
    {
        const auto& light = m_lights[i];
        out << "\n[Light" << i << "]" << std::endl;
        out << "Name=" << light.Name << std::endl;
        out << "Type=" << (int)light.Type << std::endl;
        out << "Position=" << light.Position.x << "," << light.Position.y << "," << light.Position.z << std::endl;
        out << "Direction=" << light.Direction.x << "," << light.Direction.y << "," << light.Direction.z << std::endl;
        out << "Color=" << light.Color.x << "," << light.Color.y << "," << light.Color.z << std::endl;
        out << "Intensity=" << light.Intensity << std::endl;
        out << "Constant=" << light.Constant << std::endl;
        out << "Linear=" << light.Linear << std::endl;
        out << "Quadratic=" << light.Quadratic << std::endl;
        out << "InnerCutoff=" << light.InnerCutoff << std::endl;
        out << "OuterCutoff=" << light.OuterCutoff << std::endl;
        out << "CastsShadows=" << light.CastsShadows << std::endl;
        out << "ShadowMapSize=" << light.ShadowMapSize << std::endl;
        out << "ShadowBias=" << light.ShadowBias << std::endl;
        out << "ShadowOrthoSize=" << light.ShadowOrthoSize << std::endl;
        out << "ShadowNearPlane=" << light.ShadowNearPlane << std::endl;
        out << "ShadowFarPlane=" << light.ShadowFarPlane << std::endl;
        out << "ShadowFOV=" << light.ShadowFOV << std::endl;
        out << "Enabled=" << light.Enabled << std::endl;
    }
    
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
        out << "MeshPath=" << e.MeshPath << std::endl;
        
        // Texture overrides
        if (e.HasDiffuseTextureOverride)
            out << "DiffuseTexturePath=" << e.DiffuseTexturePath << std::endl;
        if (e.HasSpecularTextureOverride)
            out << "SpecularTexturePath=" << e.SpecularTexturePath << std::endl;
        if (e.HasNormalTextureOverride)
            out << "NormalTexturePath=" << e.NormalTexturePath << std::endl;
        
        // Material properties
        out << "Shininess=" << e.Shininess << std::endl;
        out << "Alpha=" << e.Alpha << std::endl;

        // Gameplay tags (only write non-default values to keep files clean)
        if (e.IsSpawnPoint)
            out << "IsSpawnPoint=1" << std::endl;
        if (e.IsPlayer)
            out << "IsPlayer=1" << std::endl;
        if (e.IsTeleporter)
        {
            out << "IsTeleporter=1" << std::endl;
            out << "TeleporterPairID=" << e.TeleporterPairID << std::endl;
            out << "TeleporterRadius=" << e.TeleporterRadius << std::endl;
        }
        if (e.IsGoal)
        {
            out << "IsGoal=1" << std::endl;
            out << "GoalRadius=" << e.GoalRadius << std::endl;
        }
        if (!e.CollidesWithPlayer)
            out << "CollidesWithPlayer=0" << std::endl;
        if (e.IsEnemy)
        {
            out << "IsEnemy=1" << std::endl;
            out << "EnemySpeed=" << e.EnemySpeed << std::endl;
            out << "EnemyCollisionRadius=" << e.EnemyCollisionRadius << std::endl;
            out << "EnemyPatrolMode=" << static_cast<int>(e.EnemyPatrolMode) << std::endl;
            out << "EnemyWaypointCount=" << e.PatrolWaypoints.size() << std::endl;
            for (size_t w = 0; w < e.PatrolWaypoints.size(); ++w)
            {
                out << "EnemyWaypoint" << w << "="
                    << e.PatrolWaypoints[w].x << ","
                    << e.PatrolWaypoints[w].y << ","
                    << e.PatrolWaypoints[w].z << std::endl;
            }
        }
        if (e.IsTerrain)
        {
            out << "IsTerrain=1" << std::endl;
            out << "TerrainHeightmapPath=" << e.TerrainHeightmapPath << std::endl;
            out << "TerrainGridWidth=" << e.TerrainGridWidth << std::endl;
            out << "TerrainGridDepth=" << e.TerrainGridDepth << std::endl;
        }
    }   // end entity loop

    out.close();
    m_filePath = path;
    std::replace(m_filePath.begin(), m_filePath.end(), '\\', '/');
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

    m_filePath = path;
    std::replace(m_filePath.begin(), m_filePath.end(), '\\', '/');
    ClearEntities();
    m_lights.clear();  // Clear lights too
    
    std::string line;
    std::string currentSection;
    Entity currentEntity;
    Light currentLight;  // For loading lights
    bool inEntity = false;
    bool inLight = false;  // Track if we're in a light section
    
    auto parseVec3 = [](const std::string& str) -> Vec3 {
        Vec3 result{0, 0, 0};
        std::istringstream ss(str);
        char comma;
        ss >> result.x >> comma >> result.y >> comma >> result.z;
        return result;
    };
    
    auto parseBool = [](const std::string& str) -> bool {
        return (str == "1" || str == "true" || str == "True");
    };
    
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '[')
        {
            // Save previous entity or light
            if (inEntity)
            {
                AddEntity(currentEntity);
                currentEntity = Entity();
            }
            if (inLight)
            {
                AddLight(currentLight);
                currentLight = Light();
            }
            
            currentSection = line;
            inEntity = currentSection.find("[Entity") == 0;
            inLight = currentSection.find("[Light") == 0 && currentSection != "[Lights]";
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
        else if (inLight)  // Loading individual light
        {
            if (key == "Name") currentLight.Name = value;
            else if (key == "Type") currentLight.Type = (LightType)std::stoi(value);
            else if (key == "Position") currentLight.Position = parseVec3(value);
            else if (key == "Direction") currentLight.Direction = parseVec3(value);
            else if (key == "Color") currentLight.Color = parseVec3(value);
            else if (key == "Intensity") currentLight.Intensity = std::stof(value);
            else if (key == "Constant") currentLight.Constant = std::stof(value);
            else if (key == "Linear") currentLight.Linear = std::stof(value);
            else if (key == "Quadratic") currentLight.Quadratic = std::stof(value);
            else if (key == "InnerCutoff") currentLight.InnerCutoff = std::stof(value);
            else if (key == "OuterCutoff") currentLight.OuterCutoff = std::stof(value);
            else if (key == "CastsShadows") currentLight.CastsShadows = parseBool(value);
            else if (key == "ShadowMapSize") currentLight.ShadowMapSize = std::stoi(value);
            else if (key == "ShadowBias") currentLight.ShadowBias = std::stof(value);
            else if (key == "ShadowOrthoSize") currentLight.ShadowOrthoSize = std::stof(value);
            else if (key == "ShadowNearPlane") currentLight.ShadowNearPlane = std::stof(value);
            else if (key == "ShadowFarPlane") currentLight.ShadowFarPlane = std::stof(value);
            else if (key == "ShadowFOV") currentLight.ShadowFOV = std::stof(value);
            else if (key == "Enabled") currentLight.Enabled = parseBool(value);
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
                else if (value == "[terrain]")
                {
                    currentEntity.MeshPath = "[terrain]";
                    // Terrain mesh is generated in Scene::OnLoad after all parameters are read
                }
                else if (!value.empty())
                {
                    currentEntity.MeshHandle = MeshManager::Instance().LoadMeshSync(value);
                }
            }
            // Texture overrides
            else if (key == "DiffuseTexturePath")
            {
                currentEntity.DiffuseTexturePath = value;
                currentEntity.HasDiffuseTextureOverride = true;
                // TODO: Load texture
            }
            else if (key == "SpecularTexturePath")
            {
                currentEntity.SpecularTexturePath = value;
                currentEntity.HasSpecularTextureOverride = true;
                // TODO: Load texture
            }
            else if (key == "NormalTexturePath")
            {
                currentEntity.NormalTexturePath = value;
                currentEntity.HasNormalTextureOverride = true;
                // TODO: Load texture
            }
            // Material properties
            else if (key == "Shininess") currentEntity.Shininess = std::stof(value);
            else if (key == "Alpha") currentEntity.Alpha = std::stof(value);
            // Gameplay tags
            else if (key == "IsSpawnPoint") currentEntity.IsSpawnPoint = parseBool(value);
            else if (key == "IsPlayer")     currentEntity.IsPlayer     = parseBool(value);
            else if (key == "IsTeleporter") currentEntity.IsTeleporter = parseBool(value);
            else if (key == "TeleporterPairID") currentEntity.TeleporterPairID = std::stoi(value);
            else if (key == "TeleporterRadius") currentEntity.TeleporterRadius = std::stof(value);
            else if (key == "IsGoal") currentEntity.IsGoal = parseBool(value);
            else if (key == "GoalRadius") currentEntity.GoalRadius = std::stof(value);
            else if (key == "CollidesWithPlayer") currentEntity.CollidesWithPlayer = parseBool(value);
            else if (key == "IsEnemy") currentEntity.IsEnemy = parseBool(value);
            else if (key == "EnemySpeed") currentEntity.EnemySpeed = std::stof(value);
            else if (key == "EnemyCollisionRadius") currentEntity.EnemyCollisionRadius = std::stof(value);
            else if (key == "EnemyPatrolMode") currentEntity.EnemyPatrolMode = static_cast<PatrolMode>(std::stoi(value));
            else if (key == "EnemyWaypointCount") { /* count is informational; waypoints are loaded individually */ }
            else if (key.rfind("EnemyWaypoint", 0) == 0)
            {
                currentEntity.PatrolWaypoints.push_back(parseVec3(value));
            }
            else if (key == "IsTerrain") currentEntity.IsTerrain = parseBool(value);
            else if (key == "TerrainHeightmapPath") currentEntity.TerrainHeightmapPath = value;
            else if (key == "TerrainGridWidth")  currentEntity.TerrainGridWidth  = std::stoi(value);
            else if (key == "TerrainGridDepth")  currentEntity.TerrainGridDepth  = std::stoi(value);
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
    
    if (inLight)
    {
        AddLight(currentLight);
    }
    
    in.close();
    std::cout << "Scene loaded: " << path << " (" << m_entities.size() << " entities, " << m_lights.size() << " lights)" << std::endl;
    return true;
}

