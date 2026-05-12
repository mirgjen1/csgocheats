#include "memory/offset_finder.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

OffsetFinder::OffsetFinder(pid_t pid) : process_id(pid) {
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    mem_fd = open(mem_path, O_RDONLY);
    
    if (mem_fd == -1) {
        fprintf(stderr, "[OffsetFinder] Failed to open /proc/%d/mem\n", pid);
    }
}

OffsetFinder::~OffsetFinder() {
    if (mem_fd != -1) {
        close(mem_fd);
    }
}

std::vector<uint8_t> OffsetFinder::read_memory_chunk(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    
    if (mem_fd == -1) return buffer;
    if (lseek(mem_fd, address, SEEK_SET) == -1) return buffer;
    
    ssize_t bytes_read = ::read(mem_fd, buffer.data(), size);
    if (bytes_read <= 0) {
        buffer.clear();
    }
    
    return buffer;
}

std::vector<OffsetFinder::MemoryRange> OffsetFinder::get_memory_ranges() {
    // Read from /proc/[pid]/maps to get memory layout
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", process_id);
    
    FILE* maps_file = fopen(maps_path, "r");
    if (!maps_file) {
        fprintf(stderr, "[OffsetFinder] Failed to open /proc/%d/maps\n", process_id);
        return {};
    }
    
    std::vector<MemoryRange> ranges;
    char line[256];
    
    while (fgets(line, sizeof(line), maps_file)) {
        uintptr_t start, end;
        char perms[5];
        char pathname[256];
        
        // Parse: address perms offset dev inode pathname
        if (sscanf(line, "%lx-%lx %s %*x %*s %*d %s", &start, &end, perms, pathname) >= 3) {
            // Only include readable memory
            if (perms[0] == 'r') {
                ranges.push_back({start, end, std::string(pathname)});
            }
        }
    }
    
    fclose(maps_file);
    return ranges;
}

std::vector<OffsetFinder::FoundOffset> OffsetFinder::find_health_offsets(uint32_t health_value) {
    std::vector<FoundOffset> results;
    
    fprintf(stdout, "\n[OffsetFinder] Scanning for health value: %u\n", health_value);
    fprintf(stdout, "[OffsetFinder] This may take a minute...\n");
    
    auto ranges = get_memory_ranges();
    
    for (const auto& range : ranges) {
        // Skip very large regions for now
        size_t size = range.end - range.start;
        if (size > 100 * 1024 * 1024) continue;  // Skip regions > 100MB
        
        fprintf(stdout, "  Scanning %s (0x%lx - 0x%lx, %zu MB)\n", 
                range.name.c_str(), range.start, range.end, size / (1024 * 1024));
        
        // Scan in chunks
        const size_t chunk_size = 4 * 1024 * 1024;  // 4MB chunks
        
        for (uintptr_t addr = range.start; addr < range.end; addr += chunk_size) {
            size_t read_size = std::min(chunk_size, (size_t)(range.end - addr));
            std::vector<uint8_t> chunk = read_memory_chunk(addr, read_size);
            
            if (chunk.empty()) continue;
            
            // Search for the health value (as 32-bit uint)
            for (size_t i = 0; i < chunk.size() - sizeof(uint32_t); ++i) {
                uint32_t* value_ptr = (uint32_t*)&chunk[i];
                if (*value_ptr == health_value) {
                    uintptr_t found_addr = addr + i;
                    results.push_back({found_addr, health_value, "Possible health"});
                }
            }
        }
    }
    
    fprintf(stdout, "[OffsetFinder] Found %zu possible locations\n\n", results.size());
    return results;
}

std::vector<OffsetFinder::FoundOffset> OffsetFinder::narrow_results(
    const std::vector<FoundOffset>& previous_results,
    uint32_t new_health_value) {
    
    std::vector<FoundOffset> narrowed;
    
    fprintf(stdout, "[OffsetFinder] Narrowing from %zu results...\n", previous_results.size());
    
    for (const auto& result : previous_results) {
        uint32_t current_value = verify_offset(result.address);
        
        if (current_value == new_health_value) {
            narrowed.push_back({result.address, new_health_value, "Confirmed match"});
        }
    }
    
    fprintf(stdout, "[OffsetFinder] Narrowed to %zu results\n\n", narrowed.size());
    return narrowed;
}

uint32_t OffsetFinder::verify_offset(uintptr_t offset) {
    auto chunk = read_memory_chunk(offset, sizeof(uint32_t));
    if (chunk.size() < sizeof(uint32_t)) return 0;
    
    return *(uint32_t*)chunk.data();
}

void OffsetFinder::print_results(const std::vector<FoundOffset>& results) {
    if (results.empty()) {
        fprintf(stdout, "[OffsetFinder] No results to display\n");
        return;
    }
    
    fprintf(stdout, "\n=== FOUND OFFSETS ===\n");
    fprintf(stdout, "Address         Value   Description\n");
    fprintf(stdout, "=========================================\n");
    
    for (size_t i = 0; i < std::min(size_t(20), results.size()); ++i) {
        const auto& result = results[i];
        fprintf(stdout, "0x%016lx  %5u   %s\n", 
                result.address, result.value, result.description.c_str());
    }
    
    if (results.size() > 20) {
        fprintf(stdout, "... and %zu more\n", results.size() - 20);
    }
    
    fprintf(stdout, "\n");
}

uintptr_t OffsetFinder::get_module_base(const std::string& module_name) {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", process_id);
    
    FILE* maps_file = fopen(maps_path, "r");
    if (!maps_file) return 0;
    
    char line[256];
    while (fgets(line, sizeof(line), maps_file)) {
        uintptr_t start;
        char pathname[256];
        
        if (sscanf(line, "%lx-%*x %*s %*x %*s %*d %s", &start, pathname) >= 2) {
            if (strstr(pathname, module_name.c_str())) {
                fclose(maps_file);
                return start;
            }
        }
    }
    
    fclose(maps_file);
    return 0;
}
