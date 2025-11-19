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
#include "../utils/TownLayout.h"
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
                // GameOverStateの場合は特別な処理
                if (currentState->getType() == StateType::GAME_OVER) {
                    // ゲームオーバーからの終了フラグを設定
                    player->setGameOverExit(true);
                    // フィールドの状態を設定（直前のフィールド状態が存在する場合はそれを使用、なければデフォルト）
                    // ここではフィールドの状態を設定しない（loadGameで処理される）
                } else if (currentState->getType() != StateType::BATTLE) {
                    // BattleState以外の通常のStateの場合
                    nlohmann::json stateJson = currentState->toJson();
                    player->setSavedGameState(stateJson);
                    // ゲームオーバーからの終了フラグをクリア
                    player->setGameOverExit(false);
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
    
    // セーブファイルからロードを試みる（デバッグモードの場合はスキップ）
    float nightTimer = 0.0f;
    bool nightTimerActive = false;
    bool loaded = false;
    if (debugStartState.empty()) {
        loaded = player->autoLoad(nightTimer, nightTimerActive);
    }
    
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
            // 夜の街のデバッグモード：レベル25のプレイヤーを設定
            // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
            // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
            // レベル25のステータス: HP=150, MP=44, Attack=56, Defense=51
            int targetLevel = 25;
            player->setLevel(targetLevel);
            
            // レベル25のステータスを計算して設定
            int baseHp = 30;
            int baseMp = 20;
            int baseAttack = 8;
            int baseDefense = 3;
            
            int levelUps = targetLevel - 1; // レベル1から25まで24回レベルアップ
            int maxHp = baseHp + levelUps * 5;
            int maxMp = baseMp + levelUps * 1;
            int attack = baseAttack + levelUps * 2;
            int defense = baseDefense + levelUps * 2;
            
            player->setMaxHp(maxHp);
            player->setMaxMp(maxMp);
            player->setAttack(attack);
            player->setDefense(defense);
            player->heal(maxHp); // HPを最大値に設定
            player->restoreMp(maxMp); // MPを最大値に設定
            
            // 進行状態を設定
            player->setCurrentNight(1); // 1夜目
            player->hasSeenRoomStory = true; // チュートリアル完了
            player->hasSeenTownExplanation = true;
            player->hasSeenFieldExplanation = true;
            player->hasSeenFieldFirstVictoryExplanation = true;
            player->hasSeenBattleExplanation = true;
            player->hasSeenNightExplanation = true; // 夜の説明も見たことにする
            
            // 信頼度を適切に設定（レベル25なので適度な信頼度）
            player->setKingTrust(50);
            player->setDemonTrust(50);
            
            // メンタルを適度に設定
            player->setMental(70);
            
            // 夜の回数を設定（夜の街から開始するので1夜目）
            TownState::s_nightCount = 1;
            // 目標レベルをs_nightCountに基づいて計算（1夜目なので50）
            TownState::s_targetLevel = 25 * (TownState::s_nightCount + 1);
            TownState::s_levelGoalAchieved = false;
            
            std::cout << "デバッグモード: レベル25のプレイヤーで夜の街から開始します" << std::endl;
            std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
            
            stateManager.changeState(std::make_unique<NightState>(player));
        } else if (debugStartState == "night100") {
            // 夜の街のデバッグモード（レベル100、住民全員倒した後）：レベル100のプレイヤーを設定
            // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
            // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
            // レベル100のステータス: HP=525, MP=119, Attack=206, Defense=199
            int targetLevel = 100;
            player->setLevel(targetLevel);
            
            // レベル100のステータスを計算して設定
            int baseHp = 30;
            int baseMp = 20;
            int baseAttack = 8;
            int baseDefense = 3;
            
            int levelUps = targetLevel - 1; // レベル1から100まで99回レベルアップ
            int maxHp = baseHp + levelUps * 5;
            int maxMp = baseMp + levelUps * 1;
            int attack = baseAttack + levelUps * 2;
            int defense = baseDefense + levelUps * 2;
            
            player->setMaxHp(maxHp);
            player->setMaxMp(maxMp);
            player->setAttack(attack);
            player->setDefense(defense);
            player->heal(maxHp); // HPを最大値に設定
            player->restoreMp(maxMp); // MPを最大値に設定
            
            // 進行状態を設定
            player->setCurrentNight(4); // 4夜目（住民を全て倒した後）
            player->hasSeenRoomStory = true; // チュートリアル完了
            player->hasSeenTownExplanation = true;
            player->hasSeenFieldExplanation = true;
            player->hasSeenFieldFirstVictoryExplanation = true;
            player->hasSeenBattleExplanation = true;
            player->hasSeenNightExplanation = true; // 夜の説明も見たことにする
            player->hasSeenResidentBattleExplanation = true; // 住民戦の説明も見たことにする
            
            // 信頼度を適切に設定（レベル100なので高い信頼度）
            player->setKingTrust(30); // 住民を倒したので王様からの信頼度は低い
            player->setDemonTrust(80); // 住民を倒したので魔王からの信頼度は高い
            
            // メンタルを適度に設定（住民を12人倒したので、6人以上なので+20*12=240、ただし上限100）
            player->setMental(100); // メンタルは最大値
            
            // 夜の回数を設定（住民を全て倒した後なので4夜目）
            TownState::s_nightCount = 4;
            // 目標レベルをs_nightCountに基づいて計算（4夜目なので125）
            TownState::s_targetLevel = 25 * (TownState::s_nightCount + 1);
            TownState::s_levelGoalAchieved = true; // 目標レベル達成済み
            
            // 全ての住民を倒した状態にする
            const auto& allResidentPositions = TownLayout::RESIDENTS;
            for (const auto& pos : allResidentPositions) {
                player->addKilledResident(pos.first, pos.second);
            }
            
            std::cout << "デバッグモード: レベル100のプレイヤーで夜の街から開始します（住民全員倒した後）" << std::endl;
            std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
            std::cout << "倒した住民数: " << allResidentPositions.size() << std::endl;
            
            auto nightState = std::make_unique<NightState>(player);
            stateManager.changeState(std::move(nightState));
        } else if (debugStartState == "castle100") {
            // 城のデバッグモード（レベル100、住民・衛兵全員倒した後）：レベル100のプレイヤーを設定
            // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
            // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
            // レベル100のステータス: HP=525, MP=119, Attack=206, Defense=199
            int targetLevel = 100;
            player->setLevel(targetLevel);
            
            // レベル100のステータスを計算して設定
            int baseHp = 30;
            int baseMp = 20;
            int baseAttack = 8;
            int baseDefense = 3;
            
            int levelUps = targetLevel - 1; // レベル1から100まで99回レベルアップ
            int maxHp = baseHp + levelUps * 5;
            int maxMp = baseMp + levelUps * 1;
            int attack = baseAttack + levelUps * 2;
            int defense = baseDefense + levelUps * 2;
            
            player->setMaxHp(maxHp);
            player->setMaxMp(maxMp);
            player->setAttack(attack);
            player->setDefense(defense);
            player->heal(maxHp); // HPを最大値に設定
            player->restoreMp(maxMp); // MPを最大値に設定
            
            // 進行状態を設定
            player->setCurrentNight(4); // 4夜目（住民を全て倒した後）
            player->hasSeenRoomStory = true; // チュートリアル完了
            player->hasSeenTownExplanation = true;
            player->hasSeenFieldExplanation = true;
            player->hasSeenFieldFirstVictoryExplanation = true;
            player->hasSeenBattleExplanation = true;
            player->hasSeenNightExplanation = true; // 夜の説明も見たことにする
            player->hasSeenResidentBattleExplanation = true; // 住民戦の説明も見たことにする
            player->hasSeenCastleStory = false; // 城の会話はまだ見ていない（fromNightStateなので表示される）
            
            // 信頼度を適切に設定（レベル100なので高い信頼度）
            player->setKingTrust(30); // 住民を倒したので王様からの信頼度は低い
            player->setDemonTrust(80); // 住民を倒したので魔王からの信頼度は高い
            
            // メンタルを適度に設定（住民を12人倒したので、6人以上なので+20*12=240、ただし上限100）
            player->setMental(100); // メンタルは最大値
            
            // 夜の回数を設定（住民を全て倒した後なので4夜目）
            TownState::s_nightCount = 4;
            // 目標レベルをs_nightCountに基づいて計算（4夜目なので125）
            TownState::s_targetLevel = 25 * (TownState::s_nightCount + 1);
            TownState::s_levelGoalAchieved = true; // 目標レベル達成済み
            
            // 全ての住民を倒した状態にする
            const auto& allResidentPositions = TownLayout::RESIDENTS;
            for (const auto& pos : allResidentPositions) {
                player->addKilledResident(pos.first, pos.second);
            }
            
            std::cout << "デバッグモード: レベル100のプレイヤーで城から開始します（住民・衛兵全員倒した後）" << std::endl;
            std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
            std::cout << "倒した住民数: " << allResidentPositions.size() << std::endl;
            
            // 城の状態から開始（fromNightState = true）
            stateManager.changeState(std::make_unique<CastleState>(player, true));
        } else if (debugStartState == "castle") {
            stateManager.changeState(std::make_unique<CastleState>(player, false));
        } else if (debugStartState == "demon") {
            stateManager.changeState(std::make_unique<DemonCastleState>(player, false));
        } else if (debugStartState == "demon100") {
            // 魔王の城のデバッグモード（レベル100、魔王との戦闘直前）：レベル100のプレイヤーを設定
            // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
            // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
            // レベル100のステータス: HP=525, MP=119, Attack=206, Defense=199
            int targetLevel = 100;
            player->setLevel(targetLevel);
            
            // レベル100のステータスを計算して設定
            int baseHp = 30;
            int baseMp = 20;
            int baseAttack = 8;
            int baseDefense = 3;
            
            int levelUps = targetLevel - 1; // レベル1から100まで99回レベルアップ
            int maxHp = baseHp + levelUps * 5;
            int maxMp = baseMp + levelUps * 1;
            int attack = baseAttack + levelUps * 2;
            int defense = baseDefense + levelUps * 2;
            
            player->setMaxHp(maxHp);
            player->setMaxMp(maxMp);
            player->setAttack(attack);
            player->setDefense(defense);
            player->heal(maxHp); // HPを最大値に設定
            player->restoreMp(maxMp); // MPを最大値に設定
            
            // 進行状態を設定
            player->setCurrentNight(4); // 4夜目（住民を全て倒した後）
            player->hasSeenRoomStory = true; // チュートリアル完了
            player->hasSeenTownExplanation = true;
            player->hasSeenFieldExplanation = true;
            player->hasSeenFieldFirstVictoryExplanation = true;
            player->hasSeenBattleExplanation = true;
            player->hasSeenNightExplanation = true; // 夜の説明も見たことにする
            player->hasSeenResidentBattleExplanation = true; // 住民戦の説明も見たことにする
            player->hasSeenCastleStory = true; // 城の会話は見たことにする
            player->hasSeenDemonCastleStory = false; // 魔王の城の会話はまだ見ていない（fromCastleStateなので表示される）
            
            // 信頼度を適切に設定（レベル100なので高い信頼度）
            player->setKingTrust(30); // 住民を倒したので王様からの信頼度は低い
            player->setDemonTrust(80); // 住民を倒したので魔王からの信頼度は高い
            
            // メンタルを適度に設定（住民を12人倒したので、6人以上なので+20*12=240、ただし上限100）
            player->setMental(100); // メンタルは最大値
            
            // 夜の回数を設定（住民を全て倒した後なので4夜目）
            TownState::s_nightCount = 4;
            // 目標レベルをs_nightCountに基づいて計算（4夜目なので125）
            TownState::s_targetLevel = 25 * (TownState::s_nightCount + 1);
            TownState::s_levelGoalAchieved = true; // 目標レベル達成済み
            
            // 全ての住民を倒した状態にする
            const auto& allResidentPositions = TownLayout::RESIDENTS;
            for (const auto& pos : allResidentPositions) {
                player->addKilledResident(pos.first, pos.second);
            }
            
            std::cout << "デバッグモード: レベル100のプレイヤーで魔王の城から開始します（魔王との戦闘直前）" << std::endl;
            std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
            std::cout << "倒した住民数: " << allResidentPositions.size() << std::endl;
            
            // 魔王の城の状態から開始（fromCastleState = true）
            stateManager.changeState(std::make_unique<DemonCastleState>(player, true));
        } else if (debugStartState == "night1guard") {
            // 夜の街のデバッグモード（レベル100、住民全員倒した後、衛兵1体のみ残り）：レベル100のプレイヤーを設定
            // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
            // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
            // レベル100のステータス: HP=525, MP=119, Attack=206, Defense=199
            int targetLevel = 100;
            player->setLevel(targetLevel);
            
            // レベル100のステータスを計算して設定
            int baseHp = 30;
            int baseMp = 20;
            int baseAttack = 8;
            int baseDefense = 3;
            
            int levelUps = targetLevel - 1; // レベル1から100まで99回レベルアップ
            int maxHp = baseHp + levelUps * 5;
            int maxMp = baseMp + levelUps * 1;
            int attack = baseAttack + levelUps * 2;
            int defense = baseDefense + levelUps * 2;
            
            player->setMaxHp(maxHp);
            player->setMaxMp(maxMp);
            player->setAttack(attack);
            player->setDefense(defense);
            player->heal(maxHp); // HPを最大値に設定
            player->restoreMp(maxMp); // MPを最大値に設定
            
            // 進行状態を設定
            player->setCurrentNight(4); // 4夜目（住民を全て倒した後）
            player->hasSeenRoomStory = true; // チュートリアル完了
            player->hasSeenTownExplanation = true;
            player->hasSeenFieldExplanation = true;
            player->hasSeenFieldFirstVictoryExplanation = true;
            player->hasSeenBattleExplanation = true;
            player->hasSeenNightExplanation = true; // 夜の説明も見たことにする
            player->hasSeenResidentBattleExplanation = true; // 住民戦の説明も見たことにする
            
            // 信頼度を適切に設定（レベル100なので高い信頼度）
            player->setKingTrust(30); // 住民を倒したので王様からの信頼度は低い
            player->setDemonTrust(80); // 住民を倒したので魔王からの信頼度は高い
            
            // メンタルを適度に設定（住民を12人倒したので、6人以上なので+20*12=240、ただし上限100）
            player->setMental(100); // メンタルは最大値
            
            // 夜の回数を設定（住民を全て倒した後なので4夜目）
            TownState::s_nightCount = 4;
            // 目標レベルをs_nightCountに基づいて計算（4夜目なので125）
            TownState::s_targetLevel = 25 * (TownState::s_nightCount + 1);
            TownState::s_levelGoalAchieved = true; // 目標レベル達成済み
            
            // 全ての住民を倒した状態にする
            const auto& allResidentPositions = TownLayout::RESIDENTS;
            for (const auto& pos : allResidentPositions) {
                player->addKilledResident(pos.first, pos.second);
            }
            
            std::cout << "デバッグモード: レベル100のプレイヤーで夜の街から開始します（住民全員倒した後、衛兵1体のみ残り）" << std::endl;
            std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
            std::cout << "倒した住民数: " << allResidentPositions.size() << std::endl;
            
            auto nightState = std::make_unique<NightState>(player);
            nightState->setRemainingGuards(1); // 衛兵を1体だけ残す
            stateManager.changeState(std::move(nightState));
        } else if (debugStartState == "night1resident") {
            // 夜の街のデバッグモード（レベル100、住民が残り1人の状態）：レベル100のプレイヤーを設定
            // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
            // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
            // レベル100のステータス: HP=525, MP=119, Attack=206, Defense=199
            int targetLevel = 100;
            player->setLevel(targetLevel);
            
            // レベル100のステータスを計算して設定
            int baseHp = 30;
            int baseMp = 20;
            int baseAttack = 8;
            int baseDefense = 3;
            
            int levelUps = targetLevel - 1; // レベル1から100まで99回レベルアップ
            int maxHp = baseHp + levelUps * 5;
            int maxMp = baseMp + levelUps * 1;
            int attack = baseAttack + levelUps * 2;
            int defense = baseDefense + levelUps * 2;
            
            player->setMaxHp(maxHp);
            player->setMaxMp(maxMp);
            player->setAttack(attack);
            player->setDefense(defense);
            player->heal(maxHp); // HPを最大値に設定
            player->restoreMp(maxMp); // MPを最大値に設定
            
            // 進行状態を設定
            player->setCurrentNight(4); // 4夜目
            player->hasSeenRoomStory = true; // チュートリアル完了
            player->hasSeenTownExplanation = true;
            player->hasSeenFieldExplanation = true;
            player->hasSeenFieldFirstVictoryExplanation = true;
            player->hasSeenBattleExplanation = true;
            player->hasSeenNightExplanation = true; // 夜の説明も見たことにする
            player->hasSeenResidentBattleExplanation = true; // 住民戦の説明も見たことにする
            
            // 信頼度を適切に設定（住民を11人倒したので）
            player->setKingTrust(30); // 住民を倒したので王様からの信頼度は低い
            player->setDemonTrust(80); // 住民を倒したので魔王からの信頼度は高い
            
            // メンタルを適度に設定（住民を11人倒したので、6人以上なので+20*11=220、ただし上限100）
            player->setMental(100); // メンタルは最大値
            
            // 夜の回数を設定
            TownState::s_nightCount = 4;
            // 目標レベルをs_nightCountに基づいて計算（4夜目なので125）
            TownState::s_targetLevel = 25 * (TownState::s_nightCount + 1);
            TownState::s_levelGoalAchieved = true; // 目標レベル達成済み
            
            // 住民を11人倒した状態にする（1人だけ残す）
            const auto& allResidentPositions = TownLayout::RESIDENTS;
            // 最後の1人以外を全て倒す
            for (size_t i = 0; i < allResidentPositions.size() - 1; i++) {
                player->addKilledResident(allResidentPositions[i].first, allResidentPositions[i].second);
            }
            
            std::cout << "デバッグモード: レベル100のプレイヤーで夜の街から開始します（住民が残り1人）" << std::endl;
            std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
            std::cout << "倒した住民数: " << (allResidentPositions.size() - 1) << " / " << allResidentPositions.size() << std::endl;
            
            auto nightState = std::make_unique<NightState>(player);
            stateManager.changeState(std::move(nightState));
        } else if (debugStartState == "field") {
            stateManager.changeState(std::make_unique<FieldState>(player));
        } else if (debugStartState == "battle") {
            // バトルから開始（スライムとの戦闘）
            auto enemy = std::make_unique<Enemy>(EnemyType::SLIME);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_slime") {
            // スライムとの戦闘（プレイヤーと敵ともにレベル1）
            int enemyLevel = 1;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::SLIME);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_goblin") {
            // ゴブリンとの戦闘（プレイヤーと敵ともにレベル5）
            int enemyLevel = 5;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::GOBLIN);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_orc") {
            // オークとの戦闘（プレイヤーと敵ともにレベル10）
            int enemyLevel = 10;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::ORC);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_dragon") {
            // ドラゴンとの戦闘（プレイヤーと敵ともにレベル15）
            int enemyLevel = 15;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::DRAGON);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_skeleton") {
            // スケルトンとの戦闘（プレイヤーと敵ともにレベル20）
            int enemyLevel = 20;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::SKELETON);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_ghost") {
            // ゴーストとの戦闘（プレイヤーと敵ともにレベル25）
            int enemyLevel = 25;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::GHOST);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_vampire") {
            // ヴァンパイアとの戦闘（プレイヤーと敵ともにレベル30）
            int enemyLevel = 30;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::VAMPIRE);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_demon_soldier") {
            // デーモンソルジャーとの戦闘（プレイヤーと敵ともにレベル35）
            int enemyLevel = 35;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::DEMON_SOLDIER);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_werewolf") {
            // ウェアウルフとの戦闘（プレイヤーと敵ともにレベル40）
            int enemyLevel = 40;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::WEREWOLF);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_minotaur") {
            // ミノタウロスとの戦闘（プレイヤーと敵ともにレベル45）
            int enemyLevel = 45;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::MINOTAUR);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_cyclops") {
            // サイクロプスとの戦闘（プレイヤーと敵ともにレベル50）
            int enemyLevel = 50;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::CYCLOPS);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_gargoyle") {
            // ガーゴイルとの戦闘（プレイヤーと敵ともにレベル55）
            int enemyLevel = 55;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::GARGOYLE);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_phantom") {
            // ファントムとの戦闘（プレイヤーと敵ともにレベル60）
            int enemyLevel = 60;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::PHANTOM);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_dark_knight") {
            // ダークナイトとの戦闘（プレイヤーと敵ともにレベル65）
            int enemyLevel = 65;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::DARK_KNIGHT);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_ice_giant") {
            // アイスジャイアントとの戦闘（プレイヤーと敵ともにレベル70）
            int enemyLevel = 70;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::ICE_GIANT);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_fire_demon") {
            // ファイアデーモンとの戦闘（プレイヤーと敵ともにレベル75）
            int enemyLevel = 75;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::FIRE_DEMON);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_shadow_lord") {
            // シャドウロードとの戦闘（プレイヤーと敵ともにレベル80）
            int enemyLevel = 80;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::SHADOW_LORD);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_ancient_dragon") {
            // エンシェントドラゴンとの戦闘（プレイヤーと敵ともにレベル85）
            int enemyLevel = 85;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::ANCIENT_DRAGON);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_chaos_beast") {
            // カオスビーストとの戦闘（プレイヤーと敵ともにレベル90）
            int enemyLevel = 90;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::CHAOS_BEAST);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_elder_god") {
            // エルダーゴッドとの戦闘（プレイヤーと敵ともにレベル95）
            int enemyLevel = 95;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::ELDER_GOD);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_demon_lord") {
            // 魔王との戦闘（プレイヤーと敵ともにレベル100）
            int enemyLevel = 100;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::DEMON_LORD);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_guard") {
            // 衛兵との戦闘（プレイヤーと敵ともにレベル105）
            int enemyLevel = 105;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::GUARD);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else if (debugStartState == "battle_king") {
            // 王様との戦闘（プレイヤーと敵ともにレベル1）
            int enemyLevel = 1;
            setupPlayerForBattle(player, enemyLevel);
            auto enemy = std::make_unique<Enemy>(EnemyType::KING);
            enemy->setLevel(enemyLevel);
            stateManager.changeState(std::make_unique<BattleState>(player, std::move(enemy)));
        } else {
            std::cerr << "警告: 不明なデバッグ状態 '" << debugStartState << "'。メインメニューから開始します。" << std::endl;
            std::cerr << "利用可能な状態: room, town, night, night100, night1guard, castle100, castle, demon, demon100, field, battle" << std::endl;
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
    graphics.loadTexture("assets/textures/characters/king.png", "enemy_王様"); // 戦闘画面用
    graphics.loadTexture("assets/textures/characters/guard.png", "guard");
    graphics.loadTexture("assets/textures/characters/guard.png", "enemy_衛兵"); // 戦闘画面用
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
    graphics.loadTexture("assets/textures/enemies/ork.png", "enemy_オーク");
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
    graphics.loadTexture("assets/textures/UI/life.png", "life");
    // オブジェクト画像
    graphics.loadTexture("assets/textures/objects/bed.png", "bed");
    graphics.loadTexture("assets/textures/objects/desk.png", "desk");
    graphics.loadTexture("assets/textures/objects/closed_box.png", "closed_box");
    graphics.loadTexture("assets/textures/objects/open_box.png", "open_box");
}

void SDL2Game::setupPlayerForBattle(std::shared_ptr<Player> player, int level) {
    // プレイヤーのレベルを設定
    player->setLevel(level);
    
    // 初期ステータス: HP=30, MP=20, Attack=8, Defense=3, Level=1
    // レベルアップ増加: HP+5, MP+1, Attack+2, Defense+2 per level
    int baseHp = 30;
    int baseMp = 20;
    int baseAttack = 8;
    int baseDefense = 3;
    
    int levelUps = level - 1; // レベル1から指定レベルまで
    int maxHp = baseHp + levelUps * 5;
    int maxMp = baseMp + levelUps * 1;
    int attack = baseAttack + levelUps * 2;
    int defense = baseDefense + levelUps * 2;
    
    player->setMaxHp(maxHp);
    player->setMaxMp(maxMp);
    player->setAttack(attack);
    player->setDefense(defense);
    player->heal(maxHp); // HPを最大値に設定
    player->restoreMp(maxMp); // MPを最大値に設定
    
    // 説明を見たことにする
    player->hasSeenRoomStory = true;
    player->hasSeenTownExplanation = true;
    player->hasSeenFieldExplanation = true;
    player->hasSeenFieldFirstVictoryExplanation = true;
    player->hasSeenBattleExplanation = true;
    player->hasSeenNightExplanation = true;
    player->hasSeenResidentBattleExplanation = true;
    
    std::cout << "デバッグモード: レベル" << level << "のプレイヤーで戦闘を開始します" << std::endl;
    std::cout << "ステータス: HP=" << maxHp << ", MP=" << maxMp << ", Attack=" << attack << ", Defense=" << defense << std::endl;
} 