#include "Mesh.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "glfw3.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
// image loading: use stb_image for PNG/JPEG/BMP
#define STB_IMAGE_IMPLEMENTATION
#include "../Dependencies/stb_image.h"
#include <cstring>

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

    // key must include position, texcoord and normal indices to avoid merging vertices that differ by UVs
    std::unordered_map<std::string, uint32_t> cache;

    auto packKey = [](int p, int t, int n) {
        return std::to_string(p) + ":" + std::to_string(t) + ":" + std::to_string(n);
    };

    std::string line;
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
            // attempt to parse mtl
            std::ifstream mtlin(mtlFile);
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
                    else if (mprefix == "map_Bump" || mprefix == "bump") { std::string tex; ms >> tex; if (!tex.empty()) mtlNormalMaps[cur] = tex; }
                }
        auto itnm = mtlNormalMaps.find(currentMtl);
        if (itnm != mtlNormalMaps.end())
        {
            std::string texPath = itnm->second;
            std::string objDir;
            size_t p = path.find_last_of("/\\");
            if (p != std::string::npos) objDir = path.substr(0, p+1);
            std::string full = texPath;
            if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
            if (LoadNormalTexture(full)) NormalTexturePath = full;
        }

// (Mesh::LoadTexture and UnloadTexture are implemented after LoadTextureFromFile)
            }
            continue;
        }
        if (prefix == "usemtl") { ss >> currentMtl; continue; }
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
                    vi = std::stoi(token) - 1;
                }
                else
                {
                    std::string viStr = token.substr(0, p1);
                    vi = viStr.empty() ? 0 : (std::stoi(viStr) - 1);
                    size_t p2 = token.find('/', p1+1);
                    if (p2!=std::string::npos)
                    {
                        std::string vtStr = token.substr(p1+1, p2 - (p1+1));
                        if (!vtStr.empty()) ti = std::stoi(vtStr) - 1;
                        std::string vnStr = token.substr(p2+1);
                        if (!vnStr.empty()) ni = std::stoi(vnStr) - 1;
                    }
                    else
                    {
                        std::string vtStr = token.substr(p1+1);
                        if (!vtStr.empty()) ti = std::stoi(vtStr) - 1;
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
                outIndices.push_back(i0);
                outIndices.push_back(i1);
                outIndices.push_back(i2);
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

    // apply material color if found
    if (!currentMtl.empty())
    {
        auto itc = mtlColors.find(currentMtl);
        if (itc != mtlColors.end()) DiffuseColor = { itc->second.x, itc->second.y, itc->second.z };

        auto its = mtlSpecular.find(currentMtl);
        if (its != mtlSpecular.end()) SpecularColor = its->second;

        auto itn = mtlShininess.find(currentMtl);
        if (itn != mtlShininess.end()) Shininess = itn->second;

        auto itd = mtlAlpha.find(currentMtl);
        if (itd != mtlAlpha.end()) Alpha = itd->second;

        auto itm = mtlMaps.find(currentMtl);
        if (itm != mtlMaps.end())
        {
            std::string texPath = itm->second;
            std::string objDir;
            size_t p = path.find_last_of("/\\");
            if (p != std::string::npos) objDir = path.substr(0, p+1);
            std::string full = texPath;
            if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
            if (LoadTexture(full)) DiffuseTexturePath = full;
        }
        auto itsm = mtlSpecularMaps.find(currentMtl);
        if (itsm != mtlSpecularMaps.end())
        {
            std::string texPath = itsm->second;
            std::string objDir;
            size_t p = path.find_last_of("/\\");
            if (p != std::string::npos) objDir = path.substr(0, p+1);
            std::string full = texPath;
            if (objDir.size() && texPath.find_first_of("/\\") == std::string::npos) full = objDir + texPath;
            if (LoadSpecularTexture(full)) SpecularTexturePath = full;
        }
    }

    // Upload to GPU
    Upload();

    return true;
}

bool Mesh::LoadFromGLTF(const std::string& path)
{
    // glTF loader not implemented yet. Return false so caller can fallback to OBJ.
    (void)path;
    return false;
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
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        Vertices.size() * sizeof(Vertex),
        Vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        Indices.size() * sizeof(uint32_t),
        Indices.data(),
        GL_STATIC_DRAW
    );

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

    // Specular not stored in vertex, it's a material property

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
    glDrawElements(GL_TRIANGLES, (GLsizei)Indices.size(), GL_UNSIGNED_INT, 0);
}
