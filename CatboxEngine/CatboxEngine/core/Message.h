#pragma once
#include <string>
#include <memory>

enum class MessageType
{
    None = 0,
    EntityCreated,
    EntityDestroyed,
    MeshLoaded,
    MeshLoadFailed,
    TextureLoaded,
    TextureLoadFailed,
    ModelDropped,
    TextureDropped
};

// Base message class
class Message
{
public:
    virtual ~Message() = default;
    virtual MessageType GetType() const = 0;
    virtual std::string GetName() const = 0;
};

// Entity messages
class EntityCreatedMessage : public Message
{
public:
    int entityIndex;
    std::string entityName;

    EntityCreatedMessage(int idx, const std::string& name)
        : entityIndex(idx), entityName(name) {}

    MessageType GetType() const override { return MessageType::EntityCreated; }
    std::string GetName() const override { return "EntityCreated"; }
};

class EntityDestroyedMessage : public Message
{
public:
    int entityIndex;
    std::string entityName;

    EntityDestroyedMessage(int idx, const std::string& name)
        : entityIndex(idx), entityName(name) {}

    MessageType GetType() const override { return MessageType::EntityDestroyed; }
    std::string GetName() const override { return "EntityDestroyed"; }
};

// Mesh messages
class MeshLoadedMessage : public Message
{
public:
    std::string path;
    uint32_t handle;

    MeshLoadedMessage(const std::string& p, uint32_t h)
        : path(p), handle(h) {}

    MessageType GetType() const override { return MessageType::MeshLoaded; }
    std::string GetName() const override { return "MeshLoaded"; }
};

class MeshLoadFailedMessage : public Message
{
public:
    std::string path;
    std::string error;

    MeshLoadFailedMessage(const std::string& p, const std::string& err)
        : path(p), error(err) {}

    MessageType GetType() const override { return MessageType::MeshLoadFailed; }
    std::string GetName() const override { return "MeshLoadFailed"; }
};

// Texture messages
class TextureLoadedMessage : public Message
{
public:
    std::string path;
    int entityIndex;

    TextureLoadedMessage(const std::string& p, int idx)
        : path(p), entityIndex(idx) {}

    MessageType GetType() const override { return MessageType::TextureLoaded; }
    std::string GetName() const override { return "TextureLoaded"; }
};

class TextureLoadFailedMessage : public Message
{
public:
    std::string path;
    std::string error;

    TextureLoadFailedMessage(const std::string& p, const std::string& err)
        : path(p), error(err) {}

    MessageType GetType() const override { return MessageType::TextureLoadFailed; }
    std::string GetName() const override { return "TextureLoadFailed"; }
};

// Drop messages
class ModelDroppedMessage : public Message
{
public:
    std::string path;

    ModelDroppedMessage(const std::string& p) : path(p) {}

    MessageType GetType() const override { return MessageType::ModelDropped; }
    std::string GetName() const override { return "ModelDropped"; }
};

class TextureDroppedMessage : public Message
{
public:
    std::string path;

    TextureDroppedMessage(const std::string& p) : path(p) {}

    MessageType GetType() const override { return MessageType::TextureDropped; }
    std::string GetName() const override { return "TextureDropped"; }
};
