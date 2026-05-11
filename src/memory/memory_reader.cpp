#include "memory/memory_reader.hpp"
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>

WindowsMemoryReader::WindowsMemoryReader(const wchar_t* process_name) {
    DWORD process_ids[1024];
    DWORD bytes_returned;
    
    if (!EnumProcesses(process_ids, sizeof(process_ids), &bytes_returned)) {
        return;
    }
    
    DWORD process_count = bytes_returned / sizeof(DWORD);
    
    for (DWORD i = 0; i < process_count; ++i) {
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                                     FALSE, process_ids[i]);
        if (process != nullptr) {
            wchar_t process_name_buffer[MAX_PATH];
            if (GetModuleBaseName(process, nullptr, process_name_buffer, MAX_PATH)) {
                if (wcscmp(process_name_buffer, process_name) == 0) {
                    process_handle = OpenProcess(PROCESS_VM_READ, FALSE, process_ids[i]);
                    process_id = process_ids[i];
                    CloseHandle(process);
                    return;
                }
            }
            CloseHandle(process);
        }
    }
}

WindowsMemoryReader::~WindowsMemoryReader() {
    if (process_handle != nullptr) {
        CloseHandle(process_handle);
    }
}

size_t WindowsMemoryReader::read_memory(uintptr_t address, void* buffer, size_t size) {
    if (process_handle == nullptr) return 0;
    
    SIZE_T bytes_read = 0;
    ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(address), 
                      buffer, size, &bytes_read);
    return bytes_read;
}

uintptr_t WindowsMemoryReader::get_module_base(const wchar_t* module_name) {
    if (process_handle == nullptr) return 0;
    
    HMODULE modules[1024];
    DWORD bytes_returned;
    
    if (!EnumProcessModules(process_handle, modules, sizeof(modules), &bytes_returned)) {
        return 0;
    }
    
    DWORD module_count = bytes_returned / sizeof(HMODULE);
    
    for (DWORD i = 0; i < module_count; ++i) {
        wchar_t module_name_buffer[MAX_PATH];
        if (GetModuleBaseName(process_handle, modules[i], module_name_buffer, MAX_PATH)) {
            if (wcscmp(module_name_buffer, module_name) == 0) {
                return reinterpret_cast<uintptr_t>(modules[i]);
            }
        }
    }
    
    return 0;
}

#endif

#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>

pid_t LinuxMemoryReader::find_process_by_name(const char* process_name) {
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        fprintf(stderr, "Failed to open /proc directory\n");
        return -1;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is all digits (process ID)
        if (entry->d_type != DT_DIR) continue;
        
        bool is_pid = true;
        for (const char* p = entry->d_name; *p; ++p) {
            if (*p < '0' || *p > '9') {
                is_pid = false;
                break;
            }
        }
        
        if (!is_pid) continue;
        
        // Read the command line from /proc/[pid]/comm
        pid_t pid = atoi(entry->d_name);
        char comm_path[256];
        snprintf(comm_path, sizeof(comm_path), "/proc/%d/comm", pid);
        
        FILE* comm_file = fopen(comm_path, "r");
        if (!comm_file) continue;
        
        char comm_buffer[256];
        if (fgets(comm_buffer, sizeof(comm_buffer), comm_file)) {
            // Remove newline
            char* newline = strchr(comm_buffer, '\n');
            if (newline) *newline = '\0';
            
            if (strstr(comm_buffer, process_name)) {
                fclose(comm_file);
                closedir(proc_dir);
                fprintf(stdout, "Found process '%s' with PID %d\n", process_name, pid);
                return pid;
            }
        }
        fclose(comm_file);
    }
    
    closedir(proc_dir);
    fprintf(stderr, "Process '%s' not found\n", process_name);
    return -1;
}

LinuxMemoryReader::LinuxMemoryReader(pid_t pid) : MemoryReader() {
    process_id = pid;
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    mem_fd = open(mem_path, O_RDONLY);
    
    if (mem_fd == -1) {
        fprintf(stderr, "Failed to open /proc/%d/mem\n", pid);
    } else {
        fprintf(stdout, "Successfully opened /proc/%d/mem\n", pid);
    }
}

LinuxMemoryReader::LinuxMemoryReader(const char* process_name) : MemoryReader() {
    pid_t pid = find_process_by_name(process_name);
    if (pid == -1) {
        fprintf(stderr, "Failed to find process '%s'\n", process_name);
        process_id = 0;
        mem_fd = -1;
        return;
    }
    
    process_id = pid;
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    mem_fd = open(mem_path, O_RDONLY);
    
    if (mem_fd == -1) {
        fprintf(stderr, "Failed to open /proc/%d/mem (permission denied?)\n", pid);
    } else {
        fprintf(stdout, "Successfully opened /proc/%d/mem for process '%s'\n", pid, process_name);
        
        // Try to find offsets dynamically
        scanner = std::make_shared<SignatureScanner>(pid);
        if (find_offsets()) {
            fprintf(stdout, "Successfully found offsets via signature scanning\n");
        } else {
            fprintf(stdout, "Failed to find offsets via scanning, using defaults\n");
        }
    }
}

bool LinuxMemoryReader::find_offsets() {
    if (!scanner) {
        fprintf(stderr, "Signature scanner not initialized\n");
        return false;
    }
    
    fprintf(stdout, "[LinuxMemoryReader] Attempting to find offsets via signature scanning...\n");
    
    // Try to find entity list offset
    // Note: These signatures may need to be adjusted for your specific CS:GO Legacy version
    // Common pattern for entity list access
    SignatureScanner::Pattern entity_pattern(
        "A1 ?? ?? ?? ?? 8B 0C 88 85 C9 74",  // mov eax, [entity_list]; mov ecx, [eax+ecx*4]
        "x????xxxx",
        1  // Offset to the actual address in the instruction
    );
    
    fprintf(stdout, "[LinuxMemoryReader] Scanning for entity list pattern...\n");
    uintptr_t entity_addr = scanner->find_pattern(entity_pattern, 0x400000, 300 * 1024 * 1024);
    if (entity_addr != 0) {
        // Read the address from the MOV instruction
        std::vector<uint8_t> bytes = read_memory(entity_addr, 4);
        if (bytes.size() >= 4) {
            entity_list_offset = *(uintptr_t*)bytes.data();
            fprintf(stdout, "[LinuxMemoryReader] Found entity list offset: 0x%lx\n", entity_list_offset);
        }
    }
    
    // Try to find local player offset
    SignatureScanner::Pattern local_player_pattern(
        "8B 0D ?? ?? ?? ?? 85 C9 74",  // mov ecx, [local_player]
        "x????xxxx",
        2
    );
    
    fprintf(stdout, "[LinuxMemoryReader] Scanning for local player pattern...\n");
    uintptr_t local_addr = scanner->find_pattern(local_player_pattern, 0x400000, 300 * 1024 * 1024);
    if (local_addr != 0) {
        std::vector<uint8_t> bytes = read_memory(local_addr, 4);
        if (bytes.size() >= 4) {
            local_player_offset = *(uintptr_t*)bytes.data();
            fprintf(stdout, "[LinuxMemoryReader] Found local player offset: 0x%lx\n", local_player_offset);
        }
    }
    
    return entity_list_offset != 0 || local_player_offset != 0;
}

std::vector<uint8_t> LinuxMemoryReader::read_memory(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    if (mem_fd == -1) return buffer;
    
    if (lseek(mem_fd, address, SEEK_SET) == -1) return buffer;
    
    ssize_t bytes_read = ::read(mem_fd, buffer.data(), size);
    if (bytes_read <= 0) {
        buffer.clear();
    }
    
    return buffer;
}

LinuxMemoryReader::~LinuxMemoryReader() {
    if (mem_fd != -1) {
        close(mem_fd);
    }
}

size_t LinuxMemoryReader::read_memory(uintptr_t address, void* buffer, size_t size) {
    if (mem_fd == -1) return 0;
    
    if (lseek(mem_fd, address, SEEK_SET) == -1) return 0;
    
    ssize_t bytes_read = ::read(mem_fd, buffer, size);
    return bytes_read > 0 ? bytes_read : 0;
}

#endif

/**
 * Mock implementation for testing/demonstration
 */
MockMemoryReader::MockMemoryReader() : MemoryReader() {
    process_id = 0xDEADBEEF;
}

size_t MockMemoryReader::read_memory(uintptr_t address, void* buffer, size_t size) {
    // Return mock data for testing
    memset(buffer, 0, size);
    return size;
}
