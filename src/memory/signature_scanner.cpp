#include "memory/signature_scanner.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

SignatureScanner::SignatureScanner(pid_t pid) : process_id(pid) {
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    mem_fd = open(mem_path, O_RDONLY);
    
    if (mem_fd == -1) {
        fprintf(stderr, "[SignatureScanner] Failed to open /proc/%d/mem\n", pid);
    }
}

std::vector<uint8_t> SignatureScanner::read_memory_chunk(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    
    if (mem_fd == -1) {
        return buffer;
    }
    
    if (lseek(mem_fd, address, SEEK_SET) == -1) {
        return buffer;
    }
    
    ssize_t bytes_read = ::read(mem_fd, buffer.data(), size);
    if (bytes_read <= 0) {
        buffer.clear();
        return buffer;
    }
    
    return buffer;
}

bool SignatureScanner::matches_pattern(const uint8_t* data, const std::string& signature, 
                                       const std::string& mask) {
    // Parse signature string into bytes
    std::vector<uint8_t> sig_bytes;
    size_t i = 0;
    while (i < signature.length()) {
        while (i < signature.length() && signature[i] == ' ') i++;
        if (i >= signature.length()) break;
        
        char byte_str[3];
        byte_str[0] = signature[i++];
        byte_str[1] = signature[i++];
        byte_str[2] = '\0';
        
        uint8_t byte = strtol(byte_str, nullptr, 16);
        sig_bytes.push_back(byte);
    }
    
    if (sig_bytes.size() != mask.length()) {
        return false;
    }
    
    for (size_t j = 0; j < sig_bytes.size(); ++j) {
        if (mask[j] == 'x' && data[j] != sig_bytes[j]) {
            return false;
        }
    }
    
    return true;
}

uintptr_t SignatureScanner::find_pattern(const Pattern& pattern, uintptr_t start, size_t range) {
    if (mem_fd == -1) {
        fprintf(stderr, "[SignatureScanner] Memory file not open\n");
        return 0;
    }
    
    // Default range: 500MB from start if not specified
    if (range == 0) {
        range = 500 * 1024 * 1024;
    }
    
    const size_t chunk_size = 1024 * 1024;  // 1MB chunks
    const size_t overlap = pattern.mask.length() + 100;
    
    fprintf(stdout, "[SignatureScanner] Scanning for pattern starting at 0x%lx, range 0x%lx\n", 
            start, range);
    
    for (size_t offset = 0; offset < range; offset += chunk_size) {
        uintptr_t chunk_addr = start + offset;
        std::vector<uint8_t> chunk = read_memory_chunk(chunk_addr, chunk_size + overlap);
        
        if (chunk.empty()) {
            continue;
        }
        
        // Search within chunk
        for (size_t i = 0; i < chunk.size() - pattern.mask.length(); ++i) {
            if (matches_pattern(&chunk[i], pattern.signature, pattern.mask)) {
                uintptr_t found_addr = chunk_addr + i + pattern.offset;
                fprintf(stdout, "[SignatureScanner] Pattern found at 0x%lx\n", found_addr);
                return found_addr;
            }
        }
        
        // Show progress
        if (offset % (50 * 1024 * 1024) == 0) {
            fprintf(stdout, "[SignatureScanner] Scanned 0x%lx / 0x%lx bytes...\n", offset, range);
        }
    }
    
    fprintf(stderr, "[SignatureScanner] Pattern not found\n");
    return 0;
}

uintptr_t SignatureScanner::read_pattern(const Pattern& pattern, uintptr_t start, size_t range) {
    uintptr_t pattern_addr = find_pattern(pattern, start, range);
    if (pattern_addr == 0) {
        return 0;
    }
    
    // Read pointer from pattern location
    std::vector<uint8_t> bytes = read_memory_chunk(pattern_addr, sizeof(uintptr_t));
    if (bytes.size() < sizeof(uintptr_t)) {
        return 0;
    }
    
    return *reinterpret_cast<uintptr_t*>(bytes.data());
}
