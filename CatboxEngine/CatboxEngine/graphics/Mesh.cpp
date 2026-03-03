#include "Mesh.h"
#include "GraphicsSettings.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "glfw3.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cstring>
#include <cmath>

// GLTF support - tinygltf will handle stb_image internally
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "../Dependencies/tiny_gltf.h"

// FBX support via ufbx — implementation lives in ufbx_impl.cpp
#include "../Dependencies/ufbx.h"

// Minimal OBJ loader supporting positions, normals and triangular faces
bool Mesh::LoadFromOBJ(const std::string& path)
{
    std::cout << "[DEBUG] LoadFromOBJ called with path: " << path << std::endl;
    std::ifstream in(path);
    if (!in.is_open())
    {
        std::cerr << "[ERROR] LoadFromOBJ: Failed to open file: " << path << std::endl;
        return false;
    }
    std::cout << "[DEBUG] LoadFromOBJ: File opened successfully" << std::endl;

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
                
                if (!mtlMaps.empty() || !mtlNormalMaps.empty())
                {
                    std::cout << "MTL loaded: " << mtlMaps.size() << " diffuse, " 
                              << mtlNormalMaps.size() << " normal maps" << std::endl;
                    
                    // Debug: Print all material texture mappings
                    std::cout << "[DEBUG] Material texture mappings:" << std::endl;
                    for (const auto& pair : mtlMaps)
                    {
                        std::cout << "  Material '" << pair.first << "' -> Diffuse: " << pair.second << std::endl;
                    }
                    for (const auto& pair : mtlSpecularMaps)
                    {
                        std::cout << "  Material '" << pair.first << "' -> Specular: " << pair.second << std::endl;
                    }
                    for (const auto& pair : mtlNormalMaps)
                    {
                        std::cout << "  Material '" << pair.first << "' -> Normal: " << pair.second << std::endl;
                    }
                }
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
                if (ti >=0 && ti < (int)texcoords.size()) 
                { 
                    glm::vec2 uv = texcoords[ti]; 
                    // Flip V coordinate for proper texture mapping
                    v.UV = {uv.x, 1.0f - uv.y, 0}; 
                }
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

    // Validate that we have geometry
    bool hasGeometry = false;
    if (!outVerts.empty())
    {
        // Check if we have indices in either legacy indices or material groups
        if (!outIndices.empty())
        {
            hasGeometry = true;
        }
        else
        {
            // Check if any material group has indices
            for (const auto& group : materialGroups)
            {
                if (!group.indices.empty())
                {
                    hasGeometry = true;
                    break;
                }
            }
        }
    }
    
    if (!hasGeometry)
    {
        std::cerr << "[ERROR] LoadFromOBJ: No geometry found (vertices or indices empty)" << std::endl;
        return false;
    }

    Vertices = std::move(outVerts);
    Indices = std::move(outIndices);

    // compute per-vertex normals if model did not provide them
    bool haveNormals = false;
    for (const auto &v : Vertices) 
    {
        if (!(v.Normal.x == 0.0f && v.Normal.y == 0.0f && v.Normal.z == 0.0f)) 
        { 
            haveNormals = true; 
            break; 
        }
    }
    
    if (!haveNormals)
    {
        std::cout << "OBJ: Computing smooth normals..." << std::endl;
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
    std::cout << "OBJ: Computing tangents..." << std::endl;
    std::vector<glm::vec3> tanAccum(Vertices.size(), glm::vec3(0.0f));
    bool haveUV = false;
    for (const auto &v : Vertices) if (v.UV.x != 0.0f || v.UV.y != 0.0f) { haveUV = true; break; }
    
    // Debug: Print UV range for first 10 vertices
    if (haveUV && Vertices.size() > 0)
    {
        std::cout << "  UV Debug (first 10 vertices):" << std::endl;
        for (size_t i = 0; i < std::min<size_t>(10, Vertices.size()); ++i)
        {
            std::cout << "    v[" << i << "] UV: (" << Vertices[i].UV.x << ", " << Vertices[i].UV.y << ")" << std::endl;
        }
        
        // Find UV range
        float minU = FLT_MAX, maxU = -FLT_MAX;
        float minV = FLT_MAX, maxV = -FLT_MAX;
        for (const auto& v : Vertices)
        {
            minU = std::min(minU, v.UV.x);
            maxU = std::max(maxU, v.UV.x);
            minV = std::min(minV, v.UV.y);
            maxV = std::max(maxV, v.UV.y);
        }
        std::cout << "  UV Range: U[" << minU << ", " << maxU << "], V[" << minV << ", " << maxV << "]" << std::endl;
    }
    
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
    
    // Helper function to try loading texture with fallback patterns
    auto TryLoadTextureWithFallbacks = [&objDir](const std::string& texPath, int& width, int& height, int& channels) -> unsigned char* {
        std::vector<std::string> pathsToTry;
        
        // Build full path
        std::string full = texPath;
        if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) 
            full = objDir + texPath;
        
        // Extract directory and filename
        size_t lastSlash = full.find_last_of("/\\");
        std::string dir = (lastSlash != std::string::npos) ? full.substr(0, lastSlash + 1) : "";
        std::string filename = (lastSlash != std::string::npos) ? full.substr(lastSlash + 1) : full;
        
        // Extract base name and extension
        size_t extPos = filename.find_last_of('.');
        std::string baseName = (extPos != std::string::npos) ? filename.substr(0, extPos) : filename;
        std::string ext = (extPos != std::string::npos) ? filename.substr(extPos) : "";
        
        // Generate variations
        std::vector<std::string> baseVariations;
        baseVariations.push_back(baseName);           // Original
        baseVariations.push_back("1_" + baseName);    // With 1_ prefix
        baseVariations.push_back(baseName + "1");     // With 1 suffix
        baseVariations.push_back("1_" + baseName + "1"); // Both
        
        // Handle plural/singular (object vs objects)
        if (baseName.back() != 's')
        {
            baseVariations.push_back(baseName + "s");
            baseVariations.push_back("1_" + baseName + "s");
            baseVariations.push_back(baseName + "s1");
            baseVariations.push_back("1_" + baseName + "s1");
        }
        else
        {
            std::string singular = baseName.substr(0, baseName.length() - 1);
            baseVariations.push_back(singular);
            baseVariations.push_back("1_" + singular);
            baseVariations.push_back(singular + "1");
            baseVariations.push_back("1_" + singular + "1");
        }
        
        // Extension variations
        std::vector<std::string> extVariations;
        extVariations.push_back(ext);  // Original case
        
        std::string extUpper = ext;
        for (auto& c : extUpper) c = (char)toupper(c);
        extVariations.push_back(extUpper);
        
        std::string extLower = ext;
        for (auto& c : extLower) c = (char)tolower(c);
        extVariations.push_back(extLower);
        
        // Combine all variations
        for (const auto& base : baseVariations)
        {
            for (const auto& extension : extVariations)
            {
                pathsToTry.push_back(dir + base + extension);
            }
        }
        
        // Try each path
        for (const auto& path : pathsToTry)
        {
            // Don't flip for OBJ files - Blender and most modern exporters 
            // already export with correct UV orientation
            stbi_set_flip_vertically_on_load(false);
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
            if (data)
            {
                if (path != full)
                {
                    std::cout << "  Found texture using fallback: " << path << std::endl;
                }
                return data;
            }
        }
        
        return nullptr;
    };
    
    // Create SubMeshes for multi-material models
    if (!materialGroups.empty())
    {
        std::cout << "OBJ: Creating " << materialGroups.size() << " submeshes" << std::endl;
        
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
                
                std::cout << "[DEBUG] Loading diffuse texture for material '" << group.materialName << "'" << std::endl;
                std::cout << "  MTL texture path: " << texPath << std::endl;
                
                int width, height, channels;
                unsigned char* data = TryLoadTextureWithFallbacks(texPath, width, height, channels);
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
                    
                    std::cout << "  ? Diffuse texture loaded successfully (" << width << "x" << height << ")" << std::endl;
                    
                    sub.HasDiffuseTexture = true;
                    sub.DiffuseTexturePath = texPath;
                }
                else
                {
                    std::cerr << "  ? Failed to load diffuse texture (tried multiple paths)" << std::endl;
                    std::cerr << "    Reason: " << stbi_failure_reason() << std::endl;
                }
            }
            else
            {
                std::cout << "[DEBUG] No diffuse texture mapped for material '" << group.materialName << "'" << std::endl;
            }
            
            // Load specular texture
            auto itsm = mtlSpecularMaps.find(group.materialName);
            if (itsm != mtlSpecularMaps.end())
            {
                std::string texPath = itsm->second;
                
                std::cout << "[DEBUG] Loading specular texture for material '" << group.materialName << "'" << std::endl;
                
                int width, height, channels;
                unsigned char* data = TryLoadTextureWithFallbacks(texPath, width, height, channels);
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
                    
                    std::cout << "  ? Specular texture loaded successfully" << std::endl;
                    
                    sub.HasSpecularTexture = true;
                    sub.SpecularTexturePath = texPath;
                }
                else
                {
                    std::cerr << "  ? Failed to load specular texture (tried multiple paths)" << std::endl;
                }
            }
            
            // Load normal map
            auto itnm = mtlNormalMaps.find(group.materialName);
            if (itnm != mtlNormalMaps.end())
            {
                std::string texPath = itnm->second;
                
                std::cout << "[DEBUG] Loading normal map for material '" << group.materialName << "'" << std::endl;
                
                int width, height, channels;
                unsigned char* data = TryLoadTextureWithFallbacks(texPath, width, height, channels);
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
                    
                    std::cout << "  ? Normal map loaded successfully" << std::endl;
                    
                    sub.HasNormalTexture = true;
                    sub.NormalTexturePath = texPath;
                }
                else
                {
                    std::cerr << "  ? Failed to load normal map (tried multiple paths)" << std::endl;
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
            }
        }
    }

    // Upload to GPU
    Upload();
    
    // Calculate bounding box for frustum culling
    CalculateBounds();

    // Summary
    std::cout << "OBJ loaded successfully:" << std::endl;
    std::cout << "  Vertices: " << Vertices.size() << std::endl;
    std::cout << "  Indices: " << (!SubMeshes.empty() ? 0 : Indices.size()) << std::endl;
    std::cout << "  SubMeshes: " << SubMeshes.size() << std::endl;
    std::cout << "  Has Normals: " << (haveNormals ? "Yes" : "Computed") << std::endl;
    std::cout << "  Has UVs: " << (haveUV ? "Yes" : "No") << std::endl;
    std::cout << "  Has Tangents: " << (haveUV ? "Yes" : "No") << std::endl;

    // Validate vertex data
    if (!ValidateVertexData())
    {
        std::cerr << "OBJ validation failed!" << std::endl;
        return false;
    }

    return true;
}

bool Mesh::LoadFromGLTF(const std::string& path)
{
    // Note: You need to download tiny_gltf.h and place it in Dependencies/
    // See docs/GLTF_Setup.md for instructions
    #ifdef TINYGLTF_IMPLEMENTATION
    
    // Helper function to load texture from GLTF image (handles both embedded and external)
    auto LoadGLTFTexture = [&](const tinygltf::Model& model, int texIndex, const std::string& dir) -> unsigned int {
        if (texIndex < 0 || texIndex >= (int)model.textures.size()) return 0;

        const tinygltf::Texture& tex = model.textures[texIndex];
        if (tex.source < 0 || tex.source >= (int)model.images.size()) return 0;

        const tinygltf::Image& img = model.images[tex.source];

        // tinygltf decodes images during loading, so img.image already
        // contains raw pixel data (not PNG/JPEG bytes).  Use it directly.
        if (img.width > 0 && img.height > 0 && !img.image.empty())
        {
            GLenum format = (img.component == 4) ? GL_RGBA : GL_RGB;

            unsigned int texID;
            glGenTextures(1, &texID);
            glBindTexture(GL_TEXTURE_2D, texID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0,
                         format, GL_UNSIGNED_BYTE, img.image.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            return texID;
        }

        // Fallback: load from file if tinygltf didn't decode the image
        if (!img.uri.empty())
        {
            // Decode percent-encoding in the URI (e.g. %20 -> space)
            std::string decoded;
            decoded.reserve(img.uri.size());
            for (size_t i = 0; i < img.uri.size(); ++i)
            {
                if (img.uri[i] == '%' && i + 2 < img.uri.size())
                {
                    char hex[3] = { img.uri[i+1], img.uri[i+2], 0 };
                    char* end = nullptr;
                    long val = strtol(hex, &end, 16);
                    if (end == hex + 2) { decoded += (char)val; i += 2; continue; }
                }
                decoded += img.uri[i];
            }

            std::string texPath = dir + decoded;
            int width = 0, height = 0, channels = 0;
            unsigned char* data = stbi_load(texPath.c_str(), &width, &height, &channels, 4);
            if (data)
            {
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
                return texID;
            }

            std::cerr << "  Failed to load texture: " << img.uri << std::endl;
        }

        return 0;
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
    std::cout << "  Meshes: " << model.meshes.size() 
              << ", Materials: " << model.materials.size() 
              << ", Textures: " << model.textures.size() << std::endl;
    
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
        
        // Each primitive in GLTF is like a submesh
        for (size_t primIdx = 0; primIdx < gltfMesh.primitives.size(); ++primIdx)
        {
            const tinygltf::Primitive& primitive = gltfMesh.primitives[primIdx];
            
            SubMesh sub;
            sub.BaseVertex = (uint32_t)allVertices.size();
            sub.MaterialName = (primitive.material >= 0) ? 
                model.materials[primitive.material].name : "default";
            
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
            
            // Load morph targets (blend shapes)
            if (!primitive.targets.empty())
            {
                std::cout << "  Found " << primitive.targets.size() << " morph targets" << std::endl;

                // Helper: read a VEC3 accessor into a Vec3 vector, handling
                // sparse-only accessors (bufferView == -1) where the base data
                // is all zeros and only the sparse entries carry values.
                auto ReadVec3Accessor = [&](const tinygltf::Accessor& acc) -> std::vector<Vec3> {
                    std::vector<Vec3> result(acc.count, Vec3{0, 0, 0});

                    // Load base data when a bufferView is present
                    if (acc.bufferView >= 0 && acc.bufferView < (int)model.bufferViews.size())
                    {
                        const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
                        const tinygltf::Buffer& buf = model.buffers[bv.buffer];
                        const float* d = reinterpret_cast<const float*>(
                            &buf.data[bv.byteOffset + acc.byteOffset]);
                        for (size_t i = 0; i < acc.count; ++i)
                            result[i] = Vec3{d[i * 3], d[i * 3 + 1], d[i * 3 + 2]};
                    }

                    // Overlay sparse values (if any)
                    if (acc.sparse.isSparse)
                    {
                        const auto& si = acc.sparse.indices;
                        const tinygltf::BufferView& idxBV = model.bufferViews[si.bufferView];
                        const uint8_t* idxRaw = &model.buffers[idxBV.buffer].data[idxBV.byteOffset + si.byteOffset];

                        const auto& sv = acc.sparse.values;
                        const tinygltf::BufferView& valBV = model.bufferViews[sv.bufferView];
                        const float* valData = reinterpret_cast<const float*>(
                            &model.buffers[valBV.buffer].data[valBV.byteOffset + sv.byteOffset]);

                        for (int s = 0; s < acc.sparse.count; ++s)
                        {
                            uint32_t idx = 0;
                            if (si.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                                idx = reinterpret_cast<const uint16_t*>(idxRaw)[s];
                            else if (si.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                                idx = reinterpret_cast<const uint32_t*>(idxRaw)[s];
                            else
                                idx = idxRaw[s];

                            if (idx < acc.count)
                                result[idx] = Vec3{valData[s * 3], valData[s * 3 + 1], valData[s * 3 + 2]};
                        }
                    }

                    return result;
                };

                for (size_t targetIdx = 0; targetIdx < primitive.targets.size(); ++targetIdx)
                {
                    const auto& target = primitive.targets[targetIdx];

                    MorphTarget morphTarget;

                    // Get morph target name from mesh extras if available
                    auto targetNames = gltfMesh.extras.Get("targetNames");
                    if (targetNames.IsArray() && targetIdx < targetNames.ArrayLen())
                    {
                        morphTarget.Name = targetNames.Get(targetIdx).Get<std::string>();
                    }

                    // Fallback name
                    if (morphTarget.Name.empty())
                    {
                        morphTarget.Name = "Target_" + std::to_string(targetIdx);
                    }

                    // Load position deltas
                    if (target.count("POSITION"))
                        morphTarget.PositionDeltas = ReadVec3Accessor(model.accessors[target.at("POSITION")]);

                    // Load normal deltas
                    if (target.count("NORMAL"))
                        morphTarget.NormalDeltas = ReadVec3Accessor(model.accessors[target.at("NORMAL")]);

                    // Load tangent deltas
                    if (target.count("TANGENT"))
                        morphTarget.TangentDeltas = ReadVec3Accessor(model.accessors[target.at("TANGENT")]);

                    MorphTargets.push_back(morphTarget);
                    std::cout << "    Loaded morph target: " << morphTarget.Name << std::endl;
                }
            }
            
            // Load material
            if (primitive.material >= 0)
            {
                const tinygltf::Material& mat = model.materials[primitive.material];

                // PBR metallic-roughness base color factor
                const auto& bcf = mat.pbrMetallicRoughness.baseColorFactor;
                if (bcf.size() >= 3)
                {
                    sub.DiffuseColor = Vec3{(float)bcf[0], (float)bcf[1], (float)bcf[2]};
                }

                // Base color texture (diffuse)
                int baseColorTexIndex = mat.pbrMetallicRoughness.baseColorTexture.index;

                if (baseColorTexIndex >= 0)
                {
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
                    // FALLBACK: Try emissive texture if baseColor is missing (for incorrectly exported models)
                    int emissiveTexIndex = mat.emissiveTexture.index;

                    if (emissiveTexIndex >= 0)
                    {
                        std::cout << "  Warning: Using emissive texture as diffuse for material '" 
                                  << mat.name << "'" << std::endl;
                        sub.DiffuseTexture = LoadGLTFTexture(model, emissiveTexIndex, dir);
                        if (sub.DiffuseTexture != 0)
                        {
                            sub.HasDiffuseTexture = true;
                            sub.DiffuseTexturePath = (emissiveTexIndex < (int)model.textures.size() && 
                                                      model.textures[emissiveTexIndex].source < (int)model.images.size()) ?
                                model.images[model.textures[emissiveTexIndex].source].uri : "[embedded]";
                            sub.DiffuseColor = Vec3{1.0f, 1.0f, 1.0f};
                        }
                    }
                    else if (sub.DiffuseColor.x == 0 && sub.DiffuseColor.y == 0 && sub.DiffuseColor.z == 0)
                    {
                        // No texture and black color - use default
                        sub.DiffuseColor = Vec3{0.8f, 0.8f, 0.8f};
                    }
                }

                // Normal map
                int normalTexIndex = mat.normalTexture.index;

                if (normalTexIndex >= 0)
                {
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
    
    // Store base vertices for morph targets
    if (!MorphTargets.empty())
    {
        BaseVertices = Vertices;
        std::cout << "  Stored " << MorphTargets.size() << " morph targets" << std::endl;
    }
    
    std::cout << "GLTF loaded: " << Vertices.size() << " vertices, " 
              << SubMeshes.size() << " submeshes" << std::endl;
    
    Upload();
    CalculateBounds();  // Calculate bounding box for frustum culling
    
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
    
    // Generate mipmaps if enabled in settings
    auto& settings = GraphicsSettings::Instance();
    if (settings.EnableMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Apply graphics settings
    settings.ApplyToTexture(tex);

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

    // Bone indices (ivec4)
    glVertexAttribIPointer(
        4, 4, GL_INT, sizeof(Vertex),
        (void*)offsetof(Vertex, BoneIndices)
    );
    glEnableVertexAttribArray(4);

    // Bone weights (vec4)
    glVertexAttribPointer(
        5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, BoneWeights)
    );
    glEnableVertexAttribArray(5);

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

// Validate vertex data
bool Mesh::ValidateVertexData() const
{
    if (Vertices.empty())
    {
        std::cerr << "Validation failed: No vertices!" << std::endl;
        return false;
    }
    
    if (SubMeshes.empty() && Indices.empty())
    {
        std::cerr << "Validation failed: No indices!" << std::endl;
        return false;
    }
    
    // Check for NaN/Inf in vertex data
    for (size_t i = 0; i < Vertices.size(); ++i)
    {
        const auto& v = Vertices[i];
        
        // Check position
        if (std::isnan(v.Position.x) || std::isnan(v.Position.y) || std::isnan(v.Position.z) ||
            std::isinf(v.Position.x) || std::isinf(v.Position.y) || std::isinf(v.Position.z))
        {
            std::cerr << "Validation failed: Invalid position at vertex " << i << std::endl;
            return false;
        }
        
        // Check normal
        if (std::isnan(v.Normal.x) || std::isnan(v.Normal.y) || std::isnan(v.Normal.z))
        {
            std::cerr << "Validation failed: Invalid normal at vertex " << i << std::endl;
            return false;
        }
        
        // Check UV
        if (std::isnan(v.UV.x) || std::isnan(v.UV.y))
        {
            std::cerr << "Validation failed: Invalid UV at vertex " << i << std::endl;
            return false;
        }
    }
    
    // Check index bounds
    auto checkIndices = [&](const std::vector<uint32_t>& indices, const char* name) {
        for (size_t i = 0; i < indices.size(); ++i)
        {
            if (indices[i] >= Vertices.size())
            {
                std::cerr << "Validation failed: " << name << " index " << i 
                          << " (" << indices[i] << ") out of bounds (max: " 
                          << Vertices.size() - 1 << ")" << std::endl;
                return false;
            }
        }
        return true;
    };
    
    if (!SubMeshes.empty())
    {
        for (size_t i = 0; i < SubMeshes.size(); ++i)
        {
            std::string name = "SubMesh[" + std::to_string(i) + "]";
            if (!checkIndices(SubMeshes[i].Indices, name.c_str()))
                return false;
        }
    }
    else
    {
        if (!checkIndices(Indices, "Mesh"))
            return false;
    }
    
    std::cout << "? Vertex data validation passed!" << std::endl;
    return true;
}

// Print debug info
void Mesh::PrintDebugInfo() const
{
    std::cout << "\n=== MESH DEBUG INFO ===" << std::endl;
    std::cout << "Vertices: " << Vertices.size() << std::endl;
    std::cout << "Indices: " << Indices.size() << std::endl;
    std::cout << "SubMeshes: " << SubMeshes.size() << std::endl;
    std::cout << "VAO: " << VAO << ", VBO: " << VBO << ", EBO: " << EBO << std::endl;
    
    if (!Vertices.empty())
    {
        std::cout << "\nFirst vertex:" << std::endl;
        const auto& v = Vertices[0];
        std::cout << "  Position: (" << v.Position.x << ", " << v.Position.y << ", " << v.Position.z << ")" << std::endl;
        std::cout << "  Normal: (" << v.Normal.x << ", " << v.Normal.y << ", " << v.Normal.z << ")" << std::endl;
        std::cout << "  UV: (" << v.UV.x << ", " << v.UV.y << ")" << std::endl;
        std::cout << "  Tangent: (" << v.Tangent.x << ", " << v.Tangent.y << ", " << v.Tangent.z << ")" << std::endl;
    }
    
    // Check if all vertices have valid normals
    int validNormals = 0;
    int validUVs = 0;
    int validTangents = 0;
    
    for (const auto& v : Vertices)
    {
        if (v.Normal.x != 0.0f || v.Normal.y != 0.0f || v.Normal.z != 0.0f)
            validNormals++;
        if (v.UV.x != 0.0f || v.UV.y != 0.0f)
            validUVs++;
        if (v.Tangent.x != 0.0f || v.Tangent.y != 0.0f || v.Tangent.z != 0.0f)
            validTangents++;
    }
    
    std::cout << "\nVertex attributes:" << std::endl;
    std::cout << "  Normals: " << validNormals << "/" << Vertices.size() 
              << " (" << (validNormals * 100.0f / Vertices.size()) << "%)" << std::endl;
    std::cout << "  UVs: " << validUVs << "/" << Vertices.size() 
              << " (" << (validUVs * 100.0f / Vertices.size()) << "%)" << std::endl;
    std::cout << "  Tangents: " << validTangents << "/" << Vertices.size() 
              << " (" << (validTangents * 100.0f / Vertices.size()) << "%)" << std::endl;
    
    if (!SubMeshes.empty())
    {
        std::cout << "\nSubMesh details:" << std::endl;
        for (size_t i = 0; i < SubMeshes.size(); ++i)
        {
            const auto& sub = SubMeshes[i];
            std::cout << "  [" << i << "] " << sub.MaterialName 
                      << ": " << sub.Indices.size() << " indices, "
                      << "BaseVertex=" << sub.BaseVertex
                      << ", Textures=" << (sub.HasDiffuseTexture ? "D" : "-")
                      << (sub.HasSpecularTexture ? "S" : "-")
                      << (sub.HasNormalTexture ? "N" : "-")
                      << std::endl;
        }
    }
    
    std::cout << "=====================\n" << std::endl;
}

// Morph target (blend shape) methods
void Mesh::SetMorphTargetWeight(size_t index, float weight)
{
    if (index < MorphTargets.size())
    {
        MorphTargets[index].Weight = glm::clamp(weight, 0.0f, 1.0f);
        UpdateMorphTargets();
    }
}

void Mesh::SetMorphTargetWeight(const std::string& name, float weight)
{
    for (size_t i = 0; i < MorphTargets.size(); ++i)
    {
        if (MorphTargets[i].Name == name)
        {
            SetMorphTargetWeight(i, weight);
            return;
        }
    }
    std::cerr << "Morph target not found: " << name << std::endl;
}

void Mesh::UpdateMorphTargets()
{
    if (MorphTargets.empty() || BaseVertices.empty())
        return;
    
    // Start with base vertices
    Vertices = BaseVertices;
    
    // Apply each morph target with its weight
    for (const auto& target : MorphTargets)
    {
        if (target.Weight <= 0.0f)
            continue;
        
        size_t count = std::min(Vertices.size(), target.PositionDeltas.size());
        for (size_t i = 0; i < count; ++i)
        {
            // Apply position delta
            Vertices[i].Position.x += target.PositionDeltas[i].x * target.Weight;
            Vertices[i].Position.y += target.PositionDeltas[i].y * target.Weight;
            Vertices[i].Position.z += target.PositionDeltas[i].z * target.Weight;
            
            // Apply normal delta if available
            if (i < target.NormalDeltas.size())
            {
                Vertices[i].Normal.x += target.NormalDeltas[i].x * target.Weight;
                Vertices[i].Normal.y += target.NormalDeltas[i].y * target.Weight;
                Vertices[i].Normal.z += target.NormalDeltas[i].z * target.Weight;
            }
            
            // Apply tangent delta if available
            if (i < target.TangentDeltas.size())
            {
                Vertices[i].Tangent.x += target.TangentDeltas[i].x * target.Weight;
                Vertices[i].Tangent.y += target.TangentDeltas[i].y * target.Weight;
                Vertices[i].Tangent.z += target.TangentDeltas[i].z * target.Weight;
            }
        }
    }
    
    // Re-upload to GPU
    if (VAO != 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(Vertex), Vertices.data());
    }
}

void Mesh::CalculateBounds()
{
    if (Vertices.empty())
    {
        BoundsMin = {0, 0, 0};
        BoundsMax = {0, 0, 0};
        return;
    }
    
    BoundsMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    BoundsMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    
    for (const auto& v : Vertices)
    {
        BoundsMin.x = std::min(BoundsMin.x, v.Position.x);
        BoundsMin.y = std::min(BoundsMin.y, v.Position.y);
        BoundsMin.z = std::min(BoundsMin.z, v.Position.z);
        
        BoundsMax.x = std::max(BoundsMax.x, v.Position.x);
        BoundsMax.y = std::max(BoundsMax.y, v.Position.y);
        BoundsMax.z = std::max(BoundsMax.z, v.Position.z);
    }
    
    std::cout << "Mesh bounds: Min(" << BoundsMin.x << ", " << BoundsMin.y << ", " << BoundsMin.z << ") "
              << "Max(" << BoundsMax.x << ", " << BoundsMax.y << ", " << BoundsMax.z << ")" << std::endl;
}

// Memory tracking methods
size_t Mesh::GetCPUMemoryUsage() const
{
    size_t total = 0;
    
    // Vertex data
    total += Vertices.size() * sizeof(Vertex);
    
    // Index data (legacy)
    total += Indices.size() * sizeof(uint32_t);
    
    // SubMesh data
    for (const auto& sub : SubMeshes)
    {
        total += sub.Indices.size() * sizeof(uint32_t);
        total += sub.MaterialName.capacity();
        total += sub.DiffuseTexturePath.capacity();
        total += sub.SpecularTexturePath.capacity();
        total += sub.NormalTexturePath.capacity();
    }
    
    return total;
}

size_t Mesh::GetGPUMemoryUsage() const
{
    size_t total = 0;
    
    // VBO (vertex buffer)
    if (VAO != 0)
    {
        total += Vertices.size() * sizeof(Vertex);
    }
    
    // EBO (index buffer) - legacy
    if (EBO != 0)
    {
        total += Indices.size() * sizeof(uint32_t);
    }
    
    // SubMesh EBOs
    for (const auto& sub : SubMeshes)
    {
        if (sub.EBO != 0)
        {
            total += sub.Indices.size() * sizeof(uint32_t);
        }
    }
    
    // Textures (approximate - assumes RGBA 8-bit with mipmaps)
    auto estimateTextureSize = [](unsigned int texID) -> size_t {
        if (texID == 0) return 0;
        
        // Query texture size from OpenGL
        glBindTexture(GL_TEXTURE_2D, texID);
        int width = 0, height = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        
        // RGBA 8-bit + mipmaps (approximately 1.33x base size)
        size_t baseSize = width * height * 4;
        return (size_t)(baseSize * 1.33f);
    };
    
    // Legacy textures
    total += estimateTextureSize(DiffuseTexture);
    total += estimateTextureSize(SpecularTexture);
    total += estimateTextureSize(NormalTexture);
    
    // SubMesh textures
    for (const auto& sub : SubMeshes)
    {
        total += estimateTextureSize(sub.DiffuseTexture);
        total += estimateTextureSize(sub.SpecularTexture);
        total += estimateTextureSize(sub.NormalTexture);
    }

    return total;
}

// ---- FBX loader (ufbx) -------------------------------------------------------

bool Mesh::LoadFromFBX(const std::string& path)
{
    // Load with Y-up right-handed axes to match OpenGL, and convert units to metres.
    ufbx_load_opts opts = {};
    opts.target_axes            = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters     = 1.0f;
    opts.generate_missing_normals = true;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);
    if (!scene)
    {
        std::cerr << "FBX load failed for \"" << path << "\": "
                  << error.description.data << "\n";
        return false;
    }

    // Directory for resolving relative texture paths
    std::string dir;
    const size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) dir = path.substr(0, slash + 1);

    // Rotate a vec3 by a unit quaternion: v' = q * v * q^-1
    auto rotVec = [](const ufbx_quat& q, float vx, float vy, float vz) -> Vec3 {
        float qx = (float)q.x, qy = (float)q.y, qz = (float)q.z, qw = (float)q.w;
        float tx = 2.0f * (qy * vz - qz * vy);
        float ty = 2.0f * (qz * vx - qx * vz);
        float tz = 2.0f * (qx * vy - qy * vx);
        return {
            vx + qw * tx + (qy * tz - qz * ty),
            vy + qw * ty + (qz * tx - qx * tz),
            vz + qw * tz + (qx * ty - qy * tx)
        };
    };

    std::vector<Vertex>    allVertices;
    std::vector<SubMesh>   allSubMeshes;

    // -----------------------------------------------------------------------
    // Skeleton extraction: build bone hierarchy from all skin deformers
    // -----------------------------------------------------------------------
    Skeleton loadedSkeleton;
    bool hasSkin = false;

    // Helper: ufbx_matrix (column-major 4x3) -> glm::mat4
    auto ufbxToGlm = [](const ufbx_matrix& m) -> glm::mat4 {
        glm::mat4 r(1.0f);
        r[0][0]=(float)m.cols[0].x; r[0][1]=(float)m.cols[0].y; r[0][2]=(float)m.cols[0].z; r[0][3]=0.0f;
        r[1][0]=(float)m.cols[1].x; r[1][1]=(float)m.cols[1].y; r[1][2]=(float)m.cols[1].z; r[1][3]=0.0f;
        r[2][0]=(float)m.cols[2].x; r[2][1]=(float)m.cols[2].y; r[2][2]=(float)m.cols[2].z; r[2][3]=0.0f;
        r[3][0]=(float)m.cols[3].x; r[3][1]=(float)m.cols[3].y; r[3][2]=(float)m.cols[3].z; r[3][3]=1.0f;
        return r;
    };

    // Pass 1: collect every bone name + inverse bind pose (no parent indices yet)
    struct RawBone {
        std::string name;
        glm::mat4 inverseBindPose;
        ufbx_node* boneNode = nullptr;
    };
    std::vector<RawBone> rawBones;
    std::unordered_map<std::string, size_t> rawBoneMap;

    for (size_t si = 0; si < scene->skin_deformers.count; ++si)
    {
        ufbx_skin_deformer* skin = scene->skin_deformers.data[si];
        for (size_t ci = 0; ci < skin->clusters.count; ++ci)
        {
            ufbx_skin_cluster* cluster = skin->clusters.data[ci];
            if (!cluster->bone_node) continue;
            const std::string boneName = cluster->bone_node->name.data;
            if (rawBoneMap.count(boneName)) continue;

            rawBoneMap[boneName] = rawBones.size();
            rawBones.push_back({ boneName, ufbxToGlm(cluster->geometry_to_bone), cluster->bone_node });
        }
        hasSkin = true;
    }

    // Pass 2: resolve parent indices now that all bones are known, then
    //         topological-sort so every parent index < child index.
    if (hasSkin && !rawBones.empty())
    {
        // Build unsorted bone list with correct parent indices
        std::vector<int> unsortedParent(rawBones.size(), -1);
        for (size_t i = 0; i < rawBones.size(); ++i)
        {
            ufbx_node* p = rawBones[i].boneNode->parent;
            while (p)
            {
                auto it = rawBoneMap.find(p->name.data);
                if (it != rawBoneMap.end())
                {
                    unsortedParent[i] = (int)it->second;
                    break;
                }
                p = p->parent;
            }
        }

        // Topological sort (Kahn's algorithm): parents always come first
        std::vector<int> sortOrder;
        sortOrder.reserve(rawBones.size());
        std::vector<bool> placed(rawBones.size(), false);

        // Repeatedly pick bones whose parent is already placed (or is -1)
        while (sortOrder.size() < rawBones.size())
        {
            bool progress = false;
            for (size_t i = 0; i < rawBones.size(); ++i)
            {
                if (placed[i]) continue;
                int pid = unsortedParent[i];
                if (pid == -1 || placed[pid])
                {
                    sortOrder.push_back((int)i);
                    placed[i] = true;
                    progress = true;
                }
            }
            if (!progress)
            {
                // Remaining bones form a cycle — just append them
                for (size_t i = 0; i < rawBones.size(); ++i)
                    if (!placed[i]) sortOrder.push_back((int)i);
                break;
            }
        }

        // Build the old->new index mapping
        std::vector<int> oldToNew(rawBones.size(), -1);
        for (size_t newIdx = 0; newIdx < sortOrder.size(); ++newIdx)
            oldToNew[sortOrder[newIdx]] = (int)newIdx;

        // Populate the final skeleton in sorted order
        loadedSkeleton.Bones.resize(sortOrder.size());
        for (size_t newIdx = 0; newIdx < sortOrder.size(); ++newIdx)
        {
            int oldIdx = sortOrder[newIdx];
            Bone& b = loadedSkeleton.Bones[newIdx];
            b.Name = rawBones[oldIdx].name;
            b.InverseBindPose = rawBones[oldIdx].inverseBindPose;
            int oldParent = unsortedParent[oldIdx];
            b.ParentIndex = (oldParent >= 0) ? oldToNew[oldParent] : -1;
            loadedSkeleton.BoneNameToIndex[b.Name] = (int)newIdx;
        }

        std::cout << "FBX: skeleton extracted with " << loadedSkeleton.Bones.size() << " bones\n";
    }

    // ufbx gives us one ufbx_mesh per mesh node; iterate through mesh instances
    for (size_t ni = 0; ni < scene->nodes.count; ++ni)
    {
        ufbx_node* node = scene->nodes.data[ni];
        if (!node->mesh) continue;
        ufbx_mesh* fbxMesh = node->mesh;

        // The node's local_transform.rotation includes pre_rotation which
        // many FBX exporters use for axis conversion (e.g. -90° X from Blender).
        // Apply it to vertices so the model stands upright at rotation (0,0,0).
        // For SKINNED meshes, skip this — we fold the mesh node's transform
        // into the skeleton so the bones handle the correction instead.
        const bool isSkinned = hasSkin && fbxMesh->skin_deformers.count > 0;
        const ufbx_quat nodeRot = node->local_transform.rotation;
        const bool hasRotation = !isSkinned &&
            !(std::abs(nodeRot.x) < 1e-6 && std::abs(nodeRot.y) < 1e-6 &&
              std::abs(nodeRot.z) < 1e-6 && std::abs(nodeRot.w - 1.0) < 1e-6);

        // Capture the mesh node's full local transform for the skeleton
        if (isSkinned && !loadedSkeleton.Bones.empty())
        {
            loadedSkeleton.MeshNodeTransform = ufbxToGlm(node->node_to_parent);
        }

        // Triangulate into a flat index list (ufbx guarantees tris after this)
        const size_t maxTriIndices = fbxMesh->max_face_triangles * 3;
        std::vector<uint32_t> triIndices(maxTriIndices);

        // One SubMesh per material slot (or one if there are no materials)
        const size_t numParts = fbxMesh->material_parts.count > 0
                              ? fbxMesh->material_parts.count : 1;

        for (size_t pi = 0; pi < numParts; ++pi)
        {
            SubMesh sub;
            sub.BaseVertex = (uint32_t)allVertices.size();

            // Gather the face range for this material part
            uint32_t faceBegin = 0, faceEnd = (uint32_t)fbxMesh->faces.count;
            if (fbxMesh->material_parts.count > 0)
            {
                const ufbx_mesh_part& part = fbxMesh->material_parts.data[pi];
                // part.face_indices lists which faces belong to this part
                // We iterate over them below
                (void)part;
            }

            // Deduplication: key = (pos_idx << 40) | (nrm_idx << 20) | uv_idx
            std::unordered_map<uint64_t, uint32_t> cache;

            auto AddVertex = [&](uint32_t faceVertIdx) -> uint32_t
            {
                const uint32_t pi2 = fbxMesh->vertex_position.indices.data[faceVertIdx];
                const uint32_t ni2 = fbxMesh->vertex_normal.exists
                                   ? fbxMesh->vertex_normal.indices.data[faceVertIdx] : 0;
                const uint32_t ui2 = fbxMesh->vertex_uv.exists
                                   ? fbxMesh->vertex_uv.indices.data[faceVertIdx] : 0;

                const uint64_t key = ((uint64_t)pi2 << 40) | ((uint64_t)ni2 << 20) | ui2;
                auto it = cache.find(key);
                if (it != cache.end()) return it->second;

                Vertex v;
                const ufbx_vec3& p = fbxMesh->vertex_position.values.data[pi2];
                if (hasRotation)
                    v.Position = rotVec(nodeRot, (float)p.x, (float)p.y, (float)p.z);
                else
                    v.Position = {(float)p.x, (float)p.y, (float)p.z};

                if (fbxMesh->vertex_normal.exists)
                {
                    const ufbx_vec3& n = fbxMesh->vertex_normal.values.data[ni2];
                    if (hasRotation)
                        v.Normal = rotVec(nodeRot, (float)n.x, (float)n.y, (float)n.z);
                    else
                        v.Normal = {(float)n.x, (float)n.y, (float)n.z};
                }
                else
                {
                    v.Normal = {0.0f, 1.0f, 0.0f};
                }

                if (fbxMesh->vertex_uv.exists)
                {
                    const ufbx_vec2& uv = fbxMesh->vertex_uv.values.data[ui2];
                    v.UV = {(float)uv.x, 1.0f - (float)uv.y, 0.0f};
                }

                if (fbxMesh->vertex_tangent.exists)
                {
                    const uint32_t ti2 = fbxMesh->vertex_tangent.indices.data[faceVertIdx];
                    const ufbx_vec3& t = fbxMesh->vertex_tangent.values.data[ti2];
                    if (hasRotation)
                        v.Tangent = rotVec(nodeRot, (float)t.x, (float)t.y, (float)t.z);
                    else
                        v.Tangent = {(float)t.x, (float)t.y, (float)t.z};
                }

                // Skin weights: find the vertex in each skin deformer and
                // store up to 4 bone indices + weights.
                if (hasSkin && fbxMesh->skin_deformers.count > 0)
                {
                    ufbx_skin_deformer* skin = fbxMesh->skin_deformers.data[0];
                    // pi2 is the position-vertex index (ufbx "vertex" index)
                    if (pi2 < skin->vertices.count)
                    {
                        const ufbx_skin_vertex& sv = skin->vertices.data[pi2];
                        uint32_t count = sv.num_weights < 4 ? sv.num_weights : 4;
                        float totalW = 0.0f;
                        for (uint32_t wi = 0; wi < count; ++wi)
                        {
                            const ufbx_skin_weight& sw = skin->weights.data[sv.weight_begin + wi];
                            // Map cluster index -> bone index in our skeleton
                            if (sw.cluster_index < skin->clusters.count)
                            {
                                ufbx_skin_cluster* cl = skin->clusters.data[sw.cluster_index];
                                if (cl->bone_node)
                                {
                                    auto bit = loadedSkeleton.BoneNameToIndex.find(cl->bone_node->name.data);
                                    if (bit != loadedSkeleton.BoneNameToIndex.end())
                                        v.BoneIndices[wi] = bit->second;
                                }
                            }
                            v.BoneWeights[wi] = (float)sw.weight;
                            totalW += v.BoneWeights[wi];
                        }
                        // Normalize weights
                        if (totalW > 0.0f)
                            for (uint32_t wi = 0; wi < count; ++wi)
                                v.BoneWeights[wi] /= totalW;
                    }
                }

                const uint32_t idx = (uint32_t)(allVertices.size() - sub.BaseVertex);
                allVertices.push_back(v);
                cache[key] = idx;
                return idx;
            };

            // Triangulate each face and emit indices
            if (fbxMesh->material_parts.count > 0)
            {
                const ufbx_mesh_part& part = fbxMesh->material_parts.data[pi];
                for (size_t fi = 0; fi < part.face_indices.count; ++fi)
                {
                    const ufbx_face& face = fbxMesh->faces.data[part.face_indices.data[fi]];
                    const uint32_t nTris  = ufbx_triangulate_face(
                        triIndices.data(), maxTriIndices, fbxMesh, face);
                    for (uint32_t t = 0; t < nTris * 3; ++t)
                        sub.Indices.push_back(AddVertex(triIndices[t]) + sub.BaseVertex);
                }
            }
            else
            {
                for (size_t fi = faceBegin; fi < faceEnd; ++fi)
                {
                    const ufbx_face& face = fbxMesh->faces.data[fi];
                    const uint32_t nTris  = ufbx_triangulate_face(
                        triIndices.data(), maxTriIndices, fbxMesh, face);
                    for (uint32_t t = 0; t < nTris * 3; ++t)
                        sub.Indices.push_back(AddVertex(triIndices[t]) + sub.BaseVertex);
                }
            }

            if (sub.Indices.empty()) continue;

            // Helper: try every reasonable path to resolve an FBX texture reference,
            // including embedded content blobs (GLB-style FBX) as a last resort.
            auto LoadFBXTexture = [&](ufbx_texture* tex) -> unsigned int
            {
                if (!tex) return 0;

                // Build a list of candidate paths, from most to least reliable.
                std::vector<std::string> candidates;

                // 1. ufbx-resolved path (relative to loaded file)
                if (tex->filename.length > 0)
                {
                    candidates.push_back(tex->filename.data);

                    // Also try just the basename combined with the FBX directory.
                    // FBX files commonly embed absolute paths from the artist's machine
                    // (e.g. "C:\Users\artist\textures\rock.png") which won't resolve on
                    // another machine, but the texture sits next to the .fbx file.
                    std::string absPath = tex->filename.data;
                    size_t sl = absPath.find_last_of("/\\");
                    if (sl != std::string::npos)
                        candidates.push_back(dir + absPath.substr(sl + 1));
                }

                // 2. Directory of FBX + relative_filename as stored in the file
                if (tex->relative_filename.length > 0)
                {
                    candidates.push_back(dir + tex->relative_filename.data);

                    // 3. Just the basename — handles absolute artist-machine paths
                    //    like "C:\Users\artist\textures\rock.png"
                    std::string rel = tex->relative_filename.data;
                    size_t slash = rel.find_last_of("/\\");
                    if (slash != std::string::npos)
                        candidates.push_back(dir + rel.substr(slash + 1));
                }

                for (const auto& p : candidates)
                {
                    unsigned int id = LoadTextureFromFile(p);
                    if (id != 0)
                    {
                        std::cout << "FBX: loaded texture from: " << p << "\n";
                        return id;
                    }
                }

                // 4. Embedded content blob (textures packed inside the FBX/GLB)
                if (tex->content.size > 0)
                {
                    int w, h, c;
                    unsigned char* data = stbi_load_from_memory(
                        reinterpret_cast<const unsigned char*>(tex->content.data),
                        static_cast<int>(tex->content.size), &w, &h, &c, 4);
                    if (data)
                    {
                        unsigned int texID;
                        glGenTextures(1, &texID);
                        glBindTexture(GL_TEXTURE_2D, texID);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        stbi_image_free(data);
                        std::cout << "FBX: loaded embedded texture (" << w << "x" << h << ")\n";
                        return texID;
                    }
                }

                std::cerr << "FBX: could not load texture '"
                          << (tex->filename.length > 0 ? tex->filename.data : tex->relative_filename.data)
                          << "'\n";
                return 0;
            };

                    // Material / texture
                    if (fbxMesh->material_parts.count > 0)
                    {
                        ufbx_material* mat = nullptr;
                        if (pi < node->materials.count)
                            mat = node->materials.data[pi];
                        else if (pi < fbxMesh->materials.count)
                            mat = fbxMesh->materials.data[pi];

                        if (mat)
                        {
                            sub.MaterialName = mat->name.data;

                            // Prefer whichever channel actually has a texture attached;
                            // fall back to a plain colour value.  Many DCC exporters set
                            // pbr.base_color.has_value to true with a black default while
                            // the real texture lives on fbx.diffuse_color — so we must
                            // check for textures first across both channels.
                            const ufbx_material_map* diffMap  = nullptr;
                            const ufbx_material_map* colorMap = nullptr;
                            if (mat->pbr.base_color.texture)
                                diffMap = &mat->pbr.base_color;
                            else if (mat->fbx.diffuse_color.texture)
                                diffMap = &mat->fbx.diffuse_color;
                            else if (mat->pbr.base_color.has_value)
                                colorMap = &mat->pbr.base_color;
                            else if (mat->fbx.diffuse_color.has_value)
                                colorMap = &mat->fbx.diffuse_color;

                            // Use the colour from whichever map is available
                            const ufbx_material_map* valSrc = diffMap ? diffMap : colorMap;
                            if (valSrc && valSrc->has_value)
                                sub.DiffuseColor = {(float)valSrc->value_vec4.x,
                                                    (float)valSrc->value_vec4.y,
                                                    (float)valSrc->value_vec4.z};

                            if (diffMap && diffMap->texture)
                            {
                                unsigned int texID = LoadFBXTexture(diffMap->texture);
                                if (texID != 0)
                                {
                                    sub.DiffuseTexture     = texID;
                                    sub.HasDiffuseTexture  = true;
                                    sub.DiffuseTexturePath = diffMap->texture->filename.data;
                                }
                            }

                            // Fallback: scan all textures connected to this material
                            // for any diffuse-like property (handles custom shaders and
                            // non-standard property names).
                            if (!sub.HasDiffuseTexture)
                            {
                                for (size_t ti = 0; ti < mat->textures.count; ++ti)
                                {
                                    const ufbx_material_texture& mt = mat->textures.data[ti];
                                    std::string prop(mt.material_prop.data, mt.material_prop.length);
                                    if (prop.find("Diffuse") != std::string::npos ||
                                        prop.find("diffuse") != std::string::npos ||
                                        prop.find("BaseColor") != std::string::npos ||
                                        prop.find("base_color") != std::string::npos ||
                                        prop.find("Color") != std::string::npos ||
                                        prop.find("Albedo") != std::string::npos)
                                    {
                                        unsigned int texID = LoadFBXTexture(mt.texture);
                                        if (texID != 0)
                                        {
                                            sub.DiffuseTexture     = texID;
                                            sub.HasDiffuseTexture  = true;
                                            sub.DiffuseTexturePath = mt.texture->filename.data;
                                            std::cout << "FBX: found diffuse via material prop '"
                                                      << prop << "'\n";
                                            break;
                                        }
                                    }
                                }
                            }

                            // Last resort: use the very first texture on the material
                            if (!sub.HasDiffuseTexture && mat->textures.count > 0)
                            {
                                const ufbx_material_texture& mt = mat->textures.data[0];
                                unsigned int texID = LoadFBXTexture(mt.texture);
                                if (texID != 0)
                                {
                                    sub.DiffuseTexture     = texID;
                                    sub.HasDiffuseTexture  = true;
                                    sub.DiffuseTexturePath = mt.texture->filename.data;
                                    std::cout << "FBX: used first available texture as diffuse\n";
                                }
                            }
                        }
                    }

            allSubMeshes.push_back(std::move(sub));
        }
    }

    ufbx_free_scene(scene);

    if (allVertices.empty())
    {
        std::cerr << "FBX: no geometry loaded from \"" << path << "\"\n";
        return false;
    }

    Vertices  = std::move(allVertices);
    SubMeshes = std::move(allSubMeshes);
    BaseVertices = Vertices;

    if (hasSkin && !loadedSkeleton.Bones.empty())
    {
        MeshSkeleton = std::move(loadedSkeleton);
        HasSkeleton = true;
    }

    Upload();
    CalculateBounds();

    std::cout << "FBX loaded: " << Vertices.size() << " vertices, "
              << SubMeshes.size() << " submeshes from \"" << path << "\"";
    if (HasSkeleton)
        std::cout << " (skinned, " << MeshSkeleton.Bones.size() << " bones)";
    std::cout << "\n";
    return true;
}
