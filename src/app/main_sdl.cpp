#include "../core/SDL2Game.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --debug <state>    Start from specified state (for debugging)\n";
    std::cout << "                     Available states: room, town, night, night100, night1guard, night1resident, castle100, castle, demon, demon100, field, battle\n";
    std::cout << "                     Battle states: battle_slime, battle_goblin, battle_orc, battle_dragon, battle_skeleton,\n";
    std::cout << "                                     battle_ghost, battle_vampire, battle_demon_soldier, battle_werewolf,\n";
    std::cout << "                                     battle_minotaur, battle_cyclops, battle_gargoyle, battle_phantom,\n";
    std::cout << "                                     battle_dark_knight, battle_ice_giant, battle_fire_demon, battle_shadow_lord,\n";
    std::cout << "                                     battle_ancient_dragon, battle_chaos_beast, battle_elder_god, battle_demon_lord,\n";
    std::cout << "                                     battle_guard, battle_king\n";
    std::cout << "  -h, --help         Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "                    # Start from main menu (normal)\n";
    std::cout << "  " << programName << " --debug room       # Start from room (debug)\n";
    std::cout << "  " << programName << " --debug town       # Start from town (debug)\n";
    std::cout << "  " << programName << " --debug night      # Start from night state with level 25 player (debug)\n";
    std::cout << "  " << programName << " --debug night100   # Start from night state with level 100 player, all residents killed (debug)\n";
    std::cout << "  " << programName << " --debug night1guard # Start from night state with level 100 player, all residents killed, 1 guard remaining (debug)\n";
    std::cout << "  " << programName << " --debug night1resident # Start from night state with level 100 player, 1 resident remaining (debug)\n";
    std::cout << "  " << programName << " --debug castle100  # Start from castle with level 100 player, all guards and residents killed (debug)\n";
    std::cout << "  " << programName << " --debug castle     # Start from castle (debug)\n";
    std::cout << "  " << programName << " --debug demon      # Start from demon castle (debug)\n";
    std::cout << "  " << programName << " --debug demon100   # Start from demon castle with level 100 player, demon lord level 150 (debug)\n";
    std::cout << "  " << programName << " --debug field       # Start from field (debug)\n";
    std::cout << "  " << programName << " --debug battle     # Start from battle with slime (debug)\n";
    std::cout << "  " << programName << " --debug battle_slime # Start battle with slime (player and enemy both level 1)\n";
    std::cout << "  " << programName << " --debug battle_goblin # Start battle with goblin (player and enemy both level 5)\n";
    std::cout << "  " << programName << " --debug battle_orc # Start battle with orc (player and enemy both level 10)\n";
    std::cout << "  " << programName << " --debug battle_dragon # Start battle with dragon (player and enemy both level 15)\n";
    std::cout << "  " << programName << " --debug battle_demon_lord # Start battle with demon lord (player and enemy both level 100)\n";
}

int main(int argc, char* argv[]) {
    std::string debugStartState = "";
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--debug") == 0) {
            if (i + 1 < argc) {
                debugStartState = argv[i + 1];
                i++; // 次の引数も処理済みにする
            } else {
                std::cerr << "Error: --debug requires a state name\n";
                std::cerr << "Available states: room, town, night, night100, night1guard, night1resident, castle100, castle, demon, demon100, field, battle\n";
                std::cerr << "Battle states: battle_slime, battle_goblin, battle_orc, battle_dragon, battle_skeleton, battle_ghost, battle_vampire, battle_demon_soldier, battle_werewolf, battle_minotaur, battle_cyclops, battle_gargoyle, battle_phantom, battle_dark_knight, battle_ice_giant, battle_fire_demon, battle_shadow_lord, battle_ancient_dragon, battle_chaos_beast, battle_elder_god, battle_demon_lord, battle_guard, battle_king\n";
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            std::cerr << "Use --help for usage information.\n";
            return 1;
        }
    }
    
    try {
        SDL2Game game;
        
        if (!debugStartState.empty()) {
            game.setDebugStartState(debugStartState);
        }
        
        if (!game.initialize()) {
            std::cerr << "ゲームの初期化に失敗しました。" << std::endl;
            return 1;
        }        
        game.run();
        
    } catch (const std::exception& e) {
        std::cerr << "エラーが発生しました: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 