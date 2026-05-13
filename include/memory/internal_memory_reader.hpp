#pragma once

#include "memory_reader.hpp"
#include <cstring>

/**
 * Memory reader for internal cheats (direct memory access)
 */
class InternalMemoryReader : public MemoryReader {
public:
    InternalMemoryReader() = default;
    
    bool read_memory(uintptr_t address, void* buffer, size_t size) override {
        if (address == 0 || buffer == nullptr) return false;
        
        // In an internal cheat, we can just use memcpy
        // Note: In some cases, we might want to use a SEH/signal handler 
        // to prevent crashing on invalid addresses, but for now we'll assume
        // the addresses are valid.
        std::memcpy(buffer, reinterpret_cast<void*>(address), size);
        return true;
    }
    
    template<typename T>
    T read(uintptr_t address) {
        T value;
        if (read_memory(address, &value, sizeof(T))) {
            return value;
        }
        return T();
    }
};

using InternalMemoryReaderPtr = std::shared_ptr<InternalMemoryReader>;
