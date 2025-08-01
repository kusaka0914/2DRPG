#include "NightState.h"
#include "TownState.h"
#include "Graphics.h"
#include "InputManager.h"
#include <iostream>
#include <random>

NightState::NightState(std::shared_ptr<Player> player)
    : player(player), playerX(12), playerY(15), moveTimer(0),
      isStealthMode(true), stealthLevel(50), messageLabel(nullptr), isShowingMessage(false) {
    
    // 住民の位置を初期化
    residents = {
        {8, 8}, {15, 8}, {22, 8},  // 上段
        {5, 12}, {18, 12}, {25, 12}, // 中段
        {10, 16}, {20, 16} // 下段
    };
    
    // 見張りの位置を初期化
    guards = {
        {12, 5}, {20, 5}, // 上段の見張り
        {5, 18}, {25, 18} // 下段の見張り
    };
}

void NightState::enter() {
    std::cout << "夜間モードに入りました" << std::endl;
    player->setNightTime(true);
    showMessage("夜の街に潜入しました。住民を襲撃して街を壊滅させましょう。");
    setupUI();
}

void NightState::exit() {
    std::cout << "夜間モードを終了しました" << std::endl;
    player->setNightTime(false);
}

void NightState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
}

void NightState::render(Graphics& graphics) {
    // 背景色を設定（夜の街）
    graphics.setDrawColor(20, 20, 40, 255);
    graphics.clear();
    
    drawNightTown(graphics);
    drawPlayer(graphics);
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void NightState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中は移動不可
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
        }
        return;
    }
    
    // ESCキーで街に戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<TownState>(player));
        }
        return;
    }
    
    // スペースキーで住民を襲撃
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        checkResidentInteraction();
        return;
    }
    
    // プレイヤー移動
    handleMovement(input);
}

void NightState::setupUI() {
    ui.clear();
    
    // プレイヤー情報表示
    auto playerInfoLabel = std::make_unique<Label>(10, 10, "", "default");
    playerInfoLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(playerInfoLabel));
    
    // 信頼度表示
    auto trustLabel = std::make_unique<Label>(10, 35, "", "default");
    trustLabel->setColor({255, 200, 200, 255});
    ui.addElement(std::move(trustLabel));
    
    // 隠密レベル表示
    auto stealthLabel = std::make_unique<Label>(10, 60, "", "default");
    stealthLabel->setColor({200, 255, 200, 255});
    ui.addElement(std::move(stealthLabel));
    
    // メッセージ表示用
    auto messageLabelPtr = std::make_unique<Label>(50, 450, "", "default");
    messageLabelPtr->setColor({255, 255, 255, 255});
    messageLabel = messageLabelPtr.get();
    ui.addElement(std::move(messageLabelPtr));
    
    // 操作説明
    auto controlsLabel = std::make_unique<Label>(10, 550, "移動: 方向キー/WASD/ゲームパッド | 襲撃: スペース/Aボタン | 戻る: ESC/Bボタン", "default");
    controlsLabel->setColor({200, 200, 200, 255});
    ui.addElement(std::move(controlsLabel));
}

void NightState::handleMovement(const InputManager& input) {
    if (moveTimer > 0) return;
    
    int newX = playerX;
    int newY = playerY;
    
    // キーボード入力
    if (input.isKeyPressed(InputKey::UP) || input.isKeyPressed(InputKey::W)) {
        newY--;
    } else if (input.isKeyPressed(InputKey::DOWN) || input.isKeyPressed(InputKey::S)) {
        newY++;
    } else if (input.isKeyPressed(InputKey::LEFT) || input.isKeyPressed(InputKey::A)) {
        newX--;
    } else if (input.isKeyPressed(InputKey::RIGHT) || input.isKeyPressed(InputKey::D)) {
        newX++;
    }
    
    // ゲームパッドのアナログスティック入力
    const float DEADZONE = 0.3f;
    float stickX = input.getLeftStickX();
    float stickY = input.getLeftStickY();
    
    if (abs(stickX) > DEADZONE || abs(stickY) > DEADZONE) {
        if (abs(stickX) > abs(stickY)) {
            if (stickX < -DEADZONE) {
                newX--;
            } else if (stickX > DEADZONE) {
                newX++;
            }
        } else {
            if (stickY < -DEADZONE) {
                newY--;
            } else if (stickY > DEADZONE) {
                newY++;
            }
        }
    }
    
    if (isValidPosition(newX, newY)) {
        playerX = newX;
        playerY = newY;
        moveTimer = MOVE_DELAY;
    }
}

void NightState::checkResidentInteraction() {
    // 近くの住民をチェック
    for (auto& resident : residents) {
        int distance = abs(playerX - resident.first) + abs(playerY - resident.second);
        if (distance <= 1) {
            attackResident(resident.first, resident.second);
            return;
        }
    }
    
    showMessage("近くに住民がいません。");
}

void NightState::attackResident(int x, int y) {
    // 見張りに発見される可能性をチェック
    for (auto& guard : guards) {
        int distance = abs(x - guard.first) + abs(y - guard.second);
        if (distance <= 3) {
            // 見張りに発見される可能性
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 100);
            
            if (dis(gen) <= 30) { // 30%の確率で発見
                showMessage("見張りに発見されました！信頼度が大幅に下がります。");
                player->changeTrustLevel(-20);
                return;
            }
        }
    }
    
    // 住民を襲撃
    player->performEvilAction();
    showMessage("住民を襲撃しました。街の人口が減りました。");
    
    // 住民をリストから削除
    residents.erase(std::remove_if(residents.begin(), residents.end(),
        [x, y](const std::pair<int, int>& pos) {
            return pos.first == x && pos.second == y;
        }), residents.end());
}

void NightState::hideEvidence() {
    player->performEvilAction();
    showMessage("証拠を隠滅しました。");
}

void NightState::showMessage(const std::string& message) {
    if (messageLabel) {
        messageLabel->setText(message);
        isShowingMessage = true;
    }
}

void NightState::clearMessage() {
    if (messageLabel) {
        messageLabel->setText("");
        isShowingMessage = false;
    }
}

void NightState::drawNightTown(Graphics& graphics) {
    // 夜の街を描画（暗い背景）
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 30; x++) {
            int drawX = x * TILE_SIZE;
            int drawY = y * TILE_SIZE;
            
            // 暗い街の背景
            graphics.setDrawColor(30, 30, 50, 255);
            graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // 住民を描画（赤い点）
    graphics.setDrawColor(255, 100, 100, 255);
    for (auto& resident : residents) {
        int drawX = resident.first * TILE_SIZE + 8;
        int drawY = resident.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
    
    // 見張りを描画（黄色い点）
    graphics.setDrawColor(255, 255, 100, 255);
    for (auto& guard : guards) {
        int drawX = guard.first * TILE_SIZE + 8;
        int drawY = guard.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
}

void NightState::drawPlayer(Graphics& graphics) {
    int drawX = playerX * TILE_SIZE + 4;
    int drawY = playerY * TILE_SIZE + 4;
    int size = TILE_SIZE - 8;
    
    // プレイヤーを暗い色で描画（隠密モード）
    graphics.setDrawColor(100, 100, 150, 255);
    graphics.drawRect(drawX, drawY, size, size, true);
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(drawX, drawY, size, size, false);
}

bool NightState::isValidPosition(int x, int y) const {
    return x >= 0 && x < 30 && y >= 0 && y < 20;
} 