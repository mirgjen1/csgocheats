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

LinuxMemoryReader::LinuxMemoryReader(pid_t pid) : MemoryReader() {
    process_id = pid;
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    mem_fd = open(mem_path, O_RDONLY);
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
