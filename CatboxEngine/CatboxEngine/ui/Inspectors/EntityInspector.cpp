#include "EntityInspector.h"
#include "imgui.h"
#include "../../graphics/MeshManager.h"
#include "../../core/Platform.h"
#include <glad/glad.h>
#include <iostream>
#include "../../Dependencies/stb_image.h"

namespace
{
    // Helper function to load texture from file
    unsigned int LoadTextureFromFile(const std::string& path)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return 0;
        }

        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Loaded texture: " << path << " (ID: " << tex << ")" << std::endl;
        return tex;
    }

    // Helper function to handle mesh loading
    bool LoadMeshForEntity(Entity& entity)
    {
        constexpr int BUFFER_SIZE = 1024;
        char buf[BUFFER_SIZE] = {0};
        
        if (Platform::OpenFileDialog(buf, BUFFER_SIZE, 
            "3D Models\0*.obj;*.gltf;*.glb\0OBJ Files\0*.obj\0GLTF Files\0*.gltf;*.glb\0All\0*.*\0"))
        {
            const std::string path(buf);
            MeshHandle newHandle = MeshManager::Instance().LoadMeshSync(path);
            if (newHandle != 0)
            {
                if (entity.MeshHandle != 0)
                {
                    MeshManager::Instance().Release(entity.MeshHandle);
                }
                entity.MeshHandle = newHandle;
                entity.MeshPath = path;
                std::cout << "Successfully loaded mesh: " << path << std::endl;
                return true;
            }
            else
            {
                std::cerr << "Failed to load mesh from path: " << path << std::endl;
            }
        }
        return false;
    }

    // Helper function to render texture override controls
    void DrawTextureOverride(const char* label, unsigned int& texture, std::string& texturePath, 
                            bool& hasOverride)
    {
        ImGui::PushID(label);
        
        if (hasOverride)
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s: Override Active", label);
            ImGui::Text("  %s", texturePath.c_str());
            if (ImGui::Button("Remove Override"))
            {
                if (texture != 0)
                {
                    glDeleteTextures(1, &texture);
                    texture = 0;
                }
                hasOverride = false;
                texturePath = "";
            }
        }
        else
        {
            ImGui::Text("%s: (using mesh default)", label);
            if (ImGui::Button("Set Override"))
            {
                constexpr int BUFFER_SIZE = 1024;
                char buf[BUFFER_SIZE] = {0};
                
                if (Platform::OpenFileDialog(buf, BUFFER_SIZE, 
                    "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
                {
                    const std::string path(buf);
                    const unsigned int texID = LoadTextureFromFile(path);
                    if (texID != 0)
                    {
                        texture = texID;
                        texturePath = path;
                        hasOverride = true;
                    }
                }
            }
        }
        
        ImGui::PopID();
    }
}

void EntityInspector::Draw(Entity& entity)
{
    ImGui::Begin("Entity Inspector");

    // Name (editable)
    constexpr int NAME_BUFFER_SIZE = 128;
    char nameBuf[NAME_BUFFER_SIZE];
    strncpy_s(nameBuf, entity.name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        entity.name = std::string(nameBuf);
    }

    // Transform section
    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::InputFloat3("Position", &entity.Transform.Position.x);
    ImGui::InputFloat3("Rotation", &entity.Transform.Rotation.x);
    ImGui::InputFloat3("Scale", &entity.Transform.Scale.x);

    // Mesh section
    ImGui::Separator();
    ImGui::Text("Mesh");
    
    if (entity.MeshHandle != 0)
    {
        Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (mesh)
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Loaded: %s", entity.MeshPath.c_str());
            ImGui::Text("Vertices: %zu", mesh->Vertices.size());
            
            if (ImGui::Button("Change Mesh"))
            {
                LoadMeshForEntity(entity);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Mesh"))
            {
                MeshManager::Instance().Release(entity.MeshHandle);
                entity.MeshHandle = 0;
                entity.MeshPath = "";
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Mesh loading...");
        }
    }
    else
    {
        ImGui::Text("No mesh assigned");
        if (ImGui::Button("Load Mesh"))
        {
            LoadMeshForEntity(entity);
        }
    }
    
    // Material properties section
    ImGui::Separator();
    ImGui::Text("Material Properties");
    ImGui::SliderFloat("Shininess", &entity.Shininess, 1.0f, 256.0f);
    ImGui::SliderFloat("Alpha", &entity.Alpha, 0.0f, 1.0f);

    // Textures section
    ImGui::Separator();
    ImGui::Text("Textures");
    
    DrawTextureOverride("Diffuse", entity.DiffuseTexture, entity.DiffuseTexturePath, 
                       entity.HasDiffuseTextureOverride);
    ImGui::Spacing();
    
    DrawTextureOverride("Specular", entity.SpecularTexture, entity.SpecularTexturePath, 
                       entity.HasSpecularTextureOverride);
    ImGui::Spacing();
    
    DrawTextureOverride("Normal", entity.NormalTexture, entity.NormalTexturePath, 
                       entity.HasNormalTextureOverride);

    ImGui::End();
}
