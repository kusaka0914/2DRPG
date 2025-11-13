#include "../core/SDL2Game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc; // 未使用パラメータの警告を抑制
    (void)argv;
    
    try {
        SDL2Game game;
        
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