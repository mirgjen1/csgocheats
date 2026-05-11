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
     * Example: "55 8B EC 83 EC ?? 53 56" - ? is wildcard
     */
    struct Pattern {
        std::string signature;
        std::string mask;  // 'x' for match, '?' for wildcard
        int offset;        // Offset from pattern start to return
        
        Pattern(const std::string& sig, const std::string& m, int off = 0)
            : signature(sig), mask(m), offset(off) {}
    };
    
    explicit SignatureScanner(pid_t pid);
    
    /**
     * Scan memory for pattern
     * Returns address of pattern or 0 if not found
     */
    uintptr_t find_pattern(const Pattern& pattern, uintptr_t start = 0, size_t range = 0);
    
    /**
     * Scan for pattern and read offset
     */
    uintptr_t read_pattern(const Pattern& pattern, uintptr_t start = 0, size_t range = 0);
    
private:
    pid_t process_id;
    int mem_fd;
    
    // Read chunk of memory
    std::vector<uint8_t> read_memory_chunk(uintptr_t address, size_t size);
    
    // Compare bytes with pattern
    bool matches_pattern(const uint8_t* data, const std::string& signature, 
                        const std::string& mask);
};

using SignatureScannerPtr = std::shared_ptr<SignatureScanner>;
