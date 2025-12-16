#include <glad/glad.h>
#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../Managers/EntityManager.h"
#include "../Managers/RenderManager.h"
#include "../Cube.h"
#include "../Managers/InputManager.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>



UIManager& UIManager::Get()
{
    static UIManager instance;
    return instance;
}

UIManager::UIManager() = default;
UIManager::~UIManager() = default;

bool UIManager::Initialize(GLFWwindow* window, const char* glsl_version)
{
    if (!window) return false;
    m_Window = window;
    m_GlslVersion = glsl_version ? glsl_version : "#version 440";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(m_GlslVersion);

    m_Initialized = true;
    return true;
}

void UIManager::NewFrame()
{
    if (!m_Initialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Now that ImGui IO is updated, allow InputManager to apply pending decisions
    InputManager::Get().FrameUpdate();
}

void UIManager::Render()
{
    if (!m_Initialized) return;

    // Draw any windows we own
    DrawEntitiesWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::Shutdown()
{
    if (!m_Initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_Initialized = false;
}

void UIManager::DrawEntitiesWindow()
{
    ImGui::Begin("Entities");

    if (ImGui::Button("Create Entity"))
    {
        EntityManager::Get().CreateEntity();
    }

    ImGui::Separator();

    auto list = EntityManager::Get().GetAllEntities();
    // keep a stable selection pointer across frames
    int idx = 0;
    for (Entity* e : list)
    {
        char label[128];
        snprintf(label, sizeof(label), "%s##%d", e->name.c_str(), idx);
        if (ImGui::Selectable(label, m_SelectedEntity == e))
            m_SelectedEntity = e;
        idx++;
    }

    if (m_SelectedEntity)
    {
        ImGui::Separator();
        ImGui::Text("Editing: %s", m_SelectedEntity->name.c_str());

        if (ImGui::Button("Delete Entity"))
        {
            EntityManager::Get().DestroyEntity(m_SelectedEntity);
            m_SelectedEntity = nullptr;
            ImGui::End();
            return; // entity deleted - avoid further access this frame
        }

        // Name
        char nameBuf[256];
#ifdef _MSC_VER
        strcpy_s(nameBuf, sizeof(nameBuf), m_SelectedEntity->name.c_str());
#else
        std::strncpy(nameBuf, m_SelectedEntity->name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf)-1] = '\0';
#endif
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            m_SelectedEntity->name = std::string(nameBuf);

        // Model path
        char modelBuf[512];
#ifdef _MSC_VER
        strcpy_s(modelBuf, sizeof(modelBuf), m_SelectedEntity->modelPath.c_str());
#else
        std::strncpy(modelBuf, m_SelectedEntity->modelPath.c_str(), sizeof(modelBuf));
        modelBuf[sizeof(modelBuf)-1] = '\0';
#endif
        if (ImGui::InputText("Model Path", modelBuf, sizeof(modelBuf)))
            m_SelectedEntity->modelPath = std::string(modelBuf);

        // Texture path
        char texBuf[512];
#ifdef _MSC_VER
        strcpy_s(texBuf, sizeof(texBuf), m_SelectedEntity->texturePath.c_str());
#else
        std::strncpy(texBuf, m_SelectedEntity->texturePath.c_str(), sizeof(texBuf));
        texBuf[sizeof(texBuf)-1] = '\0';
#endif
        if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf)))
            m_SelectedEntity->texturePath = std::string(texBuf);

        // Position and rotation
        float pos[3] = { m_SelectedEntity->position.x, m_SelectedEntity->position.y, m_SelectedEntity->position.z };
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            m_SelectedEntity->position = glm::vec3(pos[0], pos[1], pos[2]);
            if (m_SelectedEntity->renderable)
            {
                Cube* c = dynamic_cast<Cube*>(m_SelectedEntity->renderable);
                if (c) c->SetModel(m_SelectedEntity->position, m_SelectedEntity->rotation);
            }
        }

        float rot[3] = { m_SelectedEntity->rotation.x, m_SelectedEntity->rotation.y, m_SelectedEntity->rotation.z };
        if (ImGui::DragFloat3("Rotation (deg)", rot, 1.0f))
        {
            m_SelectedEntity->rotation = glm::vec3(rot[0], rot[1], rot[2]);
            if (m_SelectedEntity->renderable)
            {
                Cube* c = dynamic_cast<Cube*>(m_SelectedEntity->renderable);
                if (c) c->SetModel(m_SelectedEntity->position, m_SelectedEntity->rotation);
            }
        }
    }

    ImGui::End();
}