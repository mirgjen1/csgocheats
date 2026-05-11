/**
 * Standalone offset dumper for CS:GO Legacy Linux 64-bit
 * Scans the actual .so binary on disk and process memory to find offsets.
 * 
 * Usage: sudo ./offset_dumper
 */
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

struct ModuleInfo {
    uintptr_t base;
    uintptr_t end;
    size_t size;
    std::string path;
    std::string name;
};

// Find CS:GO process
pid_t find_csgo_pid() {
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return -1;
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        
        bool is_pid = true;
        for (const char* p = entry->d_name; *p; ++p) {
            if (*p < '0' || *p > '9') { is_pid = false; break; }
        }
        if (!is_pid) continue;
        
        pid_t pid = atoi(entry->d_name);
        char comm_path[256];
        snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
        
        FILE* f = fopen(comm_path, "r");
        if (!f) continue;
        
        char buf[256];
        if (fgets(buf, sizeof(buf), f)) {
            char* nl = strchr(buf, '\n');
            if (nl) *nl = '\0';
            if (strstr(buf, "csgo_linux64")) {
                fclose(f);
                closedir(proc_dir);
                return pid;
            }
        }
        fclose(f);
    }
    closedir(proc_dir);
    return -1;
}

// Load modules from /proc/pid/maps
std::vector<ModuleInfo> load_modules(pid_t pid) {
    std::vector<ModuleInfo> modules;
    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    
    std::ifstream maps(maps_path);
    std::string line;
    while (std::getline(maps, line)) {
        std::stringstream ss(line);
        std::string range, perms, offset, dev, inode, path;
        ss >> range >> perms >> offset >> dev >> inode >> path;
        
        if (path.empty() || path[0] == '[') continue;
        if (perms.find('r') == std::string::npos) continue;
        
        size_t last_slash = path.find_last_of('/');
        std::string name = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
        
        size_t dash = range.find('-');
        uintptr_t start = std::stoull(range.substr(0, dash), nullptr, 16);
        uintptr_t end = std::stoull(range.substr(dash + 1), nullptr, 16);
        
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
        }
    }
    return modules;
}

// Find a module by partial name
const ModuleInfo* find_module(const std::vector<ModuleInfo>& modules, const std::string& partial) {
    for (const auto& mod : modules) {
        if (mod.name.find(partial) != std::string::npos || mod.path.find(partial) != std::string::npos) {
            return &mod;
        }
    }
    return nullptr;
}

// Read process memory
bool read_mem(int fd, uintptr_t addr, void* buf, size_t sz) {
    ssize_t n = pread(fd, buf, sz, addr);
    return n == (ssize_t)sz;
}

// Pattern scan in a buffer
// Returns offset within buffer, or -1 if not found
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

// Parse IDA-style pattern string
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

// Scan process memory for a pattern, return absolute address of match
uintptr_t scan_memory(int mem_fd, uintptr_t base, size_t size, 
                       const std::string& sig_str, const char* label) {
    std::vector<uint8_t> pattern;
    std::vector<uint8_t> mask;
    parse_pattern(sig_str, pattern, mask);
    
    const size_t chunk_size = 1024 * 1024; // 1MB chunks
    
    for (size_t offset = 0; offset < size; offset += chunk_size) {
        size_t read_size = std::min(chunk_size + pattern.size(), size - offset);
        std::vector<uint8_t> chunk(read_size);
        
        if (!read_mem(mem_fd, base + offset, chunk.data(), read_size)) {
            continue;
        }
        
        ssize_t pos = scan_pattern(chunk.data(), chunk.size(), 
                                    pattern.data(), mask.data(), pattern.size());
        if (pos >= 0) {
            uintptr_t match_addr = base + offset + pos;
            printf("  [FOUND] %s pattern match at 0x%lx (offset 0x%lx from base)\n", 
                   label, match_addr, match_addr - base);
            return match_addr;
        }
    }
    
    printf("  [MISS]  %s pattern not found\n", label);
    return 0;
}

// Resolve RIP-relative address
uintptr_t resolve_rip_relative(int mem_fd, uintptr_t instruction_addr, int operand_offset) {
    int32_t rel;
    if (!read_mem(mem_fd, instruction_addr + operand_offset, &rel, sizeof(rel))) {
        return 0;
    }
    // RIP-relative: target = instruction_addr + operand_offset + 4 + rel
    return instruction_addr + operand_offset + 4 + rel;
}

int main() {
    printf("=== CS:GO Legacy Linux 64-bit Offset Dumper ===\n\n");
    
    pid_t pid = find_csgo_pid();
    if (pid == -1) {
        fprintf(stderr, "ERROR: csgo_linux64 process not found!\n");
        return 1;
    }
    printf("Found csgo_linux64 with PID: %d\n\n", pid);
    
    // Open process memory
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    int mem_fd = open(mem_path, O_RDONLY);
    if (mem_fd == -1) {
        fprintf(stderr, "ERROR: Cannot open %s (run with sudo)\n", mem_path);
        return 1;
    }
    
    // Load modules
    auto modules = load_modules(pid);
    printf("Loaded %zu modules from /proc/%d/maps\n\n", modules.size(), pid);
    
    // Find key modules
    const ModuleInfo* client = find_module(modules, "client_client.so");
    const ModuleInfo* engine = find_module(modules, "engine_client.so");
    
    if (!client) {
        // Try alternate names
        client = find_module(modules, "client.so");
    }
    if (!engine) {
        engine = find_module(modules, "engine.so");
    }
    
    if (client) {
        printf("Client module: %s\n", client->path.c_str());
        printf("  Base: 0x%lx  End: 0x%lx  Size: 0x%lx (%zu MB)\n\n", 
               client->base, client->end, client->size, client->size / (1024*1024));
    } else {
        fprintf(stderr, "ERROR: Could not find client module!\n");
        
        // List all modules with 'client' in name
        printf("\nModules containing 'client':\n");
        for (const auto& m : modules) {
            if (m.name.find("client") != std::string::npos) {
                printf("  %s at 0x%lx (size: 0x%lx)\n", m.path.c_str(), m.base, m.size);
            }
        }
        close(mem_fd);
        return 1;
    }
    
    if (engine) {
        printf("Engine module: %s\n", engine->path.c_str());
        printf("  Base: 0x%lx  End: 0x%lx  Size: 0x%lx (%zu MB)\n\n", 
               engine->base, engine->end, engine->size, engine->size / (1024*1024));
    }
    
    // ========================
    // Pattern Scanning
    // ========================
    printf("--- Scanning for dwLocalPlayer ---\n");
    
    // Try multiple patterns for dwLocalPlayer
    // Pattern 1: mov rax, [rip+xxx] ; test rax, rax ; jz
    const char* lp_patterns[] = {
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74",       // mov rax,[rip+x]; test rax,rax; jz
        "48 8B 05 ?? ?? ?? ?? 48 89 C7",           // mov rax,[rip+x]; mov rdi,rax
        "48 8B 05 ?? ?? ?? ?? 48 8B 40",           // mov rax,[rip+x]; mov rax,[rax+y]
        "48 8B 1D ?? ?? ?? ?? 48 85 DB 74",        // mov rbx,[rip+x]; test rbx,rbx; jz
        "48 8B 3D ?? ?? ?? ?? 48 85 FF",           // mov rdi,[rip+x]; test rdi,rdi
    };
    
    uintptr_t lp_match = 0;
    for (const auto& pat : lp_patterns) {
        lp_match = scan_memory(mem_fd, client->base, client->size, pat, "dwLocalPlayer");
        if (lp_match) break;
    }
    
    uintptr_t dwLocalPlayer = 0;
    if (lp_match) {
        dwLocalPlayer = resolve_rip_relative(mem_fd, lp_match, 3);
        printf("  => dwLocalPlayer absolute: 0x%lx (relative to client: 0x%lx)\n", 
               dwLocalPlayer, dwLocalPlayer - client->base);
        
        // Verify: try to read what's at this address
        uintptr_t lp_value = 0;
        if (read_mem(mem_fd, dwLocalPlayer, &lp_value, sizeof(lp_value))) {
            printf("  => Value at dwLocalPlayer: 0x%lx %s\n", lp_value,
                   lp_value != 0 ? "(looks valid!)" : "(NULL - not in game?)");
        }
    }
    
    printf("\n--- Scanning for dwEntityList ---\n");
    
    // Try multiple patterns for dwEntityList
    // Pattern: lea rax, [rip+xxx]
    const char* el_patterns[] = {
        "48 8D 05 ?? ?? ?? ?? 48 89 C7 E8",        // lea rax,[rip+x]; mov rdi,rax; call
        "48 8D 05 ?? ?? ?? ?? 48 8B 04",            // lea rax,[rip+x]; mov rax,[rax+y]
        "48 8D 05 ?? ?? ?? ?? 48 63",               // lea rax,[rip+x]; movsxd
        "48 8D 35 ?? ?? ?? ?? 48",                  // lea rsi,[rip+x]
        "48 8D 0D ?? ?? ?? ?? 48 63",               // lea rcx,[rip+x]; movsxd
        "48 8D 3D ?? ?? ?? ?? 48 63",               // lea rdi,[rip+x]; movsxd
    };
    
    uintptr_t el_match = 0;
    for (const auto& pat : el_patterns) {
        el_match = scan_memory(mem_fd, client->base, client->size, pat, "dwEntityList");
        if (el_match) break;
    }
    
    uintptr_t dwEntityList = 0;
    if (el_match) {
        dwEntityList = resolve_rip_relative(mem_fd, el_match, 3);
        printf("  => dwEntityList absolute: 0x%lx (relative to client: 0x%lx)\n",
               dwEntityList, dwEntityList - client->base);
        
        // Verify: try to read first entity
        uintptr_t ent0 = 0;
        if (read_mem(mem_fd, dwEntityList, &ent0, sizeof(ent0))) {
            printf("  => First entity pointer: 0x%lx %s\n", ent0,
                   (ent0 > 0x10000 && ent0 < 0x7fffffffffff) ? "(looks valid!)" : "(suspicious)");
        }
    }
    
    printf("\n--- Scanning for dwViewMatrix ---\n");
    
    // ViewMatrix patterns
    const char* vm_patterns[] = {
        "48 8D 05 ?? ?? ?? ?? F3 0F 10",            // lea rax,[rip+x]; movss
        "48 8D 05 ?? ?? ?? ?? 48 8D 3D",            // lea rax,[rip+x]; lea rdi,[rip+y]
        "48 8D 05 ?? ?? ?? ?? 48 8D 35",            // lea rax,[rip+x]; lea rsi,[rip+y]
        "48 8D 0D ?? ?? ?? ?? F3 0F 10",            // lea rcx,[rip+x]; movss
    };
    
    uintptr_t vm_match = 0;
    for (const auto& pat : vm_patterns) {
        vm_match = scan_memory(mem_fd, client->base, client->size, pat, "dwViewMatrix");
        if (vm_match) break;
    }
    
    // Also try engine module
    if (!vm_match && engine) {
        printf("  Trying engine module...\n");
        for (const auto& pat : vm_patterns) {
            vm_match = scan_memory(mem_fd, engine->base, engine->size, pat, "dwViewMatrix(engine)");
            if (vm_match) break;
        }
    }
    
    uintptr_t dwViewMatrix = 0;
    if (vm_match) {
        dwViewMatrix = resolve_rip_relative(mem_fd, vm_match, 3);
        uintptr_t base = (vm_match >= client->base && vm_match < client->end) ? client->base : 
                         (engine ? engine->base : 0);
        printf("  => dwViewMatrix absolute: 0x%lx (relative: 0x%lx)\n",
               dwViewMatrix, dwViewMatrix - base);
        
        // Verify: read first 4 floats
        float mat[4];
        if (read_mem(mem_fd, dwViewMatrix, mat, sizeof(mat))) {
            printf("  => First 4 floats: %.4f %.4f %.4f %.4f\n", mat[0], mat[1], mat[2], mat[3]);
        }
    }
    
    // ========================
    // Summary
    // ========================
    printf("\n=== SUMMARY ===\n");
    printf("Client base: 0x%lx\n", client->base);
    if (engine) printf("Engine base: 0x%lx\n", engine->base);
    printf("\n");
    
    if (dwLocalPlayer) {
        printf("dwLocalPlayer = 0x%lx (absolute)\n", dwLocalPlayer);
        printf("dwLocalPlayer = 0x%lx (offset from client)\n", dwLocalPlayer - client->base);
    } else {
        printf("dwLocalPlayer = NOT FOUND\n");
    }
    
    if (dwEntityList) {
        printf("dwEntityList  = 0x%lx (absolute)\n", dwEntityList);
        printf("dwEntityList  = 0x%lx (offset from client)\n", dwEntityList - client->base);
    } else {
        printf("dwEntityList  = NOT FOUND\n");
    }
    
    if (dwViewMatrix) {
        printf("dwViewMatrix  = 0x%lx (absolute)\n", dwViewMatrix);
    } else {
        printf("dwViewMatrix  = NOT FOUND\n");
    }
    
    // ========================
    // Brute-force entity list discovery
    // ========================
    if (!dwEntityList && dwLocalPlayer) {
        printf("\n--- Brute-force entity list search ---\n");
        printf("Trying to find entity list by scanning for local player pointer...\n");
        
        uintptr_t local_player_value = 0;
        read_mem(mem_fd, dwLocalPlayer, &local_player_value, sizeof(local_player_value));
        
        if (local_player_value != 0) {
            // Scan the .data section for a pointer array that contains local_player_value
            // The entity list should have local_player_value at index 1 (player index 1)
            // or somewhere in the first few slots
            
            // Read client .data section (typically after .text, in the higher half of the mapping)
            // The executable part ends and data starts - scan the full module
            size_t scan_start = client->size / 2; // Start from middle (skip .text)
            size_t scan_size = client->size - scan_start;
            
            printf("Scanning client data section for local player pointer 0x%lx...\n", local_player_value);
            
            const size_t chunk_size = 1024 * 1024;
            for (size_t off = scan_start; off < client->size; off += chunk_size) {
                size_t sz = std::min(chunk_size, client->size - off);
                std::vector<uint8_t> chunk(sz);
                if (!read_mem(mem_fd, client->base + off, chunk.data(), sz)) continue;
                
                // Search for the pointer value
                for (size_t i = 0; i + 8 <= chunk.size(); i += 8) {
                    uintptr_t val;
                    memcpy(&val, &chunk[i], 8);
                    if (val == local_player_value) {
                        uintptr_t found_addr = client->base + off + i;
                        printf("  Found local player ptr at 0x%lx (offset 0x%lx)\n", 
                               found_addr, found_addr - client->base);
                        
                        // Check if this looks like an entity list
                        // Read surrounding pointers
                        uintptr_t list_start = found_addr - 0x10; // entity 0 might be before
                        uintptr_t ptrs[5];
                        if (read_mem(mem_fd, list_start, ptrs, sizeof(ptrs))) {
                            printf("  Nearby pointers:\n");
                            for (int j = 0; j < 5; ++j) {
                                printf("    [%d] 0x%lx\n", j, ptrs[j]);
                            }
                        }
                    }
                }
            }
        }
    }
    
    close(mem_fd);
    printf("\nDone.\n");
    return 0;
}
