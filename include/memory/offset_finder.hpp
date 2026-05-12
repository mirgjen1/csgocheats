#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>

/**
 * Automated offset finder tool
 * Scans game memory for known values and narrows down offsets
 */
class OffsetFinder {
public:
    struct FoundOffset {
        uintptr_t address;
        uint32_t value;
        std::string description;
    };
    
    explicit OffsetFinder(pid_t pid);
    ~OffsetFinder();
    
    /**
     * Find health offsets by value scanning
     * Returns list of addresses where health value was found
     */
    std::vector<FoundOffset> find_health_offsets(uint32_t health_value);
    
    /**
     * Narrow down results by comparing with new health value
     */
    std::vector<FoundOffset> narrow_results(
        const std::vector<FoundOffset>& previous_results,
        uint32_t new_health_value
    );
    
    /**
     * Verify offset by reading value
     */
    uint32_t verify_offset(uintptr_t offset);
    
    /**
     * Print all found offsets
     */
    void print_results(const std::vector<FoundOffset>& results);
    
    /**
     * Get module base for relative offset calculation
     */
    uintptr_t get_module_base(const std::string& module_name);
    
private:
    pid_t process_id;
    int mem_fd;
    
    std::vector<uint8_t> read_memory_chunk(uintptr_t address, size_t size);
    
    // Common memory ranges to scan
    struct MemoryRange {
        uintptr_t start;
        uintptr_t end;
        std::string name;
    };
    
    std::vector<MemoryRange> get_memory_ranges();
};
