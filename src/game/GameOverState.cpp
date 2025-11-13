#include "GameOverState.h"
#include "MainMenuState.h"
#include "NightState.h"
#include "FieldState.h"
#include "TownState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>

GameOverState::GameOverState(std::shared_ptr<Player> player, const std::string& reason)
    : player(player), gameOverReason(reason), titleLabel(nullptr), reasonLabel(nullptr), instruction(nullptr) {
}

void GameOverState::enter() {
    setupUI();
}

void GameOverState::exit() {
    
}

void GameOverState::update(float deltaTime) {
    ui.update(deltaTime);
    
    // ホットリロードチェック：設定が変更された場合はUIを再初期化
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    // リロードが発生した場合（前回falseで今回trueになった場合）
    if (!lastReloadState && currentReloadState) {
        setupUI();
    }
    lastReloadState = currentReloadState;
}

void GameOverState::render(Graphics& graphics) {
    // 黒い背景
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(0, 0, 1100, 650, true);
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void GameOverState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // スペースキーまたはAボタンでリスタート
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (stateManager) {
            // オートセーブデータをロード
            float nightTimer;
            bool nightTimerActive;
            if (player->autoLoad(nightTimer, nightTimerActive)) {
                // デバッグ出力
                std::cout << "GameOverState: タイマー情報を復元 - Timer: " << nightTimer 
                          << ", Active: " << (nightTimerActive ? "true" : "false") << std::endl;
                
                // プレイヤーのHPとMPを全回復
                player->heal(player->getMaxHp());
                player->restoreMp(player->getMaxMp());
                
                std::cout << "GameOverState: プレイヤーHP復元 - HP: " << player->getHp() 
                          << "/" << player->getMaxHp() << ", isAlive: " << (player->getIsAlive() ? "true" : "false") << std::endl;
                
                // ゲームオーバー理由に応じて適切な場所に戻る
                if (gameOverReason.find("戦闘に敗北") != std::string::npos) {
                    // 戦闘敗北の場合はフィールドに戻る
                    stateManager->changeState(std::make_unique<FieldState>(player));
                } else {
                    // 夜の街での敗北の場合は夜の街に戻る
                    stateManager->changeState(std::make_unique<NightState>(player));
                }
                
                // タイマー情報を静的変数に復元
                TownState::s_nightTimerActive = nightTimerActive;
                TownState::s_nightTimer = nightTimer;
                
                std::cout << "GameOverState: 静的変数に復元 - s_nightTimer: " << TownState::s_nightTimer 
                          << ", s_nightTimerActive: " << (TownState::s_nightTimerActive ? "true" : "false") << std::endl;
            } else {
                // ロード失敗した場合はメインメニューに戻る
                auto newPlayer = std::make_shared<Player>("勇者");
                // リスタート時に王様からの信頼度を50にリセット
                newPlayer->setKingTrust(50);
                // 夜の回数を1に設定
                newPlayer->setCurrentNight(1);
                stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
            }
        }
    }
}

void GameOverState::setupUI() {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto gameOverConfig = config.getGameOverConfig();
    
    // タイトル（JSONから座標を取得）
    int titleX, titleY;
    config.calculatePosition(titleX, titleY, gameOverConfig.title.position, 1100, 650);
    auto titleLabelPtr = std::make_unique<Label>(titleX, titleY, "GAME OVER", "default");
    titleLabelPtr->setColor(gameOverConfig.title.color);
    titleLabel = titleLabelPtr.get();
    ui.addElement(std::move(titleLabelPtr));
    
    // ゲームオーバー理由（JSONから座標を取得）
    int reasonX, reasonY;
    config.calculatePosition(reasonX, reasonY, gameOverConfig.reason.position, 1100, 650);
    auto reasonLabelPtr = std::make_unique<Label>(reasonX, reasonY, gameOverReason, "default");
    reasonLabelPtr->setColor(gameOverConfig.reason.color);
    reasonLabel = reasonLabelPtr.get();
    ui.addElement(std::move(reasonLabelPtr));
    
    // 操作説明（JSONから座標を取得）
    int instructionX, instructionY;
    config.calculatePosition(instructionX, instructionY, gameOverConfig.instruction.position, 1100, 650);
    auto instructionLabelPtr = std::make_unique<Label>(instructionX, instructionY, "スペースキーまたはAボタンで夜の街に再スタート", "default");
    instructionLabelPtr->setColor(gameOverConfig.instruction.color);
    instruction = instructionLabelPtr.get();
    ui.addElement(std::move(instructionLabelPtr));
} 