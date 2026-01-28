#include "EntityInspector.h"
#include "imgui.h"
#include "../../graphics/MeshManager.h"
#include "../../core/Platform.h"
#include <glad/glad.h>
#include <iostream>

// stb_image for texture loading (already defined in Mesh.cpp, just declare)
#include "../../Dependencies/stb_image.h"

// Helper to load texture from file
static unsigned int LoadTextureFromFile(const std::string& path)
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

void EntityInspector::Draw(Entity& entity)
{
    ImGui::Begin("Inspector");

    // Name (editable)
    char nameBuf[128];
    strncpy_s(nameBuf, entity.name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        entity.name = std::string(nameBuf);
    }

    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::InputFloat3("Position", &entity.Transform.Position.x);
    ImGui::InputFloat3("Rotation", &entity.Transform.Rotation.x);
    ImGui::InputFloat3("Scale", &entity.Transform.Scale.x);

    // Material Properties
    ImGui::Separator();
    ImGui::Text("Material Properties");
    ImGui::SliderFloat("Shininess", &entity.Shininess, 1.0f, 256.0f);
    ImGui::SliderFloat("Alpha", &entity.Alpha, 0.0f, 1.0f);

    // Textures Section
    ImGui::Separator();
    ImGui::Text("Textures");
    
    // Diffuse Texture
    ImGui::PushID("Diffuse");
    if (entity.HasDiffuseTextureOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Diffuse: Override Active");
        ImGui::Text("  %s", entity.DiffuseTexturePath.c_str());
        if (ImGui::Button("Remove Override"))
        {
            if (entity.DiffuseTexture != 0)
            {
                glDeleteTextures(1, &entity.DiffuseTexture);
                entity.DiffuseTexture = 0;
            }
            entity.HasDiffuseTextureOverride = false;
            entity.DiffuseTexturePath = "";
        }
    }
    else
    {
        ImGui::Text("Diffuse: (using mesh default)");
        if (ImGui::Button("Set Override"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, (int)sizeof(buf), 
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                std::string path(buf);
                unsigned int texID = LoadTextureFromFile(path);
                if (texID != 0)
                {
                    entity.DiffuseTexture = texID;
                    entity.DiffuseTexturePath = path;
                    entity.HasDiffuseTextureOverride = true;
                }
            }
        }
    }
    ImGui::PopID();
    
    ImGui::Spacing();
    
    // Specular Texture
    ImGui::PushID("Specular");
    if (entity.HasSpecularTextureOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Specular: Override Active");
        ImGui::Text("  %s", entity.SpecularTexturePath.c_str());
        if (ImGui::Button("Remove Override"))
        {
            if (entity.SpecularTexture != 0)
            {
                glDeleteTextures(1, &entity.SpecularTexture);
                entity.SpecularTexture = 0;
            }
            entity.HasSpecularTextureOverride = false;
            entity.SpecularTexturePath = "";
        }
    }
    else
    {
        ImGui::Text("Specular: (using mesh default)");
        if (ImGui::Button("Set Override"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, (int)sizeof(buf), 
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                std::string path(buf);
                unsigned int texID = LoadTextureFromFile(path);
                if (texID != 0)
                {
                    entity.SpecularTexture = texID;
                    entity.SpecularTexturePath = path;
                    entity.HasSpecularTextureOverride = true;
                }
            }
        }
    }
    ImGui::PopID();
    
    ImGui::Spacing();
    
    // Normal Texture
    ImGui::PushID("Normal");
    if (entity.HasNormalTextureOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Normal: Override Active");
        ImGui::Text("  %s", entity.NormalTexturePath.c_str());
        if (ImGui::Button("Remove Override"))
        {
            if (entity.NormalTexture != 0)
            {
                glDeleteTextures(1, &entity.NormalTexture);
                entity.NormalTexture = 0;
            }
            entity.HasNormalTextureOverride = false;
            entity.NormalTexturePath = "";
        }
    }
    else
    {
        ImGui::Text("Normal: (using mesh default)");
        if (ImGui::Button("Set Override"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, (int)sizeof(buf), 
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                std::string path(buf);
                unsigned int texID = LoadTextureFromFile(path);
                if (texID != 0)
                {
                    entity.NormalTexture = texID;
                    entity.NormalTexturePath = path;
                    entity.HasNormalTextureOverride = true;
                }
            }
        }
    }
    ImGui::PopID();

    // Morph Targets / Blend Shapes
    if (entity.MeshHandle != 0)
    {
        Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (mesh && !mesh->MorphTargets.empty())
        {
            ImGui::Separator();
            ImGui::Text("Morph Targets (%zu)", mesh->MorphTargets.size());
            
            for (size_t i = 0; i < mesh->MorphTargets.size(); ++i)
            {
                auto& target = mesh->MorphTargets[i];
                ImGui::PushID((int)i);
                
                if (ImGui::SliderFloat(target.Name.c_str(), &target.Weight, 0.0f, 1.0f))
                {
                    mesh->UpdateMorphTargets();
                }
                
                ImGui::PopID();
            }
        }
    }

    ImGui::End();
}
