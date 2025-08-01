#pragma once
#include "Graphics.h"
#include "InputManager.h"
#include "GameState.h"
#include "Player.h"
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
    
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;

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