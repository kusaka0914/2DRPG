#include "SDL2Game.h"
#include "MainMenuState.h"
#include <iostream>
#include <string>

SDL2Game::SDL2Game() : isRunning(false) {
}

SDL2Game::~SDL2Game() {
    cleanup();
}

bool SDL2Game::initialize() {
    if (!graphics.initialize("ドラクエ風RPG", SCREEN_WIDTH, SCREEN_HEIGHT)) {
        std::cerr << "グラフィックス初期化に失敗しました。" << std::endl;
        return false;
    }
    
    loadResources();
    initializeGame();
    
    lastTime = std::chrono::high_resolution_clock::now();
    isRunning = true;
    
    return true;
}

void SDL2Game::run() {
    while (isRunning) {
        float deltaTime = calculateDeltaTime();
        
        handleEvents();
        update(deltaTime);
        render();
    }
}

void SDL2Game::cleanup() {
    graphics.cleanup();
}

void SDL2Game::handleEvents() {
    inputManager.update();
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false;
            break;
        }
        
        inputManager.handleEvent(event);
    }
    
    // 重要：イベントをゲームステートに渡す
    stateManager.handleInput(inputManager);
}

void SDL2Game::update(float deltaTime) {
    stateManager.update(deltaTime);
}

void SDL2Game::render() {
    stateManager.render(graphics);
}

void SDL2Game::initializeGame() {
    // プレイヤー名の入力（簡略化）
    std::string playerName;
    std::cout << "勇者の名前を入力してください: ";
    std::getline(std::cin, playerName);
    
    if (playerName.empty()) {
        playerName = "勇者";
    }
    
    player = std::make_shared<Player>(playerName);
    
    // 元の構成に戻す（MainMenuStateから開始）
    stateManager.changeState(std::make_unique<MainMenuState>(player));
}

void SDL2Game::loadResources() {
    // 確実に日本語をサポートするフォントを優先的に読み込み
    bool fontLoaded = false;
    
    // 1. ヒラギノ角ゴシック W3 - 確実な日本語フォント
    if (graphics.loadFont("/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc", 16, "default")) {
        fontLoaded = true;
        std::cout << "フォント読み込み成功: ヒラギノ角ゴシック (確実な日本語対応)" << std::endl;
    }
    // 2. ヒラギノ角ゴシック W6 - バックアップ日本語フォント
    else if (graphics.loadFont("/System/Library/Fonts/ヒラギノ角ゴシック W6.ttc", 16, "default")) {
        fontLoaded = true;
        std::cout << "フォント読み込み成功: ヒラギノ角ゴシック W6 (日本語対応)" << std::endl;
    }
    // 3. ヒラギノ丸ゴ ProN - 丸ゴシック日本語フォント
    else if (graphics.loadFont("/System/Library/Fonts/ヒラギノ丸ゴ ProN W4.ttc", 16, "default")) {
        fontLoaded = true;
        std::cout << "フォント読み込み成功: ヒラギノ丸ゴ (日本語対応)" << std::endl;
    }
    // 4. Hiragino Sans GB - 最後の手段
    else if (graphics.loadFont("/System/Library/Fonts/Hiragino Sans GB.ttc", 16, "default")) {
        fontLoaded = true;
        std::cout << "フォント読み込み成功: Hiragino Sans GB (CJK対応)" << std::endl;
    }
    
    if (!fontLoaded) {
        std::cout << "警告: 日本語フォントが読み込めませんでした。テキストは表示されません。" << std::endl;
    }
    
    // 画像リソース読み込み
    loadGameImages();
    
    std::cout << "リソースの読み込みが完了しました。" << std::endl;
}

float SDL2Game::calculateDeltaTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    // デルタタイムの上限を設定（ゲームが一時停止した場合の対策）
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }
    
    return deltaTime;
}

void SDL2Game::loadGameImages() {
    // キャラクター画像
    graphics.loadTexture("assets/characters/player.png", "player");
    graphics.loadTexture("assets/characters/player_field.png", "player_field");
    
    // 敵画像
    graphics.loadTexture("assets/enemies/スライム.png", "enemy_スライム");
    graphics.loadTexture("assets/enemies/ゴブリン.png", "enemy_ゴブリン");
    graphics.loadTexture("assets/enemies/オーク.png", "enemy_オーク");
    graphics.loadTexture("assets/enemies/ドラゴン.png", "enemy_ドラゴン");
    
    std::cout << "画像リソースの読み込みを試行しました（画像ファイルが存在する場合は読み込み成功）。" << std::endl;
} 