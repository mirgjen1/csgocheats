#include "memory/memory_reader.hpp"
#include "memory/offset_manager.hpp"
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
    // First, check if PID 10330 is valid and has game modules (user specified)
    const pid_t manual_pid = 10330;
    char manual_maps[256];
    snprintf(manual_maps, sizeof(manual_maps), "/proc/%d/maps", manual_pid);
    if (access(manual_maps, F_OK) != -1) {
        fprintf(stdout, "[MemoryReader] Testing manual PID 10330...\n");
        return manual_pid;
    }

    DIR* dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;
        pid_t pid = std::stoi(entry->d_name);
        
        // Read maps to see if this process has the game loaded
        char maps_path[256];
        snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
        FILE* maps_f = fopen(maps_path, "r");
        if (maps_f) {
            char line[512];
            bool found = false;
            while (fgets(line, sizeof(line), maps_f)) {
                if (strstr(line, "client_client.so") || strstr(line, "libclient.so") || strstr(line, "client.so")) {
                    found = true;
                    break;
                }
            }
            fclose(maps_f);
            
            if (found) {
                closedir(dir);
                fprintf(stdout, "[MemoryReader] Found ACTIVE game process with PID %d\n", pid);
                return pid;
            }
        }
    }

    closedir(dir);
    fprintf(stderr, "[MemoryReader] Could not find any process with game modules loaded. Make sure you are in a match!\n");
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
        
        // Initialize dynamic offsets
        if (OffsetManager::instance().initialize(pid)) {
            fprintf(stdout, "Successfully initialized dynamic offsets\n");
        } else {
            fprintf(stderr, "Failed to initialize dynamic offsets\n");
        }

        scanner = std::make_shared<SignatureScanner>(pid);
        scanner->load_modules();
    }
}

uintptr_t LinuxMemoryReader::get_module_base(const char* module_name) {
    if (!scanner) return 0;
    const auto* mod = scanner->get_module(module_name);
    return mod ? mod->base : 0;
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
