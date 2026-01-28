#include "MemoryTracker.h"
#include <iostream>
#include <iomanip>

void MemoryTracker::RecordAllocation(void* ptr, size_t size, const char* file, int line, const char* func)
{
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AllocationInfo info;
    info.ptr = ptr;
    info.size = size;
    info.file = file;
    info.line = line;
    info.function = func;
    
    m_allocations[ptr] = info;
    m_totalAllocated += size;
    m_currentUsage += size;
    m_allocationCount++;
}

void MemoryTracker::RecordDeallocation(void* ptr)
{
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_allocations.find(ptr);
    if (it != m_allocations.end())
    {
        m_currentUsage -= it->second.size;
        m_allocations.erase(it);
        m_deallocationCount++;
    }
    else
    {
        std::cerr << "Warning: Attempted to free untracked pointer: " << ptr << std::endl;
    }
}

void MemoryTracker::PrintMemoryReport() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::cout << "\n=== MEMORY REPORT ===" << std::endl;
    std::cout << "Total Allocated:     " << std::setw(10) << m_totalAllocated << " bytes" << std::endl;
    std::cout << "Current Usage:       " << std::setw(10) << m_currentUsage << " bytes" << std::endl;
    std::cout << "Allocation Count:    " << std::setw(10) << m_allocationCount << std::endl;
    std::cout << "Deallocation Count:  " << std::setw(10) << m_deallocationCount << std::endl;
    std::cout << "Active Allocations:  " << std::setw(10) << m_allocations.size() << std::endl;
    
    if (!m_allocations.empty())
    {
        std::cout << "\n=== ACTIVE ALLOCATIONS ===" << std::endl;
        size_t index = 0;
        for (const auto& pair : m_allocations)
        {
            const auto& info = pair.second;
            std::cout << "[" << index++ << "] " 
                      << std::setw(10) << info.size << " bytes at " << info.ptr
                      << " (" << info.file << ":" << info.line << " in " << info.function << ")"
                      << std::endl;
            
            // Limit output to prevent spam
            if (index >= 20)
            {
                std::cout << "... and " << (m_allocations.size() - 20) << " more allocations" << std::endl;
                break;
            }
        }
    }
    
    std::cout << "==================\n" << std::endl;
}

void MemoryTracker::CheckForLeaks() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_allocations.empty())
    {
        std::cout << "? No memory leaks detected!" << std::endl;
        return;
    }
    
    std::cerr << "\n!!! MEMORY LEAKS DETECTED !!!" << std::endl;
    std::cerr << "Leaked allocations: " << m_allocations.size() << std::endl;
    std::cerr << "Leaked memory: " << m_currentUsage << " bytes" << std::endl;
    
    std::cerr << "\n=== LEAK DETAILS ===" << std::endl;
    size_t index = 0;
    for (const auto& pair : m_allocations)
    {
        const auto& info = pair.second;
        std::cerr << "[LEAK " << index++ << "] " 
                  << info.size << " bytes at " << info.ptr
                  << "\n  Location: " << info.file << ":" << info.line
                  << "\n  Function: " << info.function
                  << std::endl;
    }
    std::cerr << "===================\n" << std::endl;
}

size_t MemoryTracker::GetActiveAllocations() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_allocations.size();
}

void MemoryTracker::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_allocations.clear();
    m_totalAllocated = 0;
    m_currentUsage = 0;
    m_allocationCount = 0;
    m_deallocationCount = 0;
}

// MemoryScope implementation
MemoryScope::MemoryScope(const char* name)
    : m_name(name)
{
    m_startAllocations = MemoryTracker::Instance().GetActiveAllocations();
    m_startUsage = MemoryTracker::Instance().GetCurrentUsage();
    std::cout << "[MEMORY SCOPE] Entering: " << m_name << std::endl;
}

MemoryScope::~MemoryScope()
{
    size_t endAllocations = MemoryTracker::Instance().GetActiveAllocations();
    size_t endUsage = MemoryTracker::Instance().GetCurrentUsage();
    
    long long allocDelta = (long long)endAllocations - (long long)m_startAllocations;
    long long usageDelta = (long long)endUsage - (long long)m_startUsage;
    
    std::cout << "[MEMORY SCOPE] Exiting: " << m_name << std::endl;
    std::cout << "  Allocation delta: " << allocDelta << std::endl;
    std::cout << "  Memory delta: " << usageDelta << " bytes" << std::endl;
    
    if (allocDelta != 0)
    {
        std::cerr << "  ? Warning: Scope leaked " << allocDelta << " allocation(s)!" << std::endl;
    }
}
