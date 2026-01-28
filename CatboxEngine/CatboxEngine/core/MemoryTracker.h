#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <cstddef>

// Memory allocation info
struct AllocationInfo
{
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
};

class MemoryTracker
{
public:
    static MemoryTracker& Instance()
    {
        static MemoryTracker instance;
        return instance;
    }

    // Track allocation
    void RecordAllocation(void* ptr, size_t size, const char* file, int line, const char* func);
    
    // Track deallocation
    void RecordDeallocation(void* ptr);
    
    // Report current memory usage
    void PrintMemoryReport() const;
    
    // Check for leaks (call at shutdown)
    void CheckForLeaks() const;
    
    // Get statistics
    size_t GetTotalAllocated() const { return m_totalAllocated; }
    size_t GetCurrentUsage() const { return m_currentUsage; }
    size_t GetAllocationCount() const { return m_allocationCount; }
    size_t GetDeallocationCount() const { return m_deallocationCount; }
    size_t GetActiveAllocations() const;
    
    // Clear all tracking (use with caution)
    void Clear();

private:
    MemoryTracker() = default;
    ~MemoryTracker() = default;
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<void*, AllocationInfo> m_allocations;
    
    size_t m_totalAllocated = 0;
    size_t m_currentUsage = 0;
    size_t m_allocationCount = 0;
    size_t m_deallocationCount = 0;
};

// Helper macros for tracking
#ifdef _DEBUG
    #define TRACK_MEMORY 1
#else
    #define TRACK_MEMORY 0
#endif

#if TRACK_MEMORY
    #define TRACKED_NEW(type) \
        MemoryTracker::Instance().RecordAllocation( \
            new type, sizeof(type), __FILE__, __LINE__, __FUNCTION__), \
        new type
    
    #define TRACKED_DELETE(ptr) \
        do { \
            MemoryTracker::Instance().RecordDeallocation(ptr); \
            delete ptr; \
        } while(0)
    
    #define TRACKED_NEW_ARRAY(type, count) \
        MemoryTracker::Instance().RecordAllocation( \
            new type[count], sizeof(type) * count, __FILE__, __LINE__, __FUNCTION__), \
        new type[count]
    
    #define TRACKED_DELETE_ARRAY(ptr) \
        do { \
            MemoryTracker::Instance().RecordDeallocation(ptr); \
            delete[] ptr; \
        } while(0)
#else
    #define TRACKED_NEW(type) new type
    #define TRACKED_DELETE(ptr) delete ptr
    #define TRACKED_NEW_ARRAY(type, count) new type[count]
    #define TRACKED_DELETE_ARRAY(ptr) delete[] ptr
#endif

// Memory scope tracker - RAII style leak detection
class MemoryScope
{
public:
    MemoryScope(const char* name);
    ~MemoryScope();
    
private:
    const char* m_name;
    size_t m_startAllocations;
    size_t m_startUsage;
};

#if TRACK_MEMORY
    #define MEMORY_SCOPE(name) MemoryScope __memScope(name)
#else
    #define MEMORY_SCOPE(name)
#endif
