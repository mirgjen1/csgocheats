#include "memory/offset_manager.hpp"
#include "game/game_structures.hpp"
#include <cstdio>

OffsetManager& OffsetManager::instance() {
    static OffsetManager inst;
    return inst;
}

bool OffsetManager::initialize(pid_t pid) {
    if (ready) return true;

    fprintf(stdout, "[OffsetManager] Initializing for PID %d...\n", pid);
    
    SignatureScanner scanner(pid);
    if (!scanner.load_modules()) {
        fprintf(stderr, "[OffsetManager] Failed to load modules\n");
        return false;
    }

    // CS:GO Legacy Linux 64-bit module name is usually client_client.so
    const std::string client_mod = "client_client.so";
    
    fprintf(stdout, "[OffsetManager] Scanning %s for offsets...\n", client_mod.c_str());

    // Find Local Player
    SignatureScanner::Pattern local_player_pat(offsets::LOCAL_PLAYER_SIG, 3, true);
    current_offsets.local_player = scanner.find_pattern(client_mod, local_player_pat);
    if (current_offsets.local_player) {
        fprintf(stdout, "[OffsetManager] Found local_player offset: 0x%lx\n", current_offsets.local_player);
    } else {
        // Fallback to static if scanning fails (though static are often wrong)
        fprintf(stderr, "[OffsetManager] Failed to find local_player via pattern\n");
    }

    // Find Entity List
    SignatureScanner::Pattern entity_list_pat(offsets::ENTITY_LIST_SIG, 3, true);
    current_offsets.entity_list = scanner.find_pattern(client_mod, entity_list_pat);
    if (current_offsets.entity_list) {
        fprintf(stdout, "[OffsetManager] Found entity_list offset: 0x%lx\n", current_offsets.entity_list);
    } else {
        fprintf(stderr, "[OffsetManager] Failed to find entity_list via pattern\n");
    }

    // Find View Matrix
    SignatureScanner::Pattern view_matrix_pat(offsets::VIEW_MATRIX_SIG, 3, true);
    current_offsets.view_matrix = scanner.find_pattern(client_mod, view_matrix_pat);
    if (current_offsets.view_matrix) {
        fprintf(stdout, "[OffsetManager] Found view_matrix offset: 0x%lx\n", current_offsets.view_matrix);
    } else {
        fprintf(stderr, "[OffsetManager] Failed to find view_matrix via pattern\n");
    }

    ready = (current_offsets.local_player != 0 && current_offsets.entity_list != 0);
    return ready;
}
