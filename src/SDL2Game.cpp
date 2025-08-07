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
    }
    // 2. ヒラギノ角ゴシック W6 - バックアップ日本語フォント
    else if (graphics.loadFont("/System/Library/Fonts/ヒラギノ角ゴシック W6.ttc", 16, "default")) {
        fontLoaded = true;
    }
    // 3. ヒラギノ丸ゴ ProN - 丸ゴシック日本語フォント
    else if (graphics.loadFont("/System/Library/Fonts/ヒラギノ丸ゴ ProN W4.ttc", 16, "default")) {
        fontLoaded = true;
    }
    // 4. Hiragino Sans GB - 最後の手段
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
    
    // 住人画像
    graphics.loadTexture("assets/characters/resident_1.png", "resident_1");
    graphics.loadTexture("assets/characters/resident_2.png", "resident_2");
    graphics.loadTexture("assets/characters/resident_3.png", "resident_3");
    graphics.loadTexture("assets/characters/resident_4.png", "resident_4");
    graphics.loadTexture("assets/characters/resident_5.png", "resident_5");
    graphics.loadTexture("assets/characters/resident_6.png", "resident_6");
    
    // 敵画像
    graphics.loadTexture("assets/enemies/slime.png", "enemy_スライム");
    graphics.loadTexture("assets/enemies/goblin.png", "enemy_ゴブリン");
    graphics.loadTexture("assets/enemies/orc.png", "enemy_オーク");
    graphics.loadTexture("assets/enemies/dragon.png", "enemy_ドラゴン");
    graphics.loadTexture("assets/enemies/skeleton.png", "enemy_スケルトン");
    graphics.loadTexture("assets/enemies/ghost.png", "enemy_ゴースト");
    graphics.loadTexture("assets/enemies/vampire.png", "enemy_ヴァンパイア");
    graphics.loadTexture("assets/enemies/demon_soldier.png", "enemy_デーモンソルジャー");
    graphics.loadTexture("assets/enemies/werewolf.png", "enemy_ウェアウルフ");
    graphics.loadTexture("assets/enemies/minotaur.png", "enemy_ミノタウロス");
    graphics.loadTexture("assets/enemies/cyclops.png", "enemy_サイクロプス");
    graphics.loadTexture("assets/enemies/gargoyle.png", "enemy_ガーゴイル");
    graphics.loadTexture("assets/enemies/phantom.png", "enemy_ファントム");
    graphics.loadTexture("assets/enemies/dark_knight.png", "enemy_ダークナイト");
    graphics.loadTexture("assets/enemies/ice_giant.png", "enemy_アイスジャイアント");
    graphics.loadTexture("assets/enemies/fire_demon.png", "enemy_ファイアデーモン");
    graphics.loadTexture("assets/enemies/shadow_lord.png", "enemy_シャドウロード");
    graphics.loadTexture("assets/enemies/ancient_dragon.png", "enemy_エンシェントドラゴン");
    graphics.loadTexture("assets/enemies/chaos_beast.png", "enemy_カオスビースト");
    graphics.loadTexture("assets/enemies/elder_god.png", "enemy_エルダーゴッド");
    
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
    
    // 鳥居画像
    graphics.loadTexture("assets/objects/torii.png", "torii");
    
    // 住人の家画像
    graphics.loadTexture("assets/buildings/resident_home.png", "resident_home");
    // オブジェクト画像
    graphics.loadTexture("assets/objects/bed.png", "bed");
    graphics.loadTexture("assets/objects/desk.png", "desk");
    graphics.loadTexture("assets/objects/closed_box.png", "closed_box");
    graphics.loadTexture("assets/objects/open_box.png", "open_box");
} 