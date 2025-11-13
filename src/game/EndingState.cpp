#include "EndingState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "MainMenuState.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>

EndingState::EndingState(std::shared_ptr<Player> player)
    : player(player), currentPhase(EndingPhase::ENDING_MESSAGE), phaseTimer(0), scrollOffset(0),
      messageLabel(nullptr), staffRollLabel(nullptr), currentMessageIndex(0), currentStaffIndex(0) {
    
    setupUI();
    setupEndingMessages();
    setupStaffRoll();
}

void EndingState::enter() {
    currentPhase = EndingPhase::ENDING_MESSAGE;
    currentMessageIndex = 0;
    currentStaffIndex = 0;
    phaseTimer = 0;
    scrollOffset = 0;
    showCurrentMessage();
}

void EndingState::exit() {
    
}

void EndingState::update(float deltaTime) {
    phaseTimer += deltaTime;
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
    
    switch (currentPhase) {
        case EndingPhase::ENDING_MESSAGE:
            if (phaseTimer >= MESSAGE_DISPLAY_TIME) {
                currentMessageIndex++;
                if (currentMessageIndex >= endingMessages.size()) {
                    nextPhase();
                } else {
                    showCurrentMessage();
                    phaseTimer = 0;
                }
            }
            break;
            
        case EndingPhase::STAFF_ROLL:
            scrollOffset += STAFF_ROLL_SPEED * deltaTime;
            if (currentStaffIndex < staffRoll.size()) {
                if (phaseTimer >= STAFF_ROLL_DELAY) {
                    currentStaffIndex++;
                    phaseTimer = 0;
                    showStaffRoll();
                }
            } else if (scrollOffset > 1800) { // より長くスクロールするように調整
                nextPhase();
            }
            break;
            
        case EndingPhase::COMPLETE:
            // 何もしない（入力待ち）
            break;
    }
}

void EndingState::render(Graphics& graphics) {
    // 背景を黒で塗りつぶし
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(0, 0, 1100, 650, true);
    
    switch (currentPhase) {
        case EndingPhase::ENDING_MESSAGE:
            // エンディングメッセージを表示（JSONから座標を取得）
            if (messageLabel) {
                auto& config = UIConfig::UIConfigManager::getInstance();
                auto endingConfig = config.getEndingConfig();
                int msgX, msgY;
                config.calculatePosition(msgX, msgY, endingConfig.message.position, graphics.getScreenWidth(), graphics.getScreenHeight());
                graphics.drawText(messageLabel->getText(), msgX, msgY, "default", endingConfig.message.color);
            }
            break;
            
        case EndingPhase::STAFF_ROLL:
            // スタッフロールを表示（JSONから座標を取得）
            {
                auto& config = UIConfig::UIConfigManager::getInstance();
                auto endingConfig = config.getEndingConfig();
                int staffX, staffY;
                config.calculatePosition(staffX, staffY, endingConfig.staffRoll.position, graphics.getScreenWidth(), graphics.getScreenHeight());
                for (int i = 0; i < currentStaffIndex && i < staffRoll.size(); i++) {
                    int y = 650 - scrollOffset + i * 50; // 行間隔を50に調整
                    if (y > -50 && y < 700) { // 表示範囲を拡大
                        graphics.drawText(staffRoll[i], staffX, y, "default", endingConfig.staffRoll.color);
                    }
                }
            }
            break;
            
        case EndingPhase::COMPLETE:
            // 完了メッセージ（JSONから座標を取得）
            {
                auto& config = UIConfig::UIConfigManager::getInstance();
                auto endingConfig = config.getEndingConfig();
                int theEndX, theEndY, returnX, returnY;
                config.calculatePosition(theEndX, theEndY, endingConfig.theEnd.position, graphics.getScreenWidth(), graphics.getScreenHeight());
                config.calculatePosition(returnX, returnY, endingConfig.returnToMenu.position, graphics.getScreenWidth(), graphics.getScreenHeight());
                graphics.drawText("THE END", theEndX, theEndY, "default", endingConfig.theEnd.color);
                graphics.drawText("スペースキーでメインメニューに戻る", returnX, returnY, "default", endingConfig.returnToMenu.color);
            }
            break;
    }
    
    graphics.present();
}

void EndingState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // スペースキーまたはAボタンでスキップ
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (currentPhase == EndingPhase::COMPLETE) {
            // メインメニューに戻る
            if (stateManager) {
                auto newPlayer = std::make_shared<Player>("勇者");
                stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
            }
        } else {
            // 次のフェーズにスキップ
            nextPhase();
        }
    }
}

void EndingState::setupUI() {
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto endingConfig = config.getEndingConfig();
    
    // メッセージ表示用のラベル（JSONから座標を取得）
    int msgX, msgY;
    config.calculatePosition(msgX, msgY, endingConfig.message.position, 1100, 650);
    auto messageLabelPtr = std::make_unique<Label>(msgX, msgY, "", "default");
    messageLabelPtr->setColor(endingConfig.message.color);
    messageLabel = messageLabelPtr.get();
    ui.addElement(std::move(messageLabelPtr));
    
    // スタッフロール用のラベル（JSONから座標を取得）
    int staffX, staffY;
    config.calculatePosition(staffX, staffY, endingConfig.staffRoll.position, 1100, 650);
    auto staffRollLabelPtr = std::make_unique<Label>(staffX, staffY, "", "default");
    staffRollLabelPtr->setColor(endingConfig.staffRoll.color);
    staffRollLabel = staffRollLabelPtr.get();
    ui.addElement(std::move(staffRollLabelPtr));
}

void EndingState::setupEndingMessages() {
    endingMessages = {
        "魔王を倒した勇者" + player->getName() + "は...",
        "世界の支配者となった。",
        "かつての街は廃墟と化し、",
        "人々は新たな支配者の下で暮らすこととなった。",
        "しかし、これは本当に正しい選択だったのだろうか...",
        "勇者は世界を救ったのか、それとも...",
        "新たな魔王となったのか。"
    };
}

void EndingState::setupStaffRoll() {
    staffRoll = {
        "STAFF ROLL",
        "",
        "GAME DESIGN",
        "TAKUMI KUSAKA",
        "",
        "PROGRAMMING",
        "TAKUMI KUSAKA",
        "",
        "GRAPHICS",
        "TAKUMI KUSAKA",
        "",
        "MUSIC & SOUND",
        "TAKUMI KUSAKA",
        "",
        "SPECIAL THANKS",
        "PLAYERS",
        "",
        "THANK YOU FOR PLAYING",
        "",
        "勇者だって強者に逆らえない。",
        "THE END"
    };
}

void EndingState::showCurrentMessage() {
    if (currentMessageIndex < endingMessages.size()) {
        messageLabel->setText(endingMessages[currentMessageIndex]);
    }
}

void EndingState::showStaffRoll() {
    if (staffRollLabel) {
        staffRollLabel->setText("STAFF ROLL");
    }
}

void EndingState::nextPhase() {
    switch (currentPhase) {
        case EndingPhase::ENDING_MESSAGE:
            currentPhase = EndingPhase::STAFF_ROLL;
            phaseTimer = 0;
            scrollOffset = 0; // 画面の下から開始
            currentStaffIndex = 0;
            break;
            
        case EndingPhase::STAFF_ROLL:
            currentPhase = EndingPhase::COMPLETE;
            break;
            
        case EndingPhase::COMPLETE:
            // 既に完了状態
            break;
    }
}
