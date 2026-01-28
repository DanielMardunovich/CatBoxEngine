#include "Mesh.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "glfw3.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cstring>

// GLTF support - tinygltf will handle stb_image internally
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "../Dependencies/tiny_gltf.h"

// Minimal OBJ loader supporting positions, normals and triangular faces
bool Mesh::LoadFromOBJ(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open())
        return false;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::vector<uint32_t> outIndices;
    std::vector<Vertex> outVerts;
    
    // Track material groups for multi-material support
    struct MaterialGroup
    {
        std::string materialName;
        std::vector<uint32_t> indices;
    };
    std::vector<MaterialGroup> materialGroups;
    MaterialGroup* currentGroup = nullptr;

    // key must include position, texcoord and normal indices to avoid merging vertices that differ by UVs
    std::unordered_map<std::string, uint32_t> cache;

    auto packKey = [](int p, int t, int n) {
        return std::to_string(p) + ":" + std::to_string(t) + ":" + std::to_string(n);
    };

    std::string line;
    // directory of OBJ file for resolving relative MTL/texture paths
    std::string objDir;
    size_t objPos = path.find_last_of("/\\");
    if (objPos != std::string::npos) objDir = path.substr(0, objPos+1);
    std::string currentMtl;
    std::unordered_map<std::string, Vec3> mtlColors;
    std::unordered_map<std::string, Vec3> mtlSpecular;
    std::unordered_map<std::string, float> mtlShininess;
    std::unordered_map<std::string, float> mtlAlpha;
    std::unordered_map<std::string, std::string> mtlMaps;
    std::unordered_map<std::string, std::string> mtlSpecularMaps;
    std::unordered_map<std::string, std::string> mtlNormalMaps;
    while (std::getline(in, line))
    {
        if (line.size() < 2) continue;
        std::istringstream ss(line);
        std::string prefix;
        ss >> prefix;
        if (prefix == "mtllib")
        {
            std::string mtlFile;
            ss >> mtlFile;
            // attempt to parse mtl (try given path first, then relative to OBJ)
            std::ifstream mtlin(mtlFile);
            if (!mtlin.is_open() && !objDir.empty())
            {
                std::string fullMtlPath = objDir + mtlFile;
                mtlin.open(fullMtlPath);
                if (mtlin.is_open())
                {
                    std::cout << "Loaded MTL file: " << fullMtlPath << std::endl;
                }
            }
            else if (mtlin.is_open())
            {
                std::cout << "Loaded MTL file: " << mtlFile << std::endl;
            }
            
            if (mtlin.is_open())
            {
                std::string mline;
                std::string cur;
                while (std::getline(mtlin, mline))
                {
                    std::istringstream ms(mline);
                    std::string mprefix;
                    ms >> mprefix;
                    if (mprefix == "newmtl") { ms >> cur; }
                    else if (mprefix == "Kd") { float r,g,b; ms >> r >> g >> b; mtlColors[cur] = {r,g,b}; }
                    else if (mprefix == "Ks") { float r,g,b; ms >> r >> g >> b; mtlSpecular[cur] = {r,g,b}; }
                    else if (mprefix == "Ns") { float ns; ms >> ns; mtlShininess[cur] = ns; }
                    else if (mprefix == "d") { float d; ms >> d; mtlAlpha[cur] = d; }
                    else if (mprefix == "map_Kd") { std::string tex; ms >> tex; if (!tex.empty()) mtlMaps[cur] = tex; }
                    else if (mprefix == "map_Ks") { std::string tex; ms >> tex; if (!tex.empty()) mtlSpecularMaps[cur] = tex; }
                    else if (mprefix == "map_Bump" || mprefix == "bump" || mprefix == "map_bump" || mprefix == "norm") 
                    { 
                        std::string tex; 
                        ms >> tex; 
                        if (!tex.empty()) mtlNormalMaps[cur] = tex;
                    }
                }
                std::cout << "MTL parsing complete. Found " << mtlMaps.size() << " diffuse maps, " 
                          << mtlSpecularMaps.size() << " specular maps, " 
                          << mtlNormalMaps.size() << " normal maps" << std::endl;
            }
            else
            {
                std::cerr << "Failed to open MTL file: " << mtlFile << std::endl;
            }
            continue;
        }
        if (prefix == "usemtl")
        {
            ss >> currentMtl;
            // Start a new material group
            materialGroups.push_back({currentMtl, {}});
            currentGroup = &materialGroups.back();
            continue;
        }
        if (prefix == "v")
        {
            float x, y, z; ss >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }
        else if (prefix == "vt")
        {
            float u, v; ss >> u >> v;
            texcoords.emplace_back(u, v);
        }
        else if (prefix == "vn")
        {
            float x, y, z; ss >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }
        else if (prefix == "f")
        {
            // supports polygons with arbitrary vertex counts; triangulate as a fan
            std::vector<std::string> tokens;
            std::string tok;
            while (ss >> tok)
            {
                tokens.push_back(tok);
            }
            if (tokens.size() < 3) continue;

            auto processToken = [&](const std::string& token) -> uint32_t {
                int vi=0, ni=-1, ti=-1;
                size_t p1 = token.find('/');
                if (p1==std::string::npos)
                {
                    int raw = std::stoi(token);
                    if (raw < 0) vi = (int)positions.size() + raw;
                    else vi = raw - 1;
                }
                else
                {
                    std::string viStr = token.substr(0, p1);
                    if (!viStr.empty())
                    {
                        int raw = std::stoi(viStr);
                        vi = raw < 0 ? (int)positions.size() + raw : raw - 1;
                    }
                    size_t p2 = token.find('/', p1+1);
                    if (p2!=std::string::npos)
                    {
                        std::string vtStr = token.substr(p1+1, p2 - (p1+1));
                        if (!vtStr.empty())
                        {
                            int rawt = std::stoi(vtStr);
                            ti = rawt < 0 ? (int)texcoords.size() + rawt : rawt - 1;
                        }
                        std::string vnStr = token.substr(p2+1);
                        if (!vnStr.empty())
                        {
                            int rawn = std::stoi(vnStr);
                            ni = rawn < 0 ? (int)normals.size() + rawn : rawn - 1;
                        }
                    }
                    else
                    {
                        std::string vtStr = token.substr(p1+1);
                        if (!vtStr.empty())
                        {
                            int rawt = std::stoi(vtStr);
                            ti = rawt < 0 ? (int)texcoords.size() + rawt : rawt - 1;
                        }
                    }
                }

                std::string key = packKey(vi, ti, ni);
                auto it = cache.find(key);
                if (it != cache.end()) return it->second;

                Vertex v{};
                glm::vec3 p = positions[vi];
                v.Position = {p.x, p.y, p.z};
                if (ni >=0 && ni < (int)normals.size()) { glm::vec3 n = normals[ni]; v.Normal = {n.x, n.y, n.z}; }
                else v.Normal = {0,0,0};
                if (ti >=0 && ti < (int)texcoords.size()) { glm::vec2 uv = texcoords[ti]; v.UV = {uv.x, uv.y, 0}; }
                else v.UV = {0,0,0};

                uint32_t newIndex = (uint32_t)outVerts.size();
                outVerts.push_back(v);
                cache[key] = newIndex;
                return newIndex;
            };

            // triangulate fan: (0,1,2), (0,2,3), ...
            for (size_t k = 1; k + 1 < tokens.size(); ++k)
            {
                uint32_t i0 = processToken(tokens[0]);
                uint32_t i1 = processToken(tokens[k]);
                uint32_t i2 = processToken(tokens[k+1]);
                
                // Add to current material group if exists, otherwise to legacy indices
                if (currentGroup)
                {
                    currentGroup->indices.push_back(i0);
                    currentGroup->indices.push_back(i1);
                    currentGroup->indices.push_back(i2);
                }
                else
                {
                    outIndices.push_back(i0);
                    outIndices.push_back(i1);
                    outIndices.push_back(i2);
                }
            }
        }
    }

    if (outVerts.empty() || outIndices.empty())
        return false;

    Vertices = std::move(outVerts);
    Indices = std::move(outIndices);

    // compute per-vertex normals if model did not provide them
    bool haveNormals = false;
    for (const auto &v : Vertices) if (!(v.Normal.x == 0.0f && v.Normal.y == 0.0f && v.Normal.z == 0.0f)) { haveNormals = true; break; }
    if (!haveNormals)
    {
        std::vector<glm::vec3> normAccum(Vertices.size(), glm::vec3(0.0f));
        for (size_t i = 0; i + 2 < Indices.size(); i += 3)
        {
            uint32_t i0 = Indices[i];
            uint32_t i1 = Indices[i+1];
            uint32_t i2 = Indices[i+2];

            glm::vec3 p0(Vertices[i0].Position.x, Vertices[i0].Position.y, Vertices[i0].Position.z);
            glm::vec3 p1(Vertices[i1].Position.x, Vertices[i1].Position.y, Vertices[i1].Position.z);
            glm::vec3 p2(Vertices[i2].Position.x, Vertices[i2].Position.y, Vertices[i2].Position.z);

            glm::vec3 faceN = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            normAccum[i0] += faceN;
            normAccum[i1] += faceN;
            normAccum[i2] += faceN;
        }
        for (size_t i = 0; i < Vertices.size(); ++i)
        {
            glm::vec3 n = glm::normalize(normAccum[i]);
            if (glm::length(n) == 0.0f) n = glm::vec3(0.0f, 1.0f, 0.0f);
            Vertices[i].Normal = { n.x, n.y, n.z };
        }
    }

    // compute tangents if we have UVs
    std::vector<glm::vec3> tanAccum(Vertices.size(), glm::vec3(0.0f));
    bool haveUV = false;
    for (const auto &v : Vertices) if (v.UV.x != 0.0f || v.UV.y != 0.0f) { haveUV = true; break; }
    if (haveUV)
    {
        for (size_t i = 0; i + 2 < Indices.size(); i += 3)
        {
            uint32_t i0 = Indices[i];
            uint32_t i1 = Indices[i+1];
            uint32_t i2 = Indices[i+2];

            glm::vec3 p0(Vertices[i0].Position.x, Vertices[i0].Position.y, Vertices[i0].Position.z);
            glm::vec3 p1(Vertices[i1].Position.x, Vertices[i1].Position.y, Vertices[i1].Position.z);
            glm::vec3 p2(Vertices[i2].Position.x, Vertices[i2].Position.y, Vertices[i2].Position.z);

            glm::vec2 uv0(Vertices[i0].UV.x, Vertices[i0].UV.y);
            glm::vec2 uv1(Vertices[i1].UV.x, Vertices[i1].UV.y);
            glm::vec2 uv2(Vertices[i2].UV.x, Vertices[i2].UV.y);

            glm::vec3 dp1 = p1 - p0;
            glm::vec3 dp2 = p2 - p0;
            glm::vec2 duv1 = uv1 - uv0;
            glm::vec2 duv2 = uv2 - uv0;

            float r = (duv1.x * duv2.y - duv2.x * duv1.y);
            if (r == 0.0f) continue;
            r = 1.0f / r;

            glm::vec3 tangent = (dp1 * duv2.y - dp2 * duv1.y) * r;

            tanAccum[i0] += tangent;
            tanAccum[i1] += tangent;
            tanAccum[i2] += tangent;
        }

        for (size_t i = 0; i < Vertices.size(); ++i)
        {
            glm::vec3 t = glm::normalize(tanAccum[i]);
            Vertices[i].Tangent = { t.x, t.y, t.z };
        }
    }

    // apply material color if found (try currentMtl first, then first available)
    std::string materialToUse = currentMtl;
    if (materialToUse.empty() && !mtlColors.empty())
    {
        materialToUse = mtlColors.begin()->first;
    }
    
    // Create SubMeshes for multi-material models
    if (!materialGroups.empty())
    {
        std::cout << "Creating " << materialGroups.size() << " submeshes for multi-material model" << std::endl;
        
        for (auto& group : materialGroups)
        {
            if (group.indices.empty()) continue;
            
            SubMesh sub;
            sub.MaterialName = group.materialName;
            sub.Indices = std::move(group.indices);
            sub.BaseVertex = 0;
            
            // Apply material properties
            auto itc = mtlColors.find(group.materialName);
            if (itc != mtlColors.end()) sub.DiffuseColor = itc->second;
            
            auto its = mtlSpecular.find(group.materialName);
            if (its != mtlSpecular.end()) sub.SpecularColor = its->second;
            
            auto itn = mtlShininess.find(group.materialName);
            if (itn != mtlShininess.end()) sub.Shininess = itn->second;
            
            auto itd = mtlAlpha.find(group.materialName);
            if (itd != mtlAlpha.end()) sub.Alpha = itd->second;
            
            // Load diffuse texture
            auto itm = mtlMaps.find(group.materialName);
            if (itm != mtlMaps.end())
            {
                std::string texPath = itm->second;
                std::string full = texPath;
                if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
                
                int width, height, channels;
                unsigned char* data = stbi_load(full.c_str(), &width, &height, &channels, 4);
                if (data)
                {
                    glGenTextures(1, &sub.DiffuseTexture);
                    glBindTexture(GL_TEXTURE_2D, sub.DiffuseTexture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    
                    sub.HasDiffuseTexture = true;
                    sub.DiffuseTexturePath = full;
                    std::cout << "  SubMesh '" << group.materialName << "' loaded diffuse: " << full << std::endl;
                }
                else
                {
                    std::cerr << "  SubMesh '" << group.materialName << "' failed to load diffuse: " << full << std::endl;
                }
            }
            
            // Load specular texture
            auto itsm = mtlSpecularMaps.find(group.materialName);
            if (itsm != mtlSpecularMaps.end())
            {
                std::string texPath = itsm->second;
                std::string full = texPath;
                if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
                
                int width, height, channels;
                unsigned char* data = stbi_load(full.c_str(), &width, &height, &channels, 4);
                if (data)
                {
                    glGenTextures(1, &sub.SpecularTexture);
                    glBindTexture(GL_TEXTURE_2D, sub.SpecularTexture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    
                    sub.HasSpecularTexture = true;
                    sub.SpecularTexturePath = full;
                    std::cout << "  SubMesh '" << group.materialName << "' loaded specular: " << full << std::endl;
                }
            }
            
            // Load normal map
            auto itnm = mtlNormalMaps.find(group.materialName);
            if (itnm != mtlNormalMaps.end())
            {
                std::string texPath = itnm->second;
                std::string full = texPath;
                if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
                
                int width, height, channels;
                unsigned char* data = stbi_load(full.c_str(), &width, &height, &channels, 4);
                if (data)
                {
                    glGenTextures(1, &sub.NormalTexture);
                    glBindTexture(GL_TEXTURE_2D, sub.NormalTexture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                    
                    sub.HasNormalTexture = true;
                    sub.NormalTexturePath = full;
                    std::cout << "  SubMesh '" << group.materialName << "' loaded normal: " << full << std::endl;
                }
            }
            
            SubMeshes.push_back(std::move(sub));
        }
    }
    // Fallback: single material for legacy models
    else if (!materialToUse.empty())
    {
        auto itc = mtlColors.find(materialToUse);
        if (itc != mtlColors.end()) DiffuseColor = { itc->second.x, itc->second.y, itc->second.z };

        auto its = mtlSpecular.find(materialToUse);
        if (its != mtlSpecular.end()) SpecularColor = its->second;

        auto itn = mtlShininess.find(materialToUse);
        if (itn != mtlShininess.end()) Shininess = itn->second;

        auto itd = mtlAlpha.find(materialToUse);
        if (itd != mtlAlpha.end()) Alpha = itd->second;

        // Load diffuse texture
        auto itm = mtlMaps.find(materialToUse);
        if (itm != mtlMaps.end())
        {
            std::string texPath = itm->second;
            std::string full = texPath;
            if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
            if (LoadTexture(full))
            {
                DiffuseTexturePath = full;
                std::cout << "Loaded diffuse texture: " << full << std::endl;
            }
            else
            {
                std::cerr << "Failed to load diffuse texture: " << full << std::endl;
            }
        }
        
        // Load specular texture
        auto itsm = mtlSpecularMaps.find(materialToUse);
        if (itsm != mtlSpecularMaps.end())
        {
            std::string texPath = itsm->second;
            std::string full = texPath;
            if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
            if (LoadSpecularTexture(full))
            {
                SpecularTexturePath = full;
                std::cout << "Loaded specular texture: " << full << std::endl;
            }
            else
            {
                std::cerr << "Failed to load specular texture: " << full << std::endl;
            }
        }
        
        // Load normal map
        auto itnm = mtlNormalMaps.find(materialToUse);
        if (itnm != mtlNormalMaps.end())
        {
            std::string texPath = itnm->second;
            std::string full = texPath;
            if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
            if (LoadNormalTexture(full))
            {
                NormalTexturePath = full;
                std::cout << "Loaded normal map: " << full << std::endl;
            }
            else
            {
                std::cerr << "Failed to load normal map: " << full << std::endl;
            }
        }
    }

    // Upload to GPU
    Upload();

    return true;
}

bool Mesh::LoadFromGLTF(const std::string& path)
{
    // Note: You need to download tiny_gltf.h and place it in Dependencies/
    // See docs/GLTF_Setup.md for instructions
    #ifdef TINYGLTF_IMPLEMENTATION
    
    // Helper function to load texture from GLTF image (handles both embedded and external)
    auto LoadGLTFTexture = [&](const tinygltf::Model& model, int texIndex, const std::string& dir) -> unsigned int {
        if (texIndex < 0 || texIndex >= (int)model.textures.size())
        {
            std::cerr << "    ? Invalid texture index: " << texIndex << std::endl;
            return 0;
        }
        
        const tinygltf::Texture& tex = model.textures[texIndex];
        if (tex.source < 0 || tex.source >= (int)model.images.size())
        {
            std::cerr << "    ? Invalid image source: " << tex.source << std::endl;
            return 0;
        }
        
        const tinygltf::Image& img = model.images[tex.source];
        unsigned char* data = nullptr;
        int width = 0, height = 0, channels = 0;
        
        std::cout << "    Image info: uri='" << img.uri << "', embedded_size=" << img.image.size() 
                  << ", component=" << img.component << ", pixel_type=" << img.pixel_type << std::endl;
        
        // Check if embedded (GLB) or external file
        if (img.image.size() > 0)
        {
            // Embedded image data
            std::cout << "    Loading embedded texture (size=" << img.image.size() << " bytes)..." << std::endl;
            data = stbi_load_from_memory(img.image.data(), (int)img.image.size(), &width, &height, &channels, 4);
            
            // If embedded load failed but we have a URI, try external as fallback
            if (!data && !img.uri.empty())
            {
                std::cerr << "    ? Embedded decode failed, trying external file as fallback..." << std::endl;
                std::string texPath = dir + img.uri;
                std::cout << "      Trying: " << texPath << std::endl;
                data = stbi_load(texPath.c_str(), &width, &height, &channels, 4);
            }
        }
        else if (!img.uri.empty())
        {
            // External file
            std::string texPath = dir + img.uri;
            std::cout << "    Loading external texture:" << std::endl;
            std::cout << "      URI: " << img.uri << std::endl;
            std::cout << "      Dir: " << dir << std::endl;
            std::cout << "      Full path: " << texPath << std::endl;
            data = stbi_load(texPath.c_str(), &width, &height, &channels, 4);
            
            if (!data)
            {
                std::cerr << "    ? Failed to load from: " << texPath << std::endl;
                std::cerr << "      stbi_failure_reason: " << stbi_failure_reason() << std::endl;
            }
        }
        else
        {
            std::cerr << "    ? Image has no data and no URI!" << std::endl;
            return 0;
        }
        
        if (!data)
        {
            std::cerr << "    ? Failed to decode image data" << std::endl;
            return 0;
        }
        
        // Create OpenGL texture
        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        
        std::cout << "    ? Texture loaded successfully (ID=" << texID << ", " << width << "x" << height << ")" << std::endl;
        return texID;
    };
    
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    // Determine if binary or ASCII format
    bool isBinary = (path.find(".glb") != std::string::npos);
    bool ret = isBinary ? 
        loader.LoadBinaryFromFile(&model, &err, &warn, path) :
        loader.LoadASCIIFromFile(&model, &err, &warn, path);
    
    if (!warn.empty()) std::cout << "GLTF Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "GLTF Error: " << err << std::endl;
    if (!ret) return false;
    
    std::cout << "Loading GLTF: " << path << std::endl;
    std::cout << "  Meshes: " << model.meshes.size() << std::endl;
    std::cout << "  Materials: " << model.materials.size() << std::endl;
    std::cout << "  Textures: " << model.textures.size() << std::endl;
    std::cout << "  Images: " << model.images.size() << std::endl;
    
    // Get directory for texture loading
    std::string dir;
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) dir = path.substr(0, pos + 1);
    
    std::vector<Vertex> allVertices;
    std::vector<SubMesh> allSubMeshes;
    
    // Process all meshes in the GLTF file
    for (size_t meshIdx = 0; meshIdx < model.meshes.size(); ++meshIdx)
    {
        const tinygltf::Mesh& gltfMesh = model.meshes[meshIdx];
        std::cout << "  Processing mesh '" << gltfMesh.name << "' with " << gltfMesh.primitives.size() << " primitives" << std::endl;
        
        // Each primitive in GLTF is like a submesh
        for (size_t primIdx = 0; primIdx < gltfMesh.primitives.size(); ++primIdx)
        {
            const tinygltf::Primitive& primitive = gltfMesh.primitives[primIdx];
            
            SubMesh sub;
            sub.BaseVertex = (uint32_t)allVertices.size();
            sub.MaterialName = (primitive.material >= 0) ? 
                model.materials[primitive.material].name : "default";
            
            std::cout << "    Primitive " << primIdx << " - Material: " << sub.MaterialName << std::endl;
            
            // Load vertex positions
            std::vector<glm::vec3> positions;
            if (primitive.attributes.count("POSITION"))
            {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                
                const float* data = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    positions.push_back(glm::vec3(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]));
                }
            }
            
            // Load normals
            std::vector<glm::vec3> normals;
            if (primitive.attributes.count("NORMAL"))
            {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                
                const float* data = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    normals.push_back(glm::vec3(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]));
                }
            }
            
            // Load UVs
            std::vector<glm::vec2> uvs;
            if (primitive.attributes.count("TEXCOORD_0"))
            {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                
                const float* data = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    uvs.push_back(glm::vec2(data[i * 2], data[i * 2 + 1]));
                }
            }
            
            // Load tangents
            std::vector<glm::vec3> tangents;
            if (primitive.attributes.count("TANGENT"))
            {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("TANGENT")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                
                const float* data = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    tangents.push_back(glm::vec3(data[i * 4], data[i * 4 + 1], data[i * 4 + 2]));
                }
            }
            
            // Build vertices
            size_t vertexCount = positions.size();
            for (size_t i = 0; i < vertexCount; ++i)
            {
                Vertex v;
                v.Position = {positions[i].x, positions[i].y, positions[i].z};
                v.Normal = (i < normals.size()) ? 
                    Vec3{normals[i].x, normals[i].y, normals[i].z} : Vec3{0, 1, 0};
                v.UV = (i < uvs.size()) ? 
                    Vec3{uvs[i].x, uvs[i].y, 0} : Vec3{0, 0, 0};
                v.Tangent = (i < tangents.size()) ? 
                    Vec3{tangents[i].x, tangents[i].y, tangents[i].z} : Vec3{0, 0, 0};
                
                allVertices.push_back(v);
            }
            
            // Load indices
            if (primitive.indices >= 0)
            {
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                
                const uint8_t* data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    uint32_t index = 0;
                    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    {
                        index = reinterpret_cast<const uint16_t*>(data)[i];
                    }
                    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    {
                        index = reinterpret_cast<const uint32_t*>(data)[i];
                    }
                    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    {
                        index = data[i];
                    }
                    sub.Indices.push_back(index + sub.BaseVertex);
                }
            }
            
            // Load material
            if (primitive.material >= 0)
            {
                const tinygltf::Material& mat = model.materials[primitive.material];
                
                std::cout << "    Material '" << mat.name << "' properties:" << std::endl;
                std::cout << "      values keys: ";
                for (auto& kv : mat.values) std::cout << kv.first << " ";
                std::cout << std::endl;
                std::cout << "      additionalValues keys: ";
                for (auto& kv : mat.additionalValues) std::cout << kv.first << " ";
                std::cout << std::endl;
                
                // PBR metallic-roughness
                if (mat.values.count("baseColorFactor"))
                {
                    auto& factor = mat.values.at("baseColorFactor");
                    if (factor.number_array.size() >= 3)
                    {
                        sub.DiffuseColor = Vec3{
                            (float)factor.number_array[0],
                            (float)factor.number_array[1],
                            (float)factor.number_array[2]
                        };
                        std::cout << "      baseColorFactor: " << sub.DiffuseColor.x << ", " 
                                  << sub.DiffuseColor.y << ", " << sub.DiffuseColor.z << std::endl;
                    }
                }
                
                // Base color texture (diffuse) - proper GLTF 2.0 parsing
                int baseColorTexIndex = -1;
                if (mat.values.count("baseColorTexture"))
                {
                    // baseColorTexture is a JSON object with "index" property
                    auto& baseColorTex = mat.values.at("baseColorTexture");
                    if (baseColorTex.has_number_value)
                    {
                        baseColorTexIndex = (int)baseColorTex.number_value;
                    }
                    else if (!baseColorTex.json_double_value.empty())
                    {
                        // Try to get "index" from json_double_value map
                        if (baseColorTex.json_double_value.count("index"))
                        {
                            baseColorTexIndex = (int)baseColorTex.json_double_value.at("index");
                        }
                    }
                }
                
                if (baseColorTexIndex >= 0)
                {
                    std::cout << "    Loading baseColor texture (index=" << baseColorTexIndex << ")..." << std::endl;
                    
                    sub.DiffuseTexture = LoadGLTFTexture(model, baseColorTexIndex, dir);
                    if (sub.DiffuseTexture != 0)
                    {
                        sub.HasDiffuseTexture = true;
                        sub.DiffuseTexturePath = (baseColorTexIndex < (int)model.textures.size() && 
                                                  model.textures[baseColorTexIndex].source < (int)model.images.size()) ?
                            model.images[model.textures[baseColorTexIndex].source].uri : "[embedded]";
                    }
                }
                else
                {
                    std::cout << "    No baseColorTexture found in material" << std::endl;
                    
                    // FALLBACK: Try emissive texture if baseColor is missing
                    int emissiveTexIndex = -1;
                    if (mat.additionalValues.count("emissiveTexture"))
                    {
                        auto& emissiveTex = mat.additionalValues.at("emissiveTexture");
                        if (emissiveTex.has_number_value)
                        {
                            emissiveTexIndex = (int)emissiveTex.number_value;
                        }
                        else if (!emissiveTex.json_double_value.empty())
                        {
                            if (emissiveTex.json_double_value.count("index"))
                            {
                                emissiveTexIndex = (int)emissiveTex.json_double_value.at("index");
                            }
                        }
                    }
                    
                    if (emissiveTexIndex >= 0)
                    {
                        std::cout << "    ? Using emissiveTexture as diffuse fallback (index=" << emissiveTexIndex << ")..." << std::endl;
                        sub.DiffuseTexture = LoadGLTFTexture(model, emissiveTexIndex, dir);
                        if (sub.DiffuseTexture != 0)
                        {
                            sub.HasDiffuseTexture = true;
                            sub.DiffuseTexturePath = (emissiveTexIndex < (int)model.textures.size() && 
                                                      model.textures[emissiveTexIndex].source < (int)model.images.size()) ?
                                model.images[model.textures[emissiveTexIndex].source].uri : "[embedded]";
                            // Set a white color so texture shows properly
                            sub.DiffuseColor = Vec3{1.0f, 1.0f, 1.0f};
                        }
                    }
                    else if (sub.DiffuseColor.x == 0 && sub.DiffuseColor.y == 0 && sub.DiffuseColor.z == 0)
                    {
                        // No texture and black color - set default gray
                        std::cout << "    ? No texture and black baseColor - using default gray" << std::endl;
                        sub.DiffuseColor = Vec3{0.8f, 0.8f, 0.8f};
                    }
                }
                
                // Normal map
                int normalTexIndex = -1;
                if (mat.additionalValues.count("normalTexture"))
                {
                    auto& normalTex = mat.additionalValues.at("normalTexture");
                    if (normalTex.has_number_value)
                    {
                        normalTexIndex = (int)normalTex.number_value;
                    }
                    else if (!normalTex.json_double_value.empty())
                    {
                        if (normalTex.json_double_value.count("index"))
                        {
                            normalTexIndex = (int)normalTex.json_double_value.at("index");
                        }
                    }
                }
                
                if (normalTexIndex >= 0)
                {
                    std::cout << "    Loading normal texture (index=" << normalTexIndex << ")..." << std::endl;
                    
                    sub.NormalTexture = LoadGLTFTexture(model, normalTexIndex, dir);
                    if (sub.NormalTexture != 0)
                    {
                        sub.HasNormalTexture = true;
                        sub.NormalTexturePath = (normalTexIndex < (int)model.textures.size() && 
                                                 model.textures[normalTexIndex].source < (int)model.images.size()) ?
                            model.images[model.textures[normalTexIndex].source].uri : "[embedded]";
                    }
                }
            }
            
            allSubMeshes.push_back(std::move(sub));
        }
    }
    
    if (allVertices.empty())
    {
        std::cerr << "GLTF: No vertices loaded" << std::endl;
        return false;
    }
    
    Vertices = std::move(allVertices);
    SubMeshes = std::move(allSubMeshes);
    
    std::cout << "GLTF loaded: " << Vertices.size() << " vertices, " 
              << SubMeshes.size() << " submeshes" << std::endl;
    
    Upload();
    return true;
    
    #else
    std::cerr << "GLTF support not compiled. Add tiny_gltf.h to Dependencies and define TINYGLTF_IMPLEMENTATION" << std::endl;
    return false;
    #endif
}

// Load a 2D texture from disk and create an OpenGL texture object
static unsigned int LoadTextureFromFile(const std::string& path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) return 0;

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
    return tex;
}

void Mesh::Upload()
{
    if (VAO != 0)
        return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        Vertices.size() * sizeof(Vertex),
        Vertices.data(),
        GL_STATIC_DRAW
    );

    // Upload SubMesh EBOs if we have multiple materials
    if (!SubMeshes.empty())
    {
        for (auto& sub : SubMeshes)
        {
            glGenBuffers(1, &sub.EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                sub.Indices.size() * sizeof(uint32_t),
                sub.Indices.data(),
                GL_STATIC_DRAW
            );
        }
    }
    else
    {
        // Legacy single EBO
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            Indices.size() * sizeof(uint32_t),
            Indices.data(),
            GL_STATIC_DRAW
        );
    }

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Normal)
    );
    glEnableVertexAttribArray(1);

    // UV (vec2 stored in Vec3 UV.x, UV.y)
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, UV)
    );
    glEnableVertexAttribArray(2);

    // Tangent attribute
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, Tangent)
    );
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

bool Mesh::LoadTexture(const std::string& path)
{
    unsigned int tex = LoadTextureFromFile(path);
    if (tex == 0) return false;
    DiffuseTexture = tex;
    HasDiffuseTexture = true;
    DiffuseTexturePath = path;
    return true;
}

void Mesh::UnloadTexture()
{
    if (DiffuseTexture != 0)
    {
        glDeleteTextures(1, &DiffuseTexture);
        DiffuseTexture = 0;
        HasDiffuseTexture = false;
    }
}

bool Mesh::LoadSpecularTexture(const std::string& path)
{
    unsigned int tex = LoadTextureFromFile(path);
    if (tex == 0) return false;
    SpecularTexture = tex;
    HasSpecularTexture = true;
    SpecularTexturePath = path;
    return true;
}

void Mesh::UnloadSpecularTexture()
{
    if (SpecularTexture != 0)
    {
        glDeleteTextures(1, &SpecularTexture);
        SpecularTexture = 0;
        HasSpecularTexture = false;
    }
}

bool Mesh::LoadNormalTexture(const std::string& path)
{
    unsigned int tex = LoadTextureFromFile(path);
    if (tex == 0) return false;
    NormalTexture = tex;
    HasNormalTexture = true;
    NormalTexturePath = path;
    return true;
}

void Mesh::UnloadNormalTexture()
{
    if (NormalTexture != 0)
    {
        glDeleteTextures(1, &NormalTexture);
        NormalTexture = 0;
        HasNormalTexture = false;
    }
}

void Mesh::Draw() const
{
    glBindVertexArray(VAO);
    
    // Legacy single-material draw
    if (SubMeshes.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)Indices.size(), GL_UNSIGNED_INT, 0);
    }
    else
    {
        // Multi-material draw: render each submesh
        // Note: Caller must set appropriate textures/materials between submesh draws
        for (const auto& sub : SubMeshes)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
            glDrawElements(GL_TRIANGLES, (GLsizei)sub.Indices.size(), GL_UNSIGNED_INT, 0);
        }
    }
}
