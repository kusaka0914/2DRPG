#pragma once
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "GameState.h"
#include "../entities/Player.h"
#include <memory>
#include <chrono>

class SDL2Game {
private:
    Graphics graphics;
    InputManager inputManager;
    GameStateManager stateManager;
    std::shared_ptr<Player> player;
    
    bool isRunning;
    std::chrono::high_resolution_clock::time_point lastTime;
    
    // UI設定ファイルのホットリロード用タイマー
    float uiConfigCheckTimer;
    const float UI_CONFIG_CHECK_INTERVAL = 0.1f;  // 0.1秒ごとにチェック（より頻繁にチェック）
    
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;

public:
    SDL2Game();
    ~SDL2Game();
    
    bool initialize();
    void run();
    void cleanup();
    
private:
    void handleEvents();
    void update(float deltaTime);
    void render();
    void initializeGame();
    void loadResources();
    void loadGameImages();
    float calculateDeltaTime();
}; 