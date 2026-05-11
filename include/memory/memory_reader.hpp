#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "game/game_structures.hpp"

/**
 * Memory reading interface for accessing game memory
 */
class MemoryReader {
public:
    MemoryReader() = default;
    virtual ~MemoryReader() = default;
    
    /**
     * Read raw bytes from memory
     * @param address Virtual address to read from
     * @param buffer Output buffer
     * @param size Number of bytes to read
     * @return Number of bytes actually read
     */
    virtual size_t read_memory(uintptr_t address, void* buffer, size_t size) = 0;
    
    /**
     * Read templated value from memory
     */
    template<typename T>
    T read(uintptr_t address) {
        T value = T();
        read_memory(address, &value, sizeof(T));
        return value;
    }
    
    /**
     * Read Vector3 from memory
     */
    Vector3 read_vector3(uintptr_t address) {
        return read<Vector3>(address);
    }
    
    /**
     * Read Matrix4x4 from memory
     */
    Matrix4x4 read_matrix(uintptr_t address) {
        return read<Matrix4x4>(address);
    }
    
protected:
    pid_t process_id = 0;
};

/**
 * Platform-specific Windows implementation
 */
#ifdef _WIN32
class WindowsMemoryReader : public MemoryReader {
public:
    explicit WindowsMemoryReader(const wchar_t* process_name);
    ~WindowsMemoryReader() override;
    
    size_t read_memory(uintptr_t address, void* buffer, size_t size) override;
    uintptr_t get_module_base(const wchar_t* module_name);
    
private:
    void* process_handle = nullptr;
};
#endif

/**
 * Platform-specific Linux implementation
 */
#ifdef __linux__
#include "memory/signature_scanner.hpp"

class LinuxMemoryReader : public MemoryReader {
public:
    explicit LinuxMemoryReader(pid_t pid);
    explicit LinuxMemoryReader(const char* process_name);
    ~LinuxMemoryReader() override;
    
    size_t read_memory(uintptr_t address, void* buffer, size_t size) override;
    
    /**
     * Try to find offsets dynamically using signature scanning
     */
    bool find_offsets();
    
    /**
     * Get current entity list offset (may be dynamically found)
     */
    uintptr_t get_entity_list_offset() const { return entity_list_offset; }
    
    /**
     * Get current local player offset (may be dynamically found)
     */
    uintptr_t get_local_player_offset() const { return local_player_offset; }
    
private:
    int mem_fd = -1;
    SignatureScannerPtr scanner;
    
    // Dynamic offsets found by scanning
    uintptr_t entity_list_offset = 0;
    uintptr_t local_player_offset = 0;
    
    /**
     * Helper to find process ID by name
     */
    static pid_t find_process_by_name(const char* process_name);
};
#endif

/**
 * Mock memory reader for testing/demonstration
 */
class MockMemoryReader : public MemoryReader {
public:
    MockMemoryReader();
    size_t read_memory(uintptr_t address, void* buffer, size_t size) override;
};

using MemoryReaderPtr = std::shared_ptr<MemoryReader>;
