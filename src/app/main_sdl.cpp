#include "../core/SDL2Game.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --debug <state>    Start from specified state (for debugging)\n";
    std::cout << "                     Available states: room, town, night, night100, night1guard, night1resident, castle100, castle, demon, demon100, field, battle\n";
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
                std::cerr << "Error: --debug requires a state name (room, town, night, night100, night1guard, night1resident, castle100, castle, demon, demon100, field, battle)\n";
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