#include "SDL2Game.h"
#include "../game/MainMenuState.h"
#include "../game/RoomState.h"
#include "../game/CastleState.h"
#include "../game/TownState.h"
#include "../game/NightState.h"
#include "../game/DemonCastleState.h"
#include "../game/FieldState.h"
#include "../game/BattleState.h"
#include "../entities/Enemy.h"
#include "../core/utils/ui_config_manager.h"
#include "../core/GameState.h"
#include <iostream>
#include <string>
#include <memory>

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
            // 終了前にセーブ
            if (player) {
                float nightTimer = TownState::s_nightTimer;
                bool nightTimerActive = TownState::s_nightTimerActive;
                // 現在のStateの状態を取得して保存
                GameState* currentState = stateManager.getCurrentState();
                if (currentState) {
                    // BattleStateの場合は保存しない（直前のフィールド状態が既に保存されている）
                    if (currentState->getType() != StateType::BATTLE) {
                        nlohmann::json stateJson = currentState->toJson();
                        player->setSavedGameState(stateJson);
                    }
                }
                player->saveGame("autosave.json", nightTimer, nightTimerActive);
            }
            isRunning = false;
            break;
        }
        
        inputManager.handleEvent(event);
    }
    
    if (inputManager.isKeyJustPressed(InputKey::ESCAPE)) {
        // 終了前にセーブ
        if (player) {
            float nightTimer = TownState::s_nightTimer;
            bool nightTimerActive = TownState::s_nightTimerActive;
            // 現在のStateの状態を取得して保存
            GameState* currentState = stateManager.getCurrentState();
            if (currentState) {
                // BattleStateの場合は保存しない（直前のフィールド状態が既に保存されている）
                if (currentState->getType() != StateType::BATTLE) {
                    nlohmann::json stateJson = currentState->toJson();
                    player->setSavedGameState(stateJson);
                }
            }
            player->saveGame("autosave.json", nightTimer, nightTimerActive);
        }
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
    std::string playerName = "勇者";
    
    if (!debugStartState.empty()) {
        std::cout << "デバッグモード: " << debugStartState << " から開始します" << std::endl;
    }
    
    player = std::make_shared<Player>(playerName);
    
    // セーブファイルからロードを試みる
    float nightTimer = 0.0f;
    bool nightTimerActive = false;
    bool loaded = player->autoLoad(nightTimer, nightTimerActive);
    
    if (loaded) {
        // セーブファイルからロード成功
        TownState::s_nightTimer = nightTimer;
        TownState::s_nightTimerActive = nightTimerActive;
        
        // 保存されたゲーム状態を取得
        const nlohmann::json* savedState = player->getSavedGameState();
        if (savedState && !savedState->is_null() && savedState->contains("stateType")) {
            StateType savedStateType = static_cast<StateType>((*savedState)["stateType"]);
            
            // 保存されたStateに応じて適切なStateを作成
            switch (savedStateType) {
                case StateType::ROOM: {
                    auto roomState = std::make_unique<RoomState>(player);
                    roomState->fromJson(*savedState);
                    stateManager.changeState(std::move(roomState));
                    break;
                }
                case StateType::TOWN: {
                    auto townState = std::make_unique<TownState>(player);
                    townState->fromJson(*savedState);
                    stateManager.changeState(std::move(townState));
                    break;
                }
                case StateType::CASTLE: {
                    bool fromNightState = (*savedState).contains("fromNightState") ? (*savedState)["fromNightState"].get<bool>() : false;
                    auto castleState = std::make_unique<CastleState>(player, fromNightState);
                    castleState->fromJson(*savedState);
                    stateManager.changeState(std::move(castleState));
                    break;
                }
                case StateType::DEMON_CASTLE: {
                    bool fromCastleState = (*savedState).contains("fromCastleState") ? (*savedState)["fromCastleState"].get<bool>() : false;
                    auto demonState = std::make_unique<DemonCastleState>(player, fromCastleState);
                    demonState->fromJson(*savedState);
                    stateManager.changeState(std::move(demonState));
                    break;
                }
                case StateType::FIELD: {
                    auto fieldState = std::make_unique<FieldState>(player);
                    fieldState->fromJson(*savedState);
                    stateManager.changeState(std::move(fieldState));
                    break;
                }
                case StateType::NIGHT: {
                    auto nightState = std::make_unique<NightState>(player);
                    nightState->fromJson(*savedState);
                    stateManager.changeState(std::move(nightState));
                    break;
                }
                default:
                    // その他のStateはメインメニューから開始
                    stateManager.changeState(std::make_unique<MainMenuState>(player));
                    break;
            }
            return; // セーブファイルから復元したので、デバッグモードや通常の開始処理はスキップ
        }
    }
    
    // セーブファイルがない、またはデバッグモードの場合
    if (!debugStartState.empty()) {
        if (debugStartState == "room") {
            stateManager.changeState(std::make_unique<RoomState>(player));
        } else if (debugStartState == "town") {
            stateManager.changeState(std::make_unique<TownState>(player));
        } else if (debugStartState == "night") {
            stateManager.changeState(std::make_unique<NightState>(player));
        } else if (debugStartState == "castle") {
            stateManager.changeState(std::make_unique<CastleState>(player, false));
        } else if (debugStartState == "demon") {
            stateManager.changeState(std::make_unique<DemonCastleState>(player, false));
        } else if (debugStartState == "field") {
            stateManager.changeState(std::make_unique<FieldState>(player));
        } else if (debugStartState == "battle") {
            // バトルから開始（スライムとの戦闘）
            auto enemy = std::make_unique<Enemy>(EnemyType::SLIME);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else {
            std::cerr << "警告: 不明なデバッグ状態 '" << debugStartState << "'。メインメニューから開始します。" << std::endl;
            std::cerr << "利用可能な状態: room, town, night, castle, demon, field, battle" << std::endl;
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
    graphics.loadTexture("assets/textures/characters/player_defeat.png", "player_defeat");
    graphics.loadTexture("assets/textures/characters/player_captured.png", "player_captured");
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
    
    // UI画像
    graphics.loadTexture("assets/textures/UI/Rock-Paper-Scissors.png", "rock_paper_scissors");
    graphics.loadTexture("assets/textures/UI/attack.png", "command_attack");
    graphics.loadTexture("assets/textures/UI/defend.png", "command_defend");
    graphics.loadTexture("assets/textures/UI/magic.png", "command_magic");
    graphics.loadTexture("assets/textures/UI/hide.png", "command_hide");
    graphics.loadTexture("assets/textures/UI/fear.png", "command_fear");
    graphics.loadTexture("assets/textures/UI/help.png", "command_help");
    graphics.loadTexture("assets/textures/UI/vs.png", "vs_image");
    graphics.loadTexture("assets/textures/UI/title_logo.png", "title_logo");
    graphics.loadTexture("assets/textures/UI/title_bg.png", "title_bg");
    // オブジェクト画像
    graphics.loadTexture("assets/textures/objects/bed.png", "bed");
    graphics.loadTexture("assets/textures/objects/desk.png", "desk");
    graphics.loadTexture("assets/textures/objects/closed_box.png", "closed_box");
    graphics.loadTexture("assets/textures/objects/open_box.png", "open_box");
} 