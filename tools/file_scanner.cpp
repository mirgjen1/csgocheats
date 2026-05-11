#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// Pattern scan in a buffer
ssize_t scan_pattern(const uint8_t* buf, size_t buf_size, 
                     const uint8_t* pattern, const uint8_t* mask, size_t pat_size) {
    if (buf_size < pat_size) return -1;
    for (size_t i = 0; i <= buf_size - pat_size; ++i) {
        bool found = true;
        for (size_t j = 0; j < pat_size; ++j) {
            if (mask[j] && buf[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) return (ssize_t)i;
    }
    return -1;
}

void parse_pattern(const std::string& sig_str, std::vector<uint8_t>& bytes, std::vector<uint8_t>& mask) {
    std::stringstream ss(sig_str);
    std::string token;
    while (ss >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0);
            mask.push_back(0);
        } else {
            bytes.push_back((uint8_t)std::stoul(token, nullptr, 16));
            mask.push_back(1);
        }
    }
}

uintptr_t scan_file(const std::string& path, const std::string& sig_str, const char* label) {
    std::vector<uint8_t> pattern;
    std::vector<uint8_t> mask;
    parse_pattern(sig_str, pattern, mask);
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        printf("Failed to open %s\n", path.c_str());
        return 0;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read((char*)data.data(), size);
    
    ssize_t pos = scan_pattern(data.data(), data.size(), 
                                pattern.data(), mask.data(), pattern.size());
    if (pos >= 0) {
        printf("  [FOUND] %s at offset 0x%lx\n", label, (long)pos);
        return (uintptr_t)pos;
    }
    
    printf("  [MISS]  %s not found\n", label);
    return 0;
}

uintptr_t resolve_rip(const std::string& path, uintptr_t offset, int operand_offset) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;
    
    int32_t rel;
    file.seekg(offset + operand_offset);
    file.read((char*)&rel, sizeof(rel));
    
    // RIP = address of instruction + size of instruction
    // In our case, instruction starts at 'offset'
    // The relative address is at 'offset + operand_offset'
    // The NEXT instruction starts at 'offset + operand_offset + 4'
    return offset + operand_offset + 4 + rel;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <path_to_so_file>\n", argv[0]);
        return 1;
    }
    
    std::string path = argv[1];
    printf("Scanning file: %s\n", path.c_str());
    
    const char* lp_patterns[] = {
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 0B",
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 11",
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 12",
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74",
        "48 8B 05 ?? ?? ?? ?? 48 89 C7"
    };
    
    uintptr_t lp_off = 0;
    for (auto p : lp_patterns) {
        lp_off = scan_file(path, p, "dwLocalPlayer");
        if (lp_off) break;
    }
    if (lp_off) {
        uintptr_t addr = resolve_rip(path, lp_off, 3);
        printf("  => dwLocalPlayer relative offset: 0x%lx\n", addr);
    }
    
    const char* el_patterns[] = {
        "48 8D 05 ?? ?? ?? ?? 48 8B 00 48 85 C0 74 ?? 48 8B 00",
        "48 8D 05 ?? ?? ?? ?? 48 8B 00 48 85 C0 74",
        "48 8D 05 ?? ?? ?? ?? 48 8B 00",
        "48 8D 05 ?? ?? ?? ?? 48 89 C7 E8"
    };
    
    uintptr_t el_off = 0;
    for (auto p : el_patterns) {
        el_off = scan_file(path, p, "dwEntityList");
        if (el_off) break;
    }
    if (el_off) {
        uintptr_t addr = resolve_rip(path, el_off, 3);
        printf("  => dwEntityList relative offset: 0x%lx\n", addr);
    }

    const char* vm_patterns[] = {
        "48 8D 05 ?? ?? ?? ?? 48 8D 3D ?? ?? ?? ?? 48 8D 35",
        "48 8D 05 ?? ?? ?? ?? 48 8D 3D",
        "F3 0F 10 05 ?? ?? ?? ?? F3 0F 11 44 24 ?? F3 0F 10 05"
    };
    
    uintptr_t vm_off = 0;
    for (auto p : vm_patterns) {
        vm_off = scan_file(path, p, "dwViewMatrix");
        if (vm_off) break;
    }
    if (vm_off) {
        uintptr_t addr = resolve_rip(path, vm_off, 3);
        printf("  => dwViewMatrix relative offset: 0x%lx\n", addr);
    }
    
    return 0;
}
