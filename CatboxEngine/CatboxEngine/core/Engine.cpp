#include "Engine.h"
#include "Platform.h"
#include "MessageQueue.h"
#include "MemoryTracker.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "Time.h"
#include "../graphics/Mesh.h"
#include "../graphics/MeshManager.h"
#include "../resources/EntityManager.h"
#include "../resources/SceneManager.h"


void Engine::OnMouseMove(double xpos, double ypos)
{
    // forward mouse movement to camera (camera itself ignores movement when not captured)
    camera.OnMouseMove(xpos, ypos);
}

void Engine::OnDrop(const std::vector<std::string>& paths)
{
    if (paths.empty()) return;
    // handle first dropped path: try load model or texture depending on extension
    std::string p = paths[0];
    std::string ext;
    auto pos = p.find_last_of('.');
    if (pos != std::string::npos) ext = p.substr(pos+1);
    for (auto &c : ext) c = (char)tolower(c);

    if (ext == "obj" || ext == "gltf" || ext == "glb")
    {
        // Post model dropped message
        auto msg = std::make_shared<ModelDroppedMessage>(p);
        MessageQueue::Instance().Post(msg);
        
        MeshHandle h = MeshManager::Instance().LoadMeshSync(p);
        if (h != 0)
        {
            Entity e; e.name = std::string("Model: ") + p; e.MeshHandle = h;
            entityManager.AddEntity(e, useSharedCube);
        }
    }
    else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
    {
        // Post texture dropped message
        auto msg = std::make_shared<TextureDroppedMessage>(p);
        MessageQueue::Instance().Post(msg);
        
        // assign to selected entity if any
        if (selectedEntityIndex >= 0 && selectedEntityIndex < (int)entityManager.Size())
        {
            auto& ent = entityManager.GetAll()[selectedEntityIndex];
            if (ent.MeshHandle != 0)
            {
                Mesh* mptr = MeshManager::Instance().GetMesh(ent.MeshHandle);
                if (mptr)
                {
                    mptr->LoadTexture(p);
                    mptr->DiffuseTexturePath = p;
                    
                    // Post texture loaded message
                    auto loadMsg = std::make_shared<TextureLoadedMessage>(p, selectedEntityIndex);
                    MessageQueue::Instance().Post(loadMsg);
                }
            }
        }
    }
}

void Engine::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    camera.OnMouseButton(window, button, action, mods);
}

// mouse handling moved to Camera

Engine::~Engine()
{
    // Auto-save active scene before shutdown
    auto& sceneMgr = SceneManager::Instance();
    auto* activeScene = sceneMgr.GetActiveScene();
    if (activeScene)
    {
        std::cout << "Auto-saving active scene: " << activeScene->GetName() << std::endl;
        sceneMgr.SaveScene(sceneMgr.GetActiveSceneID(), "autosave.scene", entityManager);
    }
    
    Cleanup();
    
    // Check for memory leaks at shutdown
    std::cout << "\n=== Engine Shutdown ===" << std::endl;
    MemoryTracker::Instance().CheckForLeaks();
}

// Forward declare a static callback that will be set on the GLFW window
static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Engine* eng = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (!eng) return;
    // If ImGui is initialized and wants the mouse, don't forward
    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;
    }
    eng->OnMouseMove(xpos, ypos);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Engine* eng = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (!eng) return;
    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        bool overUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();
        if (io.WantCaptureMouse && overUI) return;
    }
    eng->OnMouseButton(window, button, action, mods);
}

void Engine::app()
{
    MEMORY_SCOPE("Engine::app");
    
    Initialize();
    
    std::cout << "Initial memory state:" << std::endl;
    MemoryTracker::Instance().PrintMemoryReport();
    
    while (!glfwWindowShouldClose(window))
    {
        Time::Update();

        Update(Time::DeltaTime());
        Render();
    }
    
    std::cout << "Final memory state:" << std::endl;
    MemoryTracker::Instance().PrintMemoryReport();
}

// mouse movement handled in Camera

void Engine::Update(float deltaTime)
{
    // handle window-level input
    if (glfwGetKey(GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(GetWindow(), true);

    // let camera handle inputs
    camera.Update(GetWindow(), deltaTime);
    glfwPollEvents();

    // UI frame and draw
    uiManager.NewFrame();
    uiManager.Draw(entityManager, spawnPosition, spawnScale, deltaTime, selectedEntityIndex, camera, useSharedCube);

    // poll mesh manager for completed async loads and invoke callbacks
    MeshManager::Instance().PollCompleted();
    
    // process message queue
    MessageQueue::Instance().ProcessMessages();

    // check clipboard for dropped path (used as a bridge by DropCallback)
    const char* clip = glfwGetClipboardString(GetWindow());
    static std::string lastClip;
    if (clip && std::string(clip) != lastClip)
    {
        lastClip = clip;
        std::vector<std::string> paths;
        paths.push_back(std::string(clip));
        OnDrop(paths);
    }

}

void Engine::Render()
{
    int display_w, display_h;
    glfwGetFramebufferSize(GetWindow(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(0.4f, 0.3f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render ImGui
    ImGui::Render();
    
    // Render scene
    myShader.Use();

    // Use camera for view/projection
    camera.Aspect = (float)display_w / (float)display_h;
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    glm::mat4 vp = proj * view;
    
    // Set shader uniforms that are constant for all entities
    myShader.setVec3("u_CameraPos", camera.Position.x, camera.Position.y, camera.Position.z);
    myShader.setVec3("u_LightDir", 0.5f, -0.7f, 1.0f);  // Directional light
    
    // Frustum culling statistics
    int totalEntities = 0;
    int culledEntities = 0;

    // Draw all spawned entities using cubeMesh
    for (const auto& e : entityManager.GetAll())
    {
        totalEntities++;
        
        // Get mesh for frustum culling
        Mesh* mesh = nullptr;
        if (e.MeshHandle != 0) 
            mesh = MeshManager::Instance().GetMesh(e.MeshHandle);
        
        // Transform bounding box to world space
        if (mesh)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(e.Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));
            
            // Transform bounding box corners
            glm::vec4 min4(mesh->BoundsMin.x, mesh->BoundsMin.y, mesh->BoundsMin.z, 1.0f);
            glm::vec4 max4(mesh->BoundsMax.x, mesh->BoundsMax.y, mesh->BoundsMax.z, 1.0f);
            
            glm::vec4 worldMin = model * min4;
            glm::vec4 worldMax = model * max4;
            
            Vec3 worldBoundsMin{worldMin.x, worldMin.y, worldMin.z};
            Vec3 worldBoundsMax{worldMax.x, worldMax.y, worldMax.z};
            
            // Frustum culling check
            if (!camera.IsBoxInFrustum(worldBoundsMin, worldBoundsMax))
            {
                culledEntities++;
                continue;  // Skip rendering this entity
            }
        }
        
        // Entity is visible, render it
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));

        // apply rotation from inspector (Euler degrees -> radians)
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));

        myShader.SetMat4("u_MVP", vp);
        myShader.SetMat4("transform", model);

        // mesh was already loaded during frustum culling check above
        if (mesh)
        {
            // Check if this is a multi-material mesh
            if (!mesh->SubMeshes.empty())
            {
                // Multi-material rendering: draw each submesh with its own material
                glBindVertexArray(mesh->VAO);
                
                for (const auto& sub : mesh->SubMeshes)
                {
                    // Set material properties for this submesh
                    myShader.setVec3("u_DiffuseColor", sub.DiffuseColor.x, sub.DiffuseColor.y, sub.DiffuseColor.z);
                    
                    // Check for entity texture overrides, otherwise use submesh textures
                    bool hasDiffuse = e.HasDiffuseTextureOverride ? true : sub.HasDiffuseTexture;
                    unsigned int diffuseTex = e.HasDiffuseTextureOverride ? e.DiffuseTexture : sub.DiffuseTexture;
                    
                    myShader.SetBool("u_HasDiffuseMap", hasDiffuse);
                    if (hasDiffuse)
                    {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, diffuseTex);
                        myShader.SetTexture("u_DiffuseMap", 0);
                    }
                    
                    // Normal map (entity override or submesh)
                    bool hasNormal = e.HasNormalTextureOverride ? true : sub.HasNormalTexture;
                    unsigned int normalTex = e.HasNormalTextureOverride ? e.NormalTexture : sub.NormalTexture;
                    
                    myShader.SetBool("u_HasNormalMap", hasNormal);
                    if (hasNormal)
                    {
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, normalTex);
                        myShader.SetTexture("u_NormalMap", 2);
                    }

                    // Specular map (entity override or submesh)
                    bool hasSpecular = e.HasSpecularTextureOverride ? true : sub.HasSpecularTexture;
                    unsigned int specularTex = e.HasSpecularTextureOverride ? e.SpecularTexture : sub.SpecularTexture;
                    
                    myShader.setVec3("u_SpecularColor", sub.SpecularColor.x, sub.SpecularColor.y, sub.SpecularColor.z);
                    myShader.setFloat("u_Shininess", e.Shininess);  // Use entity shininess
                    myShader.setFloat("u_Alpha", e.Alpha);  // Use entity alpha
                    
                    myShader.SetBool("u_HasSpecularMap", hasSpecular);
                    if (hasSpecular)
                    {
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, specularTex);
                        myShader.SetTexture("u_SpecularMap", 1);
                    }
                    
                    // Draw this submesh
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)sub.Indices.size(), GL_UNSIGNED_INT, 0);
                }
            }
            else
            {
                // Legacy single-material rendering with entity texture overrides
                myShader.setVec3("u_DiffuseColor", mesh->DiffuseColor.x, mesh->DiffuseColor.y, mesh->DiffuseColor.z);
                
                // Check for entity diffuse texture override
                bool hasDiffuse = e.HasDiffuseTextureOverride ? true : mesh->HasDiffuseTexture;
                unsigned int diffuseTex = e.HasDiffuseTextureOverride ? e.DiffuseTexture : mesh->DiffuseTexture;
                
                myShader.SetBool("u_HasDiffuseMap", hasDiffuse);
                if (hasDiffuse)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, diffuseTex);
                    myShader.SetTexture("u_DiffuseMap", 0);
                }
                
                // Check for entity normal texture override
                bool hasNormal = e.HasNormalTextureOverride ? true : mesh->HasNormalTexture;
                unsigned int normalTex = e.HasNormalTextureOverride ? e.NormalTexture : mesh->NormalTexture;
                
                myShader.SetBool("u_HasNormalMap", hasNormal);
                if (hasNormal)
                {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, normalTex);
                    myShader.SetTexture("u_NormalMap", 2);
                }

                // Check for entity specular texture override
                bool hasSpecular = e.HasSpecularTextureOverride ? true : mesh->HasSpecularTexture;
                unsigned int specularTex = e.HasSpecularTextureOverride ? e.SpecularTexture : mesh->SpecularTexture;
                
                myShader.setVec3("u_SpecularColor", mesh->SpecularColor.x, mesh->SpecularColor.y, mesh->SpecularColor.z);
                myShader.setFloat("u_Shininess", e.Shininess);  // Use entity shininess
                myShader.setFloat("u_Alpha", e.Alpha);  // Use entity alpha
                
                myShader.SetBool("u_HasSpecularMap", hasSpecular);
                if (hasSpecular)
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, specularTex);
                    myShader.SetTexture("u_SpecularMap", 1);
                }

                if (mesh->VAO != 0)
                    mesh->Draw();
            }
        }
    }
    
    // Optional: Print frustum culling statistics (debug only)
    // std::cout << "Frustum Culling: " << culledEntities << "/" << totalEntities 
    //           << " entities culled" << std::endl;

    
    
    uiManager.Render();

    glfwSwapBuffers(GetWindow());
}


int Engine::Initialize()
{
    // platform (GLFW window)
    if (!platform.Init((int)width, (int)height, name))
        return -1;

    window = platform.GetWindow();

    // Set user pointer so callbacks can access the Engine instance
    glfwSetWindowUserPointer(window, this);
    // install input callbacks
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // install drop callback for drag-and-drop support
    Platform::InstallDropCallback(window);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    glEnable(GL_DEPTH_TEST);

    // initialize the simple shader
    myShader.Initialize("./shaders/VertexShader.vert", "./shaders/FragmentShader.frag");

    // Initialize camera
    camera.Initialize({0,0,3}, {0,0,0}, {0,1,0}, 60.0f, width / height, 0.1f, 100.0f);

    // ImGui
    if (InitImGui() != 0)
        return -1;

    // Subscribe to messages
    SetupMessageSubscriptions();
    
    // Try to load autosave scene
    auto& sceneMgr = SceneManager::Instance();
    std::ifstream checkFile("autosave.scene");
    if (checkFile.good())
    {
        checkFile.close();
        std::cout << "Loading autosave scene..." << std::endl;
        SceneID id = sceneMgr.LoadScene("autosave.scene");
        if (id != 0)
        {
            sceneMgr.SetActiveScene(id, entityManager);
        }
    }
    else
    {
        // No autosave, create default scene
        std::cout << "No autosave found, creating default scene..." << std::endl;
        SceneID defaultScene = sceneMgr.CreateScene("Default Scene");
        sceneMgr.SetActiveScene(defaultScene, entityManager);
    }

    return 0;
}

void Engine::SetupMessageSubscriptions()
{
    // Subscribe to entity events
    MessageQueue::Instance().Subscribe(MessageType::EntityCreated, [](const Message& msg) {
        const auto& m = static_cast<const EntityCreatedMessage&>(msg);
        std::cout << "Entity created: " << m.entityName << " at index " << m.entityIndex << std::endl;
    });

    MessageQueue::Instance().Subscribe(MessageType::EntityDestroyed, [](const Message& msg) {
        const auto& m = static_cast<const EntityDestroyedMessage&>(msg);
        std::cout << "Entity destroyed: " << m.entityName << " at index " << m.entityIndex << std::endl;
    });

    // Subscribe to mesh events
    MessageQueue::Instance().Subscribe(MessageType::MeshLoaded, [](const Message& msg) {
        const auto& m = static_cast<const MeshLoadedMessage&>(msg);
        std::cout << "Mesh loaded: " << m.path << " (handle: " << m.handle << ")" << std::endl;
    });

    MessageQueue::Instance().Subscribe(MessageType::MeshLoadFailed, [](const Message& msg) {
        const auto& m = static_cast<const MeshLoadFailedMessage&>(msg);
        std::cerr << "Mesh load failed: " << m.path << " - " << m.error << std::endl;
    });
}

int Engine::InitGlfw()
{
    // removed: platform initialization is handled by Platform class
    return 0;
}

int Engine::InitGlad()
{
    // handled in Initialize
    return 0;
}

int Engine::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    //Setup platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440");

    imguiInitialized = true;

    return 0;
}

void Engine::Cleanup()
{
    if (imguiInitialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        imguiInitialized = false;
    }

    if (glfwInitialized)
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        glfwInitialized = false;
    }
}
