#include "SDL2Game.h"
#include "../game/MainMenuState.h"
#include "../game/RoomState.h"
#include "../game/CastleState.h"
#include "../game/TownState.h"
#include "../game/DemonCastleState.h"
#include "../game/FieldState.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>
#include <string>

SDL2Game::SDL2Game() : isRunning(false), uiConfigCheckTimer(0.0f), debugStartState("") {
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
    
    UIConfig::UIConfigManager::getInstance().loadConfig("assets/config/ui_config.json");
    
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
    
    if (inputManager.isKeyJustPressed(InputKey::ESCAPE)) {
        isRunning = false;
        return;
    }
    
    stateManager.handleInput(inputManager);
}

void SDL2Game::update(float deltaTime) {
    uiConfigCheckTimer += deltaTime;
    if (uiConfigCheckTimer >= UI_CONFIG_CHECK_INTERVAL) {
        uiConfigCheckTimer = 0.0f;
        UIConfig::UIConfigManager::getInstance().checkAndReloadConfig();
    }
    
    stateManager.update(deltaTime);
}

void SDL2Game::render() {
    stateManager.render(graphics);
}

void SDL2Game::setDebugStartState(const std::string& state) {
    debugStartState = state;
}

void SDL2Game::initializeGame() {
    std::string playerName;
    if (debugStartState.empty()) {
    std::cout << "勇者の名前を入力してください: ";
    std::getline(std::cin, playerName);
    } else {
        playerName = "デバッガー";
        std::cout << "デバッグモード: " << debugStartState << " から開始します" << std::endl;
    }
    
    if (playerName.empty()) {
        playerName = "勇者";
    }
    
    player = std::make_shared<Player>(playerName);
    
    if (!debugStartState.empty()) {
        if (debugStartState == "room") {
            stateManager.changeState(std::make_unique<RoomState>(player));
        } else if (debugStartState == "town") {
            stateManager.changeState(std::make_unique<TownState>(player));
        } else if (debugStartState == "castle") {
            stateManager.changeState(std::make_unique<CastleState>(player, false));
        } else if (debugStartState == "demon") {
            stateManager.changeState(std::make_unique<DemonCastleState>(player, false));
        } else if (debugStartState == "field") {
            stateManager.changeState(std::make_unique<FieldState>(player));
        } else {
            std::cerr << "警告: 不明なデバッグ状態 '" << debugStartState << "'。メインメニューから開始します。" << std::endl;
            std::cerr << "利用可能な状態: room, town, castle, demon, field" << std::endl;
            stateManager.changeState(std::make_unique<MainMenuState>(player));
        }
    } else {
    stateManager.changeState(std::make_unique<MainMenuState>(player));
    }
}

void SDL2Game::loadResources() {
    bool fontLoaded = false;
    
    // 1. ヒラギノ角ゴシック W3 - 確実な日本語フォント
    if (graphics.loadFont("/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc", 16, "default")) {
        fontLoaded = true;
    }
    // 2. ヒラギノ角ゴシック W6 - バックアップ日本語フォント
    else if (graphics.loadFont("/System/Library/Fonts/ヒラギノ角ゴシック W6.ttc", 16, "default")) {
        fontLoaded = true;
    }
    // 3. ヒラギノ丸ゴ ProN - 丸ゴシック日本語フォント
    else if (graphics.loadFont("/System/Library/Fonts/ヒラギノ丸ゴ ProN W4.ttc", 16, "default")) {
        fontLoaded = true;
    }
    
    if (fontLoaded) {
        if (graphics.loadFont("/System/Library/Fonts/ヒラギノ角ゴシック W6.ttc", 32, "title")) {
        }
    }
    else if (graphics.loadFont("/System/Library/Fonts/Hiragino Sans GB.ttc", 16, "default")) {
        fontLoaded = true;
    }
    // 画像リソース読み込み
    loadGameImages();
}

float SDL2Game::calculateDeltaTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }
    
    return deltaTime;
}

void SDL2Game::loadGameImages() {
    graphics.loadTexture("assets/textures/characters/player.png", "player");
    graphics.loadTexture("assets/textures/characters/player_field.png", "player_field");
    graphics.loadTexture("assets/textures/characters/king.png", "king");
    graphics.loadTexture("assets/textures/characters/guard.png", "guard");
    graphics.loadTexture("assets/textures/characters/demon.png", "demon");
    
    // 住人画像
    graphics.loadTexture("assets/textures/characters/resident_1.png", "resident_1");
    graphics.loadTexture("assets/textures/characters/resident_2.png", "resident_2");
    graphics.loadTexture("assets/textures/characters/resident_3.png", "resident_3");
    graphics.loadTexture("assets/textures/characters/resident_4.png", "resident_4");
    graphics.loadTexture("assets/textures/characters/resident_5.png", "resident_5");
    graphics.loadTexture("assets/textures/characters/resident_6.png", "resident_6");
    
    graphics.loadTexture("assets/textures/enemies/slime.png", "enemy_スライム");
    graphics.loadTexture("assets/textures/enemies/goblin.png", "enemy_ゴブリン");
    graphics.loadTexture("assets/textures/enemies/orc.png", "enemy_オーク");
    graphics.loadTexture("assets/textures/enemies/dragon.png", "enemy_ドラゴン");
    graphics.loadTexture("assets/textures/enemies/skeleton.png", "enemy_スケルトン");
    graphics.loadTexture("assets/textures/enemies/ghost.png", "enemy_ゴースト");
    graphics.loadTexture("assets/textures/enemies/vampire.png", "enemy_ヴァンパイア");
    graphics.loadTexture("assets/textures/enemies/demon_soldier.png", "enemy_デーモンソルジャー");
    graphics.loadTexture("assets/textures/enemies/werewolf.png", "enemy_ウェアウルフ");
    graphics.loadTexture("assets/textures/enemies/minotaur.png", "enemy_ミノタウロス");
    graphics.loadTexture("assets/textures/enemies/cyclops.png", "enemy_サイクロプス");
    graphics.loadTexture("assets/textures/enemies/gargoyle.png", "enemy_ガーゴイル");
    graphics.loadTexture("assets/textures/enemies/phantom.png", "enemy_ファントム");
    graphics.loadTexture("assets/textures/enemies/dark_knight.png", "enemy_ダークナイト");
    graphics.loadTexture("assets/textures/enemies/ice_giant.png", "enemy_アイスジャイアント");
    graphics.loadTexture("assets/textures/enemies/fire_demon.png", "enemy_ファイアデーモン");
    graphics.loadTexture("assets/textures/enemies/shadow_lord.png", "enemy_シャドウロード");
    graphics.loadTexture("assets/textures/enemies/ancient_dragon.png", "enemy_エンシェントドラゴン");
    graphics.loadTexture("assets/textures/enemies/chaos_beast.png", "enemy_カオスビースト");
    graphics.loadTexture("assets/textures/enemies/elder_god.png", "enemy_エルダーゴッド");
    graphics.loadTexture("assets/textures/characters/demon.png", "enemy_魔王");
    
    // フィールド用タイル画像
    graphics.loadTexture("assets/textures/tiles/grass.png", "grass");
    graphics.loadTexture("assets/textures/tiles/forest.png", "forest");
    graphics.loadTexture("assets/textures/tiles/river.png", "river");
    graphics.loadTexture("assets/textures/tiles/bridge.png", "bridge");
    graphics.loadTexture("assets/textures/tiles/rock.png", "rock");
    // graphics.loadTexture("assets/textures/tiles/town_entrance.png", "town_entrance");
    
    // 建物画像
    graphics.loadTexture("assets/textures/buildings/house.png", "house");
    graphics.loadTexture("assets/textures/buildings/castle.png", "castle");
    
    // 鳥居画像
    graphics.loadTexture("assets/textures/objects/torii.png", "torii");
    
    graphics.loadTexture("assets/textures/buildings/resident_home.png", "resident_home");
    // オブジェクト画像
    graphics.loadTexture("assets/textures/objects/bed.png", "bed");
    graphics.loadTexture("assets/textures/objects/desk.png", "desk");
    graphics.loadTexture("assets/textures/objects/closed_box.png", "closed_box");
    graphics.loadTexture("assets/textures/objects/open_box.png", "open_box");
} 