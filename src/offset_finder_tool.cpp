#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "memory/offset_finder.hpp"

void print_usage() {
    fprintf(stdout, R"(
=== CS:GO Legacy Offset Finder ===

This tool automatically finds game offsets by scanning memory for known values.

USAGE:
  1. Start the offset finder
  2. Enter your current health value
  3. Take damage and enter new health value
  4. Repeat 2-3 times to narrow down results
  5. Use the found offset in the overlay

INSTRUCTIONS:
  Step 1: Find the game process PID
    pgrep -f csgo_linux64

  Step 2: Run this tool with the PID:
    ./offset_finder <pid>

EXAMPLE SESSION:
  $ ./offset_finder 12345
  Found process: csgo_linux64 (12345)
  
  Enter your current health: 100
  Scanning for 100... Found 523 locations
  
  Take damage now. Enter new health: 75
  Narrowing results... Found 12 locations
  
  Take damage again. Enter new health: 50
  Narrowing results... Found 1 location
  
  HEALTH OFFSET: 0x4a32b8

Then update the offset in the overlay and recompile!

)");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage();
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Invalid PID: %s\n", argv[1]);
        return 1;
    }
    
    fprintf(stdout, "\n=== CS:GO Legacy Offset Finder ===\n");
    fprintf(stdout, "Target PID: %d\n", pid);
    fprintf(stdout, "Make sure the game is running and you have root privileges!\n\n");
    
    OffsetFinder finder(pid);
    std::vector<OffsetFinder::FoundOffset> results;
    
    int iteration = 0;
    while (true) {
        fprintf(stdout, "\nIteration %d\n", ++iteration);
        fprintf(stdout, "================================\n");
        
        uint32_t health;
        fprintf(stdout, "Enter your CURRENT health value: ");
        fflush(stdout);
        
        if (scanf("%u", &health) != 1) {
            fprintf(stderr, "Invalid input\n");
            while (getchar() != '\n');
            continue;
        }
        
        // First iteration: scan for the value
        if (iteration == 1) {
            results = finder.find_health_offsets(health);
        } else if (!results.empty()) {
            // Subsequent iterations: narrow down
            results = finder.narrow_results(results, health);
        }
        
        // Print current results
        finder.print_results(results);
        
        if (results.size() <= 5) {
            fprintf(stdout, "POSSIBLE HEALTH OFFSETS FOUND!\n");
            fprintf(stdout, "Try each offset in the overlay:\n\n");
            
            for (size_t i = 0; i < results.size(); ++i) {
                fprintf(stdout, "  Option %zu: 0x%lx (verify: %u)\n",
                        i + 1, results[i].address,
                        finder.verify_offset(results[i].address));
            }
            
            fprintf(stdout, "\nCalculate relative offset (if needed):\n");
            fprintf(stdout, "  module_base + (offset - 0x400000) = final_offset\n\n");
            break;
        }
        
        fprintf(stdout, "\nTake damage now and try again.\n");
        fprintf(stdout, "Continue narrowing? (y/n): ");
        fflush(stdout);
        
        char response;
        if (scanf("%c", &response) == 1) {
            if (response != 'y' && response != 'Y') {
                break;
            }
        }
        
        // Clear input buffer
        while (getchar() != '\n');
    }
    
    fprintf(stdout, "\n=== OFFSET FINDER COMPLETE ===\n");
    fprintf(stdout, "Update the offsets in include/game/game_structures.hpp\n");
    fprintf(stdout, "Then rebuild the overlay and run it!\n\n");
    
    return 0;
}
