#include "GameOverState.h"
#include "MainMenuState.h"
#include "NightState.h"
#include "Graphics.h"
#include "InputManager.h"
#include <iostream>

GameOverState::GameOverState(std::shared_ptr<Player> player, const std::string& reason)
    : player(player), gameOverReason(reason), titleLabel(nullptr), reasonLabel(nullptr), instruction(nullptr) {
}

void GameOverState::enter() {
    std::cout << "ゲームオーバー: " << gameOverReason << std::endl;
    setupUI();
}

void GameOverState::exit() {
    std::cout << "ゲームオーバー画面を終了しました" << std::endl;
}

void GameOverState::update(float deltaTime) {
    ui.update(deltaTime);
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
    
    // スペースキーまたはAボタンでNightStateに再スタート
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (stateManager) {
            // オートセーブデータをロード
            if (player->autoLoad()) {
                // ロード成功した場合はNightStateに戻る
                stateManager->changeState(std::make_unique<NightState>(player));
            } else {
                // ロード失敗した場合はメインメニューに戻る
                auto newPlayer = std::make_shared<Player>("勇者");
                stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
            }
        }
    }
}

void GameOverState::setupUI() {
    ui.clear();
    
    // タイトル
    auto titleLabelPtr = std::make_unique<Label>(550, 200, "GAME OVER", "default");
    titleLabelPtr->setColor({255, 0, 0, 255});
    titleLabel = titleLabelPtr.get();
    ui.addElement(std::move(titleLabelPtr));
    
    // ゲームオーバー理由
    auto reasonLabelPtr = std::make_unique<Label>(550, 250, gameOverReason, "default");
    reasonLabelPtr->setColor({255, 255, 255, 255});
    reasonLabel = reasonLabelPtr.get();
    ui.addElement(std::move(reasonLabelPtr));
    
    // 操作説明
    auto instructionLabelPtr = std::make_unique<Label>(550, 400, "スペースキーまたはAボタンで夜の街に再スタート", "default");
    instructionLabelPtr->setColor({200, 200, 200, 255});
    instruction = instructionLabelPtr.get();
    ui.addElement(std::move(instructionLabelPtr));
} 