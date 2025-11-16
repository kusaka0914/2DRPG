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
    
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    if (!lastReloadState && currentReloadState) {
        setupUI();
    }
    lastReloadState = currentReloadState;
}

void GameOverState::render(Graphics& graphics) {
    // 黒い背景
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(0, 0, 1100, 650, true);
    
    ui.render(graphics);
    
    graphics.present();
}

void GameOverState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (stateManager) {
            float nightTimer;
            bool nightTimerActive;
            if (player->autoLoad(nightTimer, nightTimerActive)) {
                // デバッグ出力
                std::cout << "GameOverState: タイマー情報を復元 - Timer: " << nightTimer 
                          << ", Active: " << (nightTimerActive ? "true" : "false") << std::endl;
                
                player->heal(player->getMaxHp());
                player->restoreMp(player->getMaxMp());
                
                std::cout << "GameOverState: プレイヤーHP復元 - HP: " << player->getHp() 
                          << "/" << player->getMaxHp() << ", isAlive: " << (player->getIsAlive() ? "true" : "false") << std::endl;
                
                if (gameOverReason.find("戦闘に敗北") != std::string::npos) {
                    stateManager->changeState(std::make_unique<FieldState>(player));
                } else {
                    stateManager->changeState(std::make_unique<NightState>(player));
                }
                
                TownState::s_nightTimerActive = nightTimerActive;
                TownState::s_nightTimer = nightTimer;
                
                std::cout << "GameOverState: 静的変数に復元 - s_nightTimer: " << TownState::s_nightTimer 
                          << ", s_nightTimerActive: " << (TownState::s_nightTimerActive ? "true" : "false") << std::endl;
            } else {
                auto newPlayer = std::make_shared<Player>("勇者");
                newPlayer->setKingTrust(50);
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
    
    int titleX, titleY;
    config.calculatePosition(titleX, titleY, gameOverConfig.title.position, 1100, 650);
    auto titleLabelPtr = std::make_unique<Label>(titleX, titleY, "GAME OVER", "default");
    titleLabelPtr->setColor(gameOverConfig.title.color);
    titleLabel = titleLabelPtr.get();
    ui.addElement(std::move(titleLabelPtr));
    
    int reasonX, reasonY;
    config.calculatePosition(reasonX, reasonY, gameOverConfig.reason.position, 1100, 650);
    auto reasonLabelPtr = std::make_unique<Label>(reasonX, reasonY, gameOverReason, "default");
    reasonLabelPtr->setColor(gameOverConfig.reason.color);
    reasonLabel = reasonLabelPtr.get();
    ui.addElement(std::move(reasonLabelPtr));
    
    int instructionX, instructionY;
    config.calculatePosition(instructionX, instructionY, gameOverConfig.instruction.position, 1100, 650);
    auto instructionLabelPtr = std::make_unique<Label>(instructionX, instructionY, "スペースキーまたはAボタンで夜の街に再スタート", "default");
    instructionLabelPtr->setColor(gameOverConfig.instruction.color);
    instruction = instructionLabelPtr.get();
    ui.addElement(std::move(instructionLabelPtr));
} 