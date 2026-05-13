#pragma once

#include "memory/memory_reader.hpp"
#include <cstring>

/**
 * Memory reader for internal cheats (direct memory access)
 */
class InternalMemoryReader : public MemoryReader {
public:
    InternalMemoryReader() = default;
    
    size_t read_memory(uintptr_t address, void* buffer, size_t size) override {
        if (address == 0 || buffer == nullptr) return 0;
        
        // In an internal cheat, we can just use memcpy
        std::memcpy(buffer, reinterpret_cast<void*>(address), size);
        return size;
    }
};

using InternalMemoryReaderPtr = std::shared_ptr<InternalMemoryReader>;
