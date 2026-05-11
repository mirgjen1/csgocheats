#include "memory/signature_scanner.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

#include <fstream>
#include <sstream>

SignatureScanner::SignatureScanner(pid_t pid) : process_id(pid) {
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    mem_fd = open(mem_path, O_RDONLY);
    
    if (mem_fd == -1) {
        fprintf(stderr, "[SignatureScanner] Failed to open /proc/%d/mem\n", pid);
    }
}

bool SignatureScanner::load_modules() {
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", process_id);
    
    std::ifstream maps(maps_path);
    if (!maps.is_open()) {
        fprintf(stderr, "[SignatureScanner] Failed to open %s\n", maps_path);
        return false;
    }
    
    modules.clear();
    std::string line;
    while (std::getline(maps, line)) {
        std::stringstream ss(line);
        std::string range, perms, offset, dev, inode, path;
        
        ss >> range >> perms >> offset >> dev >> inode >> path;
        
        if (path.empty() || path[0] == '[') continue;
        
        // Find if this path is already in modules
        size_t last_slash = path.find_last_of('/');
        std::string name = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
        
        size_t dash = range.find('-');
        uintptr_t start = std::stoull(range.substr(0, dash), nullptr, 16);
        uintptr_t end = std::stoull(range.substr(dash + 1), nullptr, 16);
        
        // Check if we already have this module (multiple mappings)
        bool found = false;
        for (auto& mod : modules) {
            if (mod.path == path) {
                if (start < mod.base) mod.base = start;
                if (end > mod.end) mod.end = end;
                mod.size = mod.end - mod.base;
                found = true;
                break;
            }
        }
        
        if (!found) {
            modules.push_back({start, end, (size_t)(end - start), path, name});
            // Optional: fprintf(stdout, "[SignatureScanner] Found module: %s (0x%lx)\n", name.c_str(), start);
        }
    }
    
    fprintf(stdout, "[SignatureScanner] Loaded %zu modules from maps\n", modules.size());
    return !modules.empty();
}

const SignatureScanner::ModuleInfo* SignatureScanner::get_module(const std::string& name) const {
    // First try exact basename match
    for (const auto& mod : modules) {
        if (mod.name == name) {
            return &mod;
        }
    }
    // Then try matching the full path ending (e.g. "csgo/bin/linux64/client_client.so")
    for (const auto& mod : modules) {
        // Only match if the name appears after a '/' to avoid partial matches
        size_t pos = mod.path.rfind("/" + name);
        if (pos != std::string::npos && pos + 1 + name.size() == mod.path.size()) {
            return &mod;
        }
    }
    return nullptr;
}

uintptr_t SignatureScanner::find_pattern(const std::string& module_name, const Pattern& pattern) {
    const ModuleInfo* mod = get_module(module_name);
    if (!mod) {
        fprintf(stderr, "[SignatureScanner] Module %s not found\n", module_name.c_str());
        return 0;
    }
    
    uintptr_t addr = find_pattern(pattern, mod->base, mod->size);
    if (addr == 0) return 0;
    
    // Extract address from instruction
    // In x64, many addresses are RIP-relative (offset from next instruction)
    if (pattern.relative) {
        int32_t rel_offset = 0;
        if (pread(mem_fd, &rel_offset, sizeof(int32_t), addr + pattern.offset) != sizeof(int32_t)) {
            return 0;
        }
        // RIP-relative address = Address of instruction + Offset to operand + Operand size + Relative Offset
        // Most common case: addr + pattern.offset + 4 + rel_offset
        uintptr_t result = addr + pattern.offset + 4 + rel_offset;
        fprintf(stdout, "[SignatureScanner] Resolved relative addr: 0x%lx (offset: 0x%x)\n", result, rel_offset);
        return result - mod->base; // Return offset from module base
    } else {
        uintptr_t absolute_addr = 0;
        if (pread(mem_fd, &absolute_addr, sizeof(uintptr_t), addr + pattern.offset) != sizeof(uintptr_t)) {
            return 0;
        }
        return absolute_addr - mod->base;
    }
}

std::vector<uint8_t> SignatureScanner::read_memory_chunk(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    if (mem_fd == -1) return buffer;
    
    ssize_t bytes_read = pread(mem_fd, buffer.data(), size, address);
    if (bytes_read <= 0) {
        buffer.clear();
    } else if ((size_t)bytes_read < size) {
        buffer.resize(bytes_read);
    }
    
    return buffer;
}

uintptr_t SignatureScanner::find_pattern(const Pattern& pattern, uintptr_t start, size_t range) {
    if (mem_fd == -1) return 0;
    
    // Convert IDA pattern to bytes and mask
    std::vector<uint8_t> sig_bytes;
    std::vector<bool> mask;
    std::stringstream ss(pattern.signature);
    std::string byte_str;
    
    while (ss >> byte_str) {
        if (byte_str == "?" || byte_str == "??") {
            sig_bytes.push_back(0);
            mask.push_back(false);
        } else {
            sig_bytes.push_back((uint8_t)std::stoul(byte_str, nullptr, 16));
            mask.push_back(true);
        }
    }
    
    const size_t chunk_size = 1024 * 1024; // 1MB
    const size_t overlap = sig_bytes.size();
    
    for (size_t offset = 0; offset < range; offset += chunk_size) {
        size_t current_chunk_size = std::min(chunk_size, range - offset);
        std::vector<uint8_t> chunk = read_memory_chunk(start + offset, current_chunk_size + overlap);
        
        if (chunk.size() < sig_bytes.size()) continue;
        
        for (size_t i = 0; i < chunk.size() - sig_bytes.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < sig_bytes.size(); ++j) {
                if (mask[j] && chunk[i + j] != sig_bytes[j]) {
                    found = false;
                    break;
                }
            }
            
            if (found) {
                return start + offset + i;
            }
        }
    }
    
    return 0;
}
