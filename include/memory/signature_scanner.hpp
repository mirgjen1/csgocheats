#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

/**
 * Signature pattern scanner for finding offsets dynamically
 * Scans game memory for known byte patterns and returns addresses
 */
class SignatureScanner {
public:
    /**
     * Pattern definition with wildcards
     * Example: "48 8B 05 ?? ?? ?? ??" - IDA style
     */
    struct Pattern {
        std::string signature;
        int offset;        // Offset from pattern start to extract value
        bool relative;     // Whether the address is RIP-relative (common on x64)
        
        Pattern(const std::string& sig, int off = 0, bool rel = false)
            : signature(sig), offset(off), relative(rel) {}
    };

    /**
     * Module information parsed from /proc/pid/maps
     */
    struct ModuleInfo {
        uintptr_t base;
        uintptr_t end;
        size_t size;
        std::string path;
        std::string name;
    };
    
    explicit SignatureScanner(pid_t pid);
    
    /**
     * Load module information from /proc/pid/maps
     */
    bool load_modules();
    
    /**
     * Find pattern in a specific module
     */
    uintptr_t find_pattern(const std::string& module_name, const Pattern& pattern);

    /**
     * Get module by name
     */
    const ModuleInfo* get_module(const std::string& name) const;

    /**
     * Low-level pattern finding
     */
    uintptr_t find_pattern(const Pattern& pattern, uintptr_t start, size_t range);
    
    /**
     * Get all loaded modules
     */
    const std::vector<ModuleInfo>& get_all_modules() const { return modules; }

private:
    pid_t process_id;
    int mem_fd;
    std::vector<ModuleInfo> modules;
    
    // Read chunk of memory
    std::vector<uint8_t> read_memory_chunk(uintptr_t address, size_t size);
    
    // Compare bytes with pattern
    bool matches_pattern(const uint8_t* data, const std::string& signature, 
                        const std::string& mask);
};

using SignatureScannerPtr = std::shared_ptr<SignatureScanner>;
