# Memory Tracking System

## Overview
The memory tracking system helps detect memory leaks, monitor allocations, and debug memory issues in CatBox Engine.

## Features

? **Allocation Tracking** - Records every allocation with file/line info  
? **Leak Detection** - Automatically detects leaks at shutdown  
? **Memory Reports** - View current memory usage and statistics  
? **Scope Tracking** - RAII-based leak detection for code scopes  
? **Thread-Safe** - Safe to use from multiple threads  
? **Debug Only** - Zero overhead in Release builds  

## Usage

### Automatic Tracking (Debug Mode)

Memory tracking is **automatically enabled** in Debug builds:

```cpp
#ifdef _DEBUG
    #define TRACK_MEMORY 1  // Automatic in Debug
#else
    #define TRACK_MEMORY 0  // Disabled in Release
#endif
```

### Manual Tracking (Optional)

Use tracked allocation macros for explicit tracking:

```cpp
// Instead of:
MyClass* obj = new MyClass();
delete obj;

// Use:
MyClass* obj = TRACKED_NEW(MyClass)();
TRACKED_DELETE(obj);

// Arrays:
int* arr = TRACKED_NEW_ARRAY(int, 100);
TRACKED_DELETE_ARRAY(arr);
```

### Memory Scopes

Track memory within a function scope:

```cpp
void LoadModel(const std::string& path)
{
    MEMORY_SCOPE("LoadModel");
    
    // ... your code ...
    
    // Automatically reports leaks when scope exits
}
```

**Output:**
```
[MEMORY SCOPE] Entering: LoadModel
[MEMORY SCOPE] Exiting: LoadModel
  Allocation delta: 0
  Memory delta: 0 bytes
```

If there are leaks:
```
[MEMORY SCOPE] Exiting: LoadModel
  Allocation delta: 5
  Memory delta: 1024 bytes
  ? Warning: Scope leaked 5 allocation(s)!
```

### Memory Reports

#### In Code

```cpp
// Print current memory stats
MemoryTracker::Instance().PrintMemoryReport();

// Check for leaks
MemoryTracker::Instance().CheckForLeaks();

// Get statistics
size_t current = MemoryTracker::Instance().GetCurrentUsage();
size_t total = MemoryTracker::Instance().GetTotalAllocated();
size_t count = MemoryTracker::Instance().GetActiveAllocations();
```

#### In UI

The main UI window shows memory stats:
- **Current**: Memory currently in use
- **Total**: Total memory allocated since start
- **Allocations**: Number of active allocations

**Buttons:**
- `Print Report` - Print detailed report to console
- `Check Leaks` - Check for memory leaks now

## Console Output

### Memory Report

```
=== MEMORY REPORT ===
Total Allocated:          52428800 bytes
Current Usage:             5242880 bytes
Allocation Count:               150
Deallocation Count:             120
Active Allocations:              30

=== ACTIVE ALLOCATIONS ===
[0]       1024 bytes at 0x00007FF7F1234567 (Mesh.cpp:45 in LoadFromOBJ)
[1]       2048 bytes at 0x00007FF7F1234568 (Mesh.cpp:120 in Upload)
[2]        512 bytes at 0x00007FF7F1234569 (Entity.cpp:30 in AddEntity)
...
==================
```

### Leak Detection

```
!!! MEMORY LEAKS DETECTED !!!
Leaked allocations: 3
Leaked memory: 3584 bytes

=== LEAK DETAILS ===
[LEAK 0] 1024 bytes at 0x00007FF7F1234567
  Location: Mesh.cpp:45
  Function: LoadFromOBJ

[LEAK 1] 2048 bytes at 0x00007FF7F1234568
  Location: Mesh.cpp:120
  Function: Upload

[LEAK 2] 512 bytes at 0x00007FF7F1234569
  Location: Entity.cpp:30
  Function: AddEntity
===================
```

## Integration

### Engine Startup

Memory tracking is automatically initialized. At startup:

```cpp
Initial memory state:
=== MEMORY REPORT ===
Current Usage: 0 bytes
Active Allocations: 0
```

### Engine Shutdown

At shutdown, leaks are automatically detected:

```cpp
=== Engine Shutdown ===
? No memory leaks detected!
```

Or if leaks exist:

```cpp
=== Engine Shutdown ===
!!! MEMORY LEAKS DETECTED !!!
Leaked allocations: 5
...
```

## Common Use Cases

### 1. Finding Leaks in a Function

```cpp
void MyFunction()
{
    MEMORY_SCOPE("MyFunction");
    
    // Your code that might leak
    auto* data = new SomeData();
    
    // ... forgot to delete!
}
// Scope reports: ? Warning: Scope leaked 1 allocation(s)!
```

### 2. Tracking Resource Loading

```cpp
MeshHandle LoadMesh(const std::string& path)
{
    MEMORY_SCOPE("LoadMesh");
    
    size_t before = MemoryTracker::Instance().GetCurrentUsage();
    
    MeshHandle h = MeshManager::Instance().LoadMeshSync(path);
    
    size_t after = MemoryTracker::Instance().GetCurrentUsage();
    std::cout << "Mesh uses: " << (after - before) << " bytes" << std::endl;
    
    return h;
}
```

### 3. Periodic Leak Checks

```cpp
void Update(float deltaTime)
{
    static float timer = 0;
    timer += deltaTime;
    
    if (timer > 60.0f) // Every 60 seconds
    {
        MemoryTracker::Instance().CheckForLeaks();
        timer = 0;
    }
}
```

## Best Practices

### ? Do's

- **Use MEMORY_SCOPE** in functions that allocate memory
- **Check leaks periodically** during development
- **Use memory reports** to find allocation hotspots
- **Test with memory tracking** before releasing

### ? Don'ts

- **Don't track in hot loops** (performance impact)
- **Don't rely on Release builds** (tracking is disabled)
- **Don't ignore leak warnings** (fix them immediately)
- **Don't mix tracked/untracked** (leads to false positives)

## Performance Impact

### Debug Mode
- **Minimal overhead** (~5-10% slower)
- **Memory overhead** (~100 bytes per allocation)
- **Thread-safe** (mutex locks on track/untrack)

### Release Mode
- **Zero overhead** (all macros become no-ops)
- **No memory usage** (tracker not compiled)

## Troubleshooting

### False Positive: "Attempted to free untracked pointer"

**Cause:** Mixing tracked and untracked allocations

**Solution:** Use either all tracked or all untracked:
```cpp
// BAD:
auto* ptr = new MyClass();        // Untracked
TRACKED_DELETE(ptr);              // Tries to track free

// GOOD:
auto* ptr = TRACKED_NEW(MyClass)();
TRACKED_DELETE(ptr);
```

### Leak Reported But Memory Was Freed

**Cause:** Using regular `delete` on tracked allocation

**Solution:** Use `TRACKED_DELETE`:
```cpp
auto* ptr = TRACKED_NEW(MyClass)();
delete ptr;  // ? Tracker doesn't see this!

// Should be:
TRACKED_DELETE(ptr);  // ? Tracker records deallocation
```

### Memory Scope Always Shows Leaks

**Cause:** Memory is intentionally kept alive beyond scope

**Solution:** This is not a bug if the memory is stored elsewhere:
```cpp
void CreateEntity()
{
    MEMORY_SCOPE("CreateEntity");
    
    Entity* e = new Entity();
    entityManager.Add(e);  // Stored in manager, not a leak!
}
// Scope reports leak - this is OK if manager owns it
```

## Integration with External Libraries

For libraries that allocate internally (like ImGui, GLFW):

```cpp
// These are NOT tracked (external allocations)
ImGui::CreateContext();
glfwInit();

// Our allocations ARE tracked
Mesh* mesh = TRACKED_NEW(Mesh)();
```

This is expected - we only track our own allocations.

## Future Enhancements

- [ ] Memory profiler UI (real-time graphs)
- [ ] Allocation categorization (by type/system)
- [ ] Memory snapshots (compare before/after)
- [ ] Stack trace capture (full call stack for leaks)
- [ ] Custom allocators (pool, arena)
- [ ] Memory limits (fail if exceeded)

## Example Session

```
[MEMORY SCOPE] Entering: Engine::app
Initial memory state:
=== MEMORY REPORT ===
Current Usage: 0 bytes

... (loading models, creating entities) ...

Print Report clicked:
=== MEMORY REPORT ===
Total Allocated: 52428800 bytes
Current Usage: 5242880 bytes
Active Allocations: 150

... (gameplay) ...

[MEMORY SCOPE] Exiting: Engine::app
  Allocation delta: 0
  Memory delta: 0 bytes

=== Engine Shutdown ===
? No memory leaks detected!
```

Your memory is clean! ??
