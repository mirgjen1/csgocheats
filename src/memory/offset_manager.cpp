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

    // Search for any module containing "client" and ending in ".so"
    std::string found_client;
    // Priority list for CS:GO Legacy / CS2 modules
    const std::vector<std::string> priorities = {
        "client_client.so", 
        "client_panorama_client.so",
        "client.so",
        "libclient.so"
    };

    for (const auto& target : priorities) {
        for (const auto& mod : all_modules) {
            if (mod.name == target) {
                found_client = mod.name;
                break;
            }
        }
        if (!found_client.empty()) break;
    }

    // Fallback if priority names not found
    if (found_client.empty()) {
        for (const auto& mod : all_modules) {
            std::string name = mod.name;
            if (name == "steamclient.so" || name.find("libwayland") != std::string::npos || name.find("vaudio") != std::string::npos) {
                continue;
            }
            if (name.find("client") != std::string::npos && name.find(".so") != std::string::npos) {
                found_client = name;
                break;
            }
        }
    }

    if (found_client.empty()) {
        fprintf(stderr, "[OffsetManager] Could not find any client module! Printing all modules:\n");
        for (const auto& mod : all_modules) {
            fprintf(stderr, "  - %s\n", mod.name.c_str());
        }
        return false;
    }
    
    client_module_name = found_client;
    const auto* client_info = scanner.get_module(found_client);
    fprintf(stdout, "[OffsetManager] Using client module: %s (Base: 0x%lx, Path: %s)\n", 
            found_client.c_str(), client_info->base, client_info->path.c_str());

    // Find Local Player
    SignatureScanner::Pattern local_player_pat(offsets::LOCAL_PLAYER_SIG, 3, true);
    current_offsets.local_player = scanner.find_pattern(found_client, local_player_pat);
    if (current_offsets.local_player) {
        fprintf(stdout, "[OffsetManager] SUCCESS: Found local_player offset: 0x%lx (Absolute: 0x%lx)\n", 
                current_offsets.local_player, client_info->base + current_offsets.local_player);
    } else {
        fprintf(stderr, "[OffsetManager] ERROR: Failed to find local_player signature!\n");
    }

    // Find Entity List
    SignatureScanner::Pattern entity_list_pat(offsets::ENTITY_LIST_SIG, 3, true);
    current_offsets.entity_list = scanner.find_pattern(found_client, entity_list_pat);
    if (current_offsets.entity_list) {
        fprintf(stdout, "[OffsetManager] SUCCESS: Found entity_list offset: 0x%lx (Absolute: 0x%lx)\n", 
                current_offsets.entity_list, client_info->base + current_offsets.entity_list);
    } else {
        fprintf(stderr, "[OffsetManager] ERROR: Failed to find entity_list signature!\n");
    }

    // Find View Matrix (Try client first, then engine)
    SignatureScanner::Pattern view_matrix_pat(offsets::VIEW_MATRIX_SIG, 3, true);
    current_offsets.view_matrix = scanner.find_pattern(found_client, view_matrix_pat);
    
    if (!current_offsets.view_matrix) {
        for (const auto& mod : all_modules) {
            if (mod.name.find("engine") != std::string::npos && mod.name.find(".so") != std::string::npos) {
                current_offsets.view_matrix = scanner.find_pattern(mod.name, view_matrix_pat);
                if (current_offsets.view_matrix) {
                    fprintf(stdout, "[OffsetManager] SUCCESS: Found view_matrix in %s: 0x%lx\n", 
                            mod.name.c_str(), current_offsets.view_matrix);
                    break;
                }
            }
        }
    } else {
        fprintf(stdout, "[OffsetManager] Found view_matrix in %s: 0x%lx\n", found_client.c_str(), current_offsets.view_matrix);
    }

    // Apply discovered fallbacks if patterns failed
    if (current_offsets.local_player == 0) {
        fprintf(stdout, "[OffsetManager] Falling back to hardcoded local_player: 0x%lx\n", offsets::LOCAL_PLAYER);
        current_offsets.local_player = offsets::LOCAL_PLAYER;
    }
    if (current_offsets.entity_list == 0) {
        fprintf(stdout, "[OffsetManager] Falling back to hardcoded entity_list: 0x%lx\n", offsets::ENTITY_LIST);
        current_offsets.entity_list = offsets::ENTITY_LIST;
    }
    if (current_offsets.view_matrix == 0) {
        fprintf(stdout, "[OffsetManager] Falling back to hardcoded view_matrix: 0x%lx\n", offsets::VIEW_MATRIX);
        current_offsets.view_matrix = offsets::VIEW_MATRIX;
    }

    ready = true;
    return true;
}
