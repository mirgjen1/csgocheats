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
        fprintf(stderr, "[OffsetManager] Failed to load modules from maps\n");
        return false;
    }

    // DEBUG: Print some loaded modules to verify
    const auto& all_modules = scanner.get_all_modules();
    fprintf(stdout, "[OffsetManager] Total modules loaded: %zu\n", all_modules.size());
    for (size_t i = 0; i < std::min(all_modules.size(), (size_t)10); ++i) {
        fprintf(stdout, "[OffsetManager] Module: %s at 0x%lx\n", all_modules[i].name.c_str(), all_modules[i].base);
    }

    // CS:GO Legacy Linux 64-bit module names
    // It can be client_client.so or client.so depending on build
    const std::vector<std::string> client_mods = {"client_client.so", "client.so", "libclient.so"};
    const std::vector<std::string> engine_mods = {"engine_client.so", "engine.so", "libengine.so"};
    
    std::string found_client;
    for (const auto& mod : client_mods) {
        if (scanner.get_module(mod)) {
            found_client = mod;
            break;
        }
    }

    if (found_client.empty()) {
        fprintf(stderr, "[OffsetManager] Could not find any client module!\n");
        return false;
    }
    
    client_module_name = found_client;
    fprintf(stdout, "[OffsetManager] Using client module: %s\n", found_client.c_str());

    // Find Local Player
    SignatureScanner::Pattern local_player_pat(offsets::LOCAL_PLAYER_SIG, 3, true);
    current_offsets.local_player = scanner.find_pattern(found_client, local_player_pat);
    if (current_offsets.local_player) {
        fprintf(stdout, "[OffsetManager] Found local_player offset: 0x%lx\n", current_offsets.local_player);
    }

    // Find Entity List
    SignatureScanner::Pattern entity_list_pat(offsets::ENTITY_LIST_SIG, 3, true);
    current_offsets.entity_list = scanner.find_pattern(found_client, entity_list_pat);
    if (current_offsets.entity_list) {
        fprintf(stdout, "[OffsetManager] Found entity_list offset: 0x%lx\n", current_offsets.entity_list);
    }

    // Find View Matrix (Try client first, then engine)
    SignatureScanner::Pattern view_matrix_pat(offsets::VIEW_MATRIX_SIG, 3, true);
    current_offsets.view_matrix = scanner.find_pattern(found_client, view_matrix_pat);
    
    if (!current_offsets.view_matrix) {
        for (const auto& mod : engine_mods) {
            if (scanner.get_module(mod)) {
                current_offsets.view_matrix = scanner.find_pattern(mod, view_matrix_pat);
                if (current_offsets.view_matrix) {
                    fprintf(stdout, "[OffsetManager] Found view_matrix in %s: 0x%lx\n", mod.c_str(), current_offsets.view_matrix);
                    break;
                }
            }
        }
    } else {
        fprintf(stdout, "[OffsetManager] Found view_matrix in %s: 0x%lx\n", found_client.c_str(), current_offsets.view_matrix);
    }

    // Apply discovered fallbacks if patterns failed
    if (current_offsets.local_player == 0) {
        fprintf(stdout, "[OffsetManager] Falling back to hardcoded local_player: 0x22eceb0\n");
        current_offsets.local_player = 0x22eceb0;
    }
    if (current_offsets.entity_list == 0) {
        fprintf(stdout, "[OffsetManager] Falling back to hardcoded entity_list: 0x6d9eef8\n");
        current_offsets.entity_list = 0x6d9eef8;
    }
    if (current_offsets.view_matrix == 0) {
        fprintf(stdout, "[OffsetManager] Falling back to hardcoded view_matrix: 0x2c83fa8\n");
        current_offsets.view_matrix = 0x2c83fa8;
    }

    ready = true;
    return true;
}
