#include "Mesh.h"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "glfw3.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
// image loading: simple BMP loader implemented in LoadTextureFromFile below

// Minimal OBJ loader supporting positions, normals and triangular faces
bool Mesh::LoadFromOBJ(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open())
        return false;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    struct PackedVert { int p, n; };
    std::vector<uint32_t> outIndices;
    std::vector<Vertex> outVerts;

    std::unordered_map<uint64_t, uint32_t> cache;

    auto packKey = [](int p, int n) {
        return (uint64_t((uint32_t)p) << 32) | uint64_t((uint32_t)n);
    };

    std::string line;
    std::string currentMtl;
    std::unordered_map<std::string, Vec3> mtlColors;
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
            // supports formats like v//vn or v/vt/vn or v
            std::string a,b,c;
            ss >> a >> b >> c;
            std::string arr[3] = {a,b,c};
            for (int i=0;i<3;i++)
            {
                std::string token = arr[i];
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

                uint64_t key = packKey(vi, ni);
                auto it = cache.find(key);
                if (it != cache.end())
                {
                    outIndices.push_back(it->second);
                }
                else
                {
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
                    outIndices.push_back(newIndex);
                }
            }
        }
    }

    if (outVerts.empty() || outIndices.empty())
        return false;

    Vertices = std::move(outVerts);
    Indices = std::move(outIndices);

    // apply material color if found
    if (!currentMtl.empty())
    {
        auto itc = mtlColors.find(currentMtl);
        if (itc != mtlColors.end())
        {
            DiffuseColor = { itc->second.x, itc->second.y, itc->second.z };
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
    // very minimal BMP loader (supports 24-bit BMP)
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return 0;

    unsigned char header[54];
    in.read((char*)header, 54);
    int dataPos = *(int*)&(header[0x0A]);
    int imageSize = *(int*)&(header[0x22]);
    int width = *(int*)&(header[0x12]);
    int height = *(int*)&(header[0x16]);

    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;

    std::vector<unsigned char> data(imageSize);
    in.seekg(dataPos);
    in.read((char*)data.data(), imageSize);
    in.close();

    // BMP stores BGR; convert to RGB
    for (int i = 0; i < imageSize; i += 3)
        std::swap(data[i], data[i+2]);

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    glBindVertexArray(0);
}

bool Mesh::LoadTexture(const std::string& path)
{
    unsigned int tex = LoadTextureFromFile(path);
    if (tex == 0) return false;
    DiffuseTexture = tex;
    HasDiffuseTexture = true;
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

void Mesh::Draw() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)Indices.size(), GL_UNSIGNED_INT, 0);
}
