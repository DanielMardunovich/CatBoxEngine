# Message System Documentation

## Overview
The CatBox Engine includes a thread-safe message/event system for decoupling subsystems and enabling reactive programming patterns.

## Architecture

### Core Components

1. **Message.h** - Base message class and specific message types
2. **MessageQueue.h** - Thread-safe message queue with pub/sub pattern

### Message Types

```cpp
enum class MessageType
{
    EntityCreated,      // Posted when an entity is added
    EntityDestroyed,    // Posted when an entity is removed
    MeshLoaded,         // Posted when a mesh loads successfully
    MeshLoadFailed,     // Posted when mesh loading fails
    TextureLoaded,      // Posted when a texture loads
    TextureLoadFailed,  // Posted when texture loading fails
    ModelDropped,       // Posted when a model file is drag-dropped
    TextureDropped      // Posted when a texture file is drag-dropped
};
```

## Usage

### Subscribing to Messages

```cpp
// Subscribe to entity creation events
MessageQueue::Instance().Subscribe(MessageType::EntityCreated, 
    [](const Message& msg) {
        const auto& m = static_cast<const EntityCreatedMessage&>(msg);
        std::cout << "Entity created: " << m.entityName 
                  << " at index " << m.entityIndex << std::endl;
    });

// Subscribe to mesh loading events
MessageQueue::Instance().Subscribe(MessageType::MeshLoaded,
    [](const Message& msg) {
        const auto& m = static_cast<const MeshLoadedMessage&>(msg);
        std::cout << "Mesh loaded: " << m.path 
                  << " (handle: " << m.handle << ")" << std::endl;
    });
```

### Posting Messages

```cpp
// Post an entity created message
auto msg = std::make_shared<EntityCreatedMessage>(entityIndex, entityName);
MessageQueue::Instance().Post(msg);

// Post a mesh loaded message  
auto msg = std::make_shared<MeshLoadedMessage>(path, handle);
MessageQueue::Instance().Post(msg);
```

### Processing Messages

Messages are automatically processed each frame in `Engine::Update()`:

```cpp
void Engine::Update(float deltaTime)
{
    // ... other update code ...
    
    // Process all messages in the queue
    MessageQueue::Instance().ProcessMessages();
}
```

## Message Flow

1. **Event Occurs** ? System posts a message to the queue
2. **Queue Stores** ? Message is stored in thread-safe queue
3. **Frame Update** ? `ProcessMessages()` dispatches to subscribers
4. **Callbacks Execute** ? All subscribed callbacks receive the message

## Thread Safety

- `Post()` is thread-safe and can be called from any thread
- `ProcessMessages()` should only be called from the main thread
- Callbacks execute on the main thread during `ProcessMessages()`

## Current Integration Points

### EntityManager
- Posts `EntityCreatedMessage` when entities are added
- Posts `EntityDestroyedMessage` when entities are removed

### MeshManager  
- Posts `MeshLoadedMessage` when meshes load successfully
- Posts `MeshLoadFailedMessage` when mesh loading fails
- Works with both sync and async loading

### Engine
- Posts `ModelDroppedMessage` when model files are dropped
- Posts `TextureDroppedMessage` when texture files are dropped
- Posts `TextureLoadedMessage` when textures are assigned

## Extending the System

### Adding New Message Types

1. Add enum value to `MessageType` in `Message.h`
2. Create message class inheriting from `Message`:

```cpp
class YourCustomMessage : public Message
{
public:
    std::string yourData;
    int yourValue;

    YourCustomMessage(const std::string& data, int val)
        : yourData(data), yourValue(val) {}

    MessageType GetType() const override { return MessageType::YourCustom; }
    std::string GetName() const override { return "YourCustom"; }
};
```

3. Post messages where appropriate:
```cpp
auto msg = std::make_shared<YourCustomMessage>("data", 42);
MessageQueue::Instance().Post(msg);
```

4. Subscribe to handle them:
```cpp
MessageQueue::Instance().Subscribe(MessageType::YourCustom,
    [](const Message& msg) {
        const auto& m = static_cast<const YourCustomMessage&>(msg);
        // Handle your message
    });
```

## Benefits

- **Decoupling**: Systems don't need direct references to each other
- **Extensibility**: Easy to add new subscribers without modifying sources
- **Debugging**: Centralized message logging for debugging
- **Async-friendly**: Thread-safe posting from background threads
- **Reactive**: UI can react to system events automatically

## Performance Notes

- Messages are processed synchronously on the main thread
- Queue operations use mutex locks (minimal overhead)
- Consider batching if posting many messages per frame
- Messages are processed in FIFO order
