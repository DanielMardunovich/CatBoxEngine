#include "Engine.h"
#include "Platform.h"
#include "MessageQueue.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "Time.h"
#include "../graphics/Mesh.h"
#include "../graphics/MeshManager.h"
#include "../resources/EntityManager.h"


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
    Cleanup();
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
    Initialize();
    
    while (!glfwWindowShouldClose(window))
    {
        Time::Update();

        Update(Time::DeltaTime());
        Render();
    }
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

    // Draw all spawned entities using cubeMesh
    for (const auto& e : entityManager.GetAll())
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));

        // apply rotation from inspector (Euler degrees -> radians)
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(e.Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));

        myShader.SetMat4("u_MVP", vp);
        myShader.SetMat4("transform", model);

        // get mesh by handle
        Mesh* mesh = nullptr;
        if (e.MeshHandle != 0) mesh = MeshManager::Instance().GetMesh(e.MeshHandle);

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
                    myShader.SetBool("u_HasDiffuseMap", sub.HasDiffuseTexture);
                    if (sub.HasDiffuseTexture)
                    {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, sub.DiffuseTexture);
                        myShader.SetTexture("u_DiffuseMap", 0);
                    }
                    
                    if (sub.HasNormalTexture)
                    {
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, sub.NormalTexture);
                        myShader.SetTexture("u_NormalMap", 2);
                        myShader.SetBool("u_HasNormalMap", true);
                    }
                    else myShader.SetBool("u_HasNormalMap", false);

                    myShader.setVec3("u_SpecularColor", sub.SpecularColor.x, sub.SpecularColor.y, sub.SpecularColor.z);
                    myShader.setFloat("u_Shininess", sub.Shininess);
                    myShader.setFloat("u_Alpha", sub.Alpha);
                    
                    if (sub.HasSpecularTexture)
                    {
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, sub.SpecularTexture);
                        myShader.SetTexture("u_SpecularMap", 1);
                        myShader.SetBool("u_HasSpecularMap", true);
                    }
                    else myShader.SetBool("u_HasSpecularMap", false);
                    
                    // Draw this submesh
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
                    glDrawElements(GL_TRIANGLES, (GLsizei)sub.Indices.size(), GL_UNSIGNED_INT, 0);
                }
            }
            else
            {
                // Legacy single-material rendering
                myShader.setVec3("u_DiffuseColor", mesh->DiffuseColor.x, mesh->DiffuseColor.y, mesh->DiffuseColor.z);
                myShader.SetBool("u_HasDiffuseMap", mesh->HasDiffuseTexture);
                if (mesh->HasDiffuseTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh->DiffuseTexture);
                    myShader.SetTexture("u_DiffuseMap", 0);
                }
                if (mesh->HasNormalTexture)
                {
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, mesh->NormalTexture);
                    myShader.SetTexture("u_NormalMap", 2);
                    myShader.SetBool("u_HasNormalMap", true);
                }
                else myShader.SetBool("u_HasNormalMap", false);

                myShader.setVec3("u_SpecularColor", mesh->SpecularColor.x, mesh->SpecularColor.y, mesh->SpecularColor.z);
                myShader.setFloat("u_Shininess", mesh->Shininess);
                myShader.setFloat("u_Alpha", mesh->Alpha);
                if (mesh->HasSpecularTexture)
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, mesh->SpecularTexture);
                    myShader.SetTexture("u_SpecularMap", 1);
                    myShader.SetBool("u_HasSpecularMap", true);
                }
                else myShader.SetBool("u_HasSpecularMap", false);

                if (mesh->VAO != 0)
                    mesh->Draw();
            }
        }
    }

    
    
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
