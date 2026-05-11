#pragma once

#include <cstdint>
#include <string>
#include "memory/signature_scanner.hpp"

class OffsetManager {
public:
    struct Offsets {
        uintptr_t local_player = 0;
        uintptr_t entity_list  = 0;
        uintptr_t view_matrix  = 0;
        
        // Player netvars (Static for CS:GO Legacy)
        uintptr_t m_iHealth      = 0x100;
        uintptr_t m_iTeamNum     = 0xF4;
        uintptr_t m_vecOrigin    = 0x138;
        uintptr_t m_dwBoneMatrix = 0x26A8;
    };

    static OffsetManager& instance();

    /**
     * Initialize offsets by scanning the game process
     */
    bool initialize(pid_t pid);

    /**
     * Get the current offsets
     */
    const Offsets& get() const { return current_offsets; }

    /**
     * Check if offsets were successfully found
     */
    bool is_ready() const { return ready; }

private:
    OffsetManager() = default;
    
    Offsets current_offsets;
    bool ready = false;
};
