#include "AnimationSystem.h"
#include "../Dependencies/ufbx.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helper: convert ufbx_matrix (column-major 4x3) to glm::mat4
// ---------------------------------------------------------------------------
static glm::mat4 UfbxToGlm(const ufbx_matrix& m)
{
    glm::mat4 r(1.0f);
    r[0][0] = (float)m.cols[0].x; r[0][1] = (float)m.cols[0].y; r[0][2] = (float)m.cols[0].z; r[0][3] = 0.0f;
    r[1][0] = (float)m.cols[1].x; r[1][1] = (float)m.cols[1].y; r[1][2] = (float)m.cols[1].z; r[1][3] = 0.0f;
    r[2][0] = (float)m.cols[2].x; r[2][1] = (float)m.cols[2].y; r[2][2] = (float)m.cols[2].z; r[2][3] = 0.0f;
    r[3][0] = (float)m.cols[3].x; r[3][1] = (float)m.cols[3].y; r[3][2] = (float)m.cols[3].z; r[3][3] = 1.0f;
    return r;
}

// ---------------------------------------------------------------------------
// AnimationClip::LoadFromFBX
//
// Opens an FBX file, takes the first anim stack and pre-samples bone
// transforms at SampleRate fps.  The skeleton reference tells us which
// bones to track and in what order.
// ---------------------------------------------------------------------------
bool AnimationClip::LoadFromFBX(const std::string& path, const Skeleton& skeleton)
{
    if (skeleton.Bones.empty())
    {
        std::cerr << "AnimationClip: skeleton is empty, cannot load " << path << "\n";
        return false;
    }

    ufbx_load_opts opts = {};
    opts.target_axes        = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);
    if (!scene)
    {
        std::cerr << "AnimationClip: failed to load " << path << ": "
                  << error.description.data << "\n";
        return false;
    }

    if (scene->anim_stacks.count == 0)
    {
        std::cerr << "AnimationClip: no animation stacks in " << path << "\n";
        ufbx_free_scene(scene);
        return false;
    }

    ufbx_anim_stack* stack = scene->anim_stacks.data[0];
    double tBegin = stack->time_begin;
    double tEnd   = stack->time_end;
    Duration = (float)(tEnd - tBegin);
    if (Duration <= 0.0f) Duration = 0.001f;
    Name = stack->name.length > 0 ? stack->name.data : "clip";
    FilePath = path;

    double step = 1.0 / (double)SampleRate;
    size_t boneCount = skeleton.Bones.size();

    Frames.clear();

    for (double t = tBegin; t <= tEnd + step * 0.5; t += step)
    {
        double sampleTime = std::min(t, tEnd);

        ufbx_error evalError;
        ufbx_scene* eval = ufbx_evaluate_scene(scene, stack->anim, sampleTime, nullptr, &evalError);
        if (!eval) continue;

        AnimationFrame frame;
        frame.Time = (float)(sampleTime - tBegin);
        frame.BonePoses.resize(boneCount);

        for (size_t bi = 0; bi < boneCount; ++bi)
        {
            const std::string& boneName = skeleton.Bones[bi].Name;

            // Find the matching node in the evaluated scene
            ufbx_node* node = nullptr;
            for (size_t ni = 0; ni < eval->nodes.count; ++ni)
            {
                if (boneName == eval->nodes.data[ni]->name.data)
                {
                    node = eval->nodes.data[ni];
                    break;
                }
            }

            BoneSample& sample = frame.BonePoses[bi];
            if (node)
            {
                const ufbx_transform& lt = node->local_transform;
                sample.Position = glm::vec3((float)lt.translation.x,
                                            (float)lt.translation.y,
                                            (float)lt.translation.z);
                sample.Rotation = glm::quat((float)lt.rotation.w,
                                            (float)lt.rotation.x,
                                            (float)lt.rotation.y,
                                            (float)lt.rotation.z);
                sample.Scale    = glm::vec3((float)lt.scale.x,
                                            (float)lt.scale.y,
                                            (float)lt.scale.z);
            }
            else
            {
                // No matching node — use identity
                sample.Position = glm::vec3(0.0f);
                sample.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                sample.Scale    = glm::vec3(1.0f);
            }
        }

        ufbx_free_scene(eval);
        Frames.push_back(std::move(frame));
    }

    ufbx_free_scene(scene);

    std::cout << "AnimationClip loaded: \"" << Name << "\" from " << path
              << " (" << Frames.size() << " frames, " << Duration << "s)\n";
    return true;
}

// ---------------------------------------------------------------------------
// AnimationPlayer
// ---------------------------------------------------------------------------
void AnimationPlayer::SetClip(AnimationClip* clip)
{
    if (m_currentClip == clip) return;
    m_currentClip = clip;
    m_currentTime = 0.0f;
    m_playing = true;
}

void AnimationPlayer::Update(float deltaTime)
{
    if (!m_currentClip || !m_playing) return;

    m_currentTime += deltaTime * m_speed;

    if (m_currentClip->Loop)
    {
        while (m_currentTime >= m_currentClip->Duration)
            m_currentTime -= m_currentClip->Duration;
        while (m_currentTime < 0.0f)
            m_currentTime += m_currentClip->Duration;
    }
    else
    {
        m_currentTime = glm::clamp(m_currentTime, 0.0f, m_currentClip->Duration);
    }
}

void AnimationPlayer::ComputeBoneMatrices(const Skeleton& skeleton,
                                          std::vector<glm::mat4>& outMatrices) const
{
    size_t boneCount = skeleton.Bones.size();
    outMatrices.resize(boneCount, glm::mat4(1.0f));

    if (!m_currentClip || m_currentClip->Frames.empty())
    {
        // Identity matrices — no animation
        return;
    }

    const auto& frames = m_currentClip->Frames;

    // Find surrounding frames for interpolation
    float frameF = m_currentTime * m_currentClip->SampleRate;
    int frame0 = (int)frameF;
    int frame1 = frame0 + 1;
    float alpha = frameF - (float)frame0;

    frame0 = glm::clamp(frame0, 0, (int)frames.size() - 1);
    frame1 = glm::clamp(frame1, 0, (int)frames.size() - 1);

    const AnimationFrame& f0 = frames[frame0];
    const AnimationFrame& f1 = frames[frame1];

    // Compute local transforms for each bone by interpolating sampled poses
    std::vector<glm::mat4> localTransforms(boneCount, glm::mat4(1.0f));

    for (size_t bi = 0; bi < boneCount; ++bi)
    {
        if (bi >= f0.BonePoses.size() || bi >= f1.BonePoses.size()) continue;

        const BoneSample& s0 = f0.BonePoses[bi];
        const BoneSample& s1 = f1.BonePoses[bi];

        glm::vec3 pos   = glm::mix(s0.Position, s1.Position, alpha);
        glm::quat rot   = glm::slerp(s0.Rotation, s1.Rotation, alpha);
        glm::vec3 scale = glm::mix(s0.Scale, s1.Scale, alpha);

        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 R = glm::mat4_cast(rot);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        localTransforms[bi] = T * R * S;
    }

    // Walk the hierarchy to compute world-space transforms, then multiply
    // by the inverse bind pose to get the final skinning matrix.
    // Finally, prepend the mesh node's local transform (e.g. the -90° X
    // pre-rotation that Mixamo/Blender exports bake into the mesh node)
    // so the model stands upright without needing an entity rotation that
    // would fight with the player controller.
    std::vector<glm::mat4> worldTransforms(boneCount, glm::mat4(1.0f));

    for (size_t bi = 0; bi < boneCount; ++bi)
    {
        int parent = skeleton.Bones[bi].ParentIndex;
        if (parent >= 0 && parent < (int)boneCount)
            worldTransforms[bi] = worldTransforms[parent] * localTransforms[bi];
        else
            worldTransforms[bi] = localTransforms[bi];

        outMatrices[bi] = skeleton.MeshNodeTransform
                        * worldTransforms[bi]
                        * skeleton.Bones[bi].InverseBindPose;
    }
}
