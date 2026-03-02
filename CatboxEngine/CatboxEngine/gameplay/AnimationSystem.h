#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

static constexpr int MAX_BONES = 128;

// A single bone in the skeleton hierarchy
struct Bone
{
    std::string Name;
    int ParentIndex = -1;
    glm::mat4 InverseBindPose = glm::mat4(1.0f);
    glm::mat4 LocalBindTransform = glm::mat4(1.0f);
};

// Skeleton: ordered list of bones + name lookup
struct Skeleton
{
    std::vector<Bone> Bones;
    std::unordered_map<std::string, int> BoneNameToIndex;

    // Mesh-node transform that converts geometry space to node space.
    // Needed for skinned FBX meshes that have a pre-rotation (e.g. Mixamo -90° X).
    glm::mat4 MeshNodeTransform = glm::mat4(1.0f);

    int FindBone(const std::string& name) const
    {
        auto it = BoneNameToIndex.find(name);
        return it != BoneNameToIndex.end() ? it->second : -1;
    }
};

// Sampled pose for one bone at one point in time
struct BoneSample
{
    glm::vec3 Position { 0.0f };
    glm::quat Rotation { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale    { 1.0f };
};

// All bone poses at one point in time
struct AnimationFrame
{
    float Time = 0.0f;
    std::vector<BoneSample> BonePoses; // indexed by bone index
};

// An animation clip loaded from an FBX file (pre-sampled)
struct AnimationClip
{
    std::string Name;
    std::string FilePath;
    float Duration = 0.0f;
    bool Loop = true;
    std::vector<AnimationFrame> Frames; // sampled at fixed intervals
    float SampleRate = 30.0f;

    bool LoadFromFBX(const std::string& path, const Skeleton& skeleton);
};

// Plays an AnimationClip and produces final bone matrices
class AnimationPlayer
{
public:
    void SetClip(AnimationClip* clip);
    void Update(float deltaTime);
    void ComputeBoneMatrices(const Skeleton& skeleton, std::vector<glm::mat4>& outMatrices) const;

    AnimationClip* GetCurrentClip() const { return m_currentClip; }
    float GetTime() const { return m_currentTime; }
    bool IsPlaying() const { return m_playing; }
    void Play() { m_playing = true; }
    void Stop() { m_playing = false; m_currentTime = 0.0f; }
    void SetSpeed(float speed) { m_speed = speed; }

private:
    AnimationClip* m_currentClip = nullptr;
    float m_currentTime = 0.0f;
    float m_speed = 1.0f;
    bool m_playing = true;
};
