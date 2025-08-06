#include "SDL2Game.h"
#include "MainMenuState.h"
#include "RoomState.h"
#include "CastleState.h"
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
    
    // 自室から冒険開始（RoomStateから開始）
    stateManager.changeState(std::make_unique<RoomState>(player));
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
    graphics.loadTexture("assets/characters/king.png", "king");
    graphics.loadTexture("assets/characters/guard.png", "guard");
    graphics.loadTexture("assets/characters/demon.png", "demon");
    
    // 敵画像
    graphics.loadTexture("assets/enemies/slime.png", "enemy_スライム");
    graphics.loadTexture("assets/enemies/goblin.png", "enemy_ゴブリン");
    graphics.loadTexture("assets/enemies/orc.png", "enemy_オーク");
    graphics.loadTexture("assets/enemies/dragon.png", "enemy_ドラゴン");
    
    // フィールド用タイル画像
    graphics.loadTexture("assets/tiles/grass.png", "grass");
    graphics.loadTexture("assets/tiles/forest.png", "forest");
    graphics.loadTexture("assets/tiles/river.png", "river");
    graphics.loadTexture("assets/tiles/bridge.png", "bridge");
    graphics.loadTexture("assets/tiles/rock.png", "rock");
    graphics.loadTexture("assets/tiles/town_entrance.png", "town_entrance");
    
    // 建物画像
    graphics.loadTexture("assets/buildings/house.png", "house");
    graphics.loadTexture("assets/buildings/castle.png", "castle");
    
    // 建物用タイル画像（フィールド用）
    if (graphics.loadTexture("assets/tiles/housetile.png", "house_tile")) {
        std::cout << "自室タイル画像読み込み成功" << std::endl;
    } else {
        std::cout << "自室タイル画像読み込み失敗" << std::endl;
    }
    if (graphics.loadTexture("assets/tiles/castletile.png", "castle_tile")) {
        std::cout << "城タイル画像読み込み成功" << std::endl;
    } else {
        std::cout << "城タイル画像読み込み失敗" << std::endl;
    }
    if (graphics.loadTexture("assets/tiles/demoncastletile.png", "demon_castle_tile")) {
        std::cout << "魔王の城タイル画像読み込み成功" << std::endl;
    } else {
        std::cout << "魔王の城タイル画像読み込み失敗" << std::endl;
    }
    
    // オブジェクト画像
    graphics.loadTexture("assets/objects/bed.png", "bed");
    graphics.loadTexture("assets/objects/desk.png", "desk");
    graphics.loadTexture("assets/objects/closed_box.png", "closed_box");
    graphics.loadTexture("assets/objects/open_box.png", "open_box");
    
    std::cout << "画像リソースの読み込みを試行しました（画像ファイルが存在する場合は読み込み成功）。" << std::endl;
} 