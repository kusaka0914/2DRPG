#include "GameOverState.h"
#include "MainMenuState.h"
#include "Graphics.h"
#include "InputManager.h"
#include <iostream>

GameOverState::GameOverState(std::shared_ptr<Player> player, const std::string& reason)
    : player(player), gameOverReason(reason), titleLabel(nullptr), reasonLabel(nullptr), instructionLabel(nullptr) {
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
    
    // スペースキーまたはAボタンでメインメニューに戻る
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (stateManager) {
            auto newPlayer = std::make_shared<Player>("勇者");
            stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
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
    auto instructionLabelPtr = std::make_unique<Label>(550, 400, "スペースキーまたはAボタンでメインメニューに戻る", "default");
    instructionLabelPtr->setColor({200, 200, 200, 255});
    instructionLabel = instructionLabelPtr.get();
    ui.addElement(std::move(instructionLabelPtr));
} 