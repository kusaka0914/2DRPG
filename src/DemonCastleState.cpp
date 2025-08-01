#include "DemonCastleState.h"
#include "TownState.h"
#include "Graphics.h"
#include "InputManager.h"
#include <iostream>

DemonCastleState::DemonCastleState(std::shared_ptr<Player> player)
    : player(player), playerX(12), playerY(15), moveTimer(0),
      messageBoard(nullptr), isShowingMessage(false),
      isTalkingToDemon(false), dialogueStep(0), hasReceivedEvilQuest(false) {
    
    // 魔王の会話を初期化
    demonDialogues = {
        "お前が勇者か...",
        "実はお前は私の手下だ。",
        "街を滅ぼしてくれ。",
        "王様に悟られないように行動しろ。"
    };
    
    setupDemonCastle();
    setupUI();
}

void DemonCastleState::enter() {
    std::cout << "魔王の城に入りました" << std::endl;
    showMessage("魔王の城に到着しました。魔王との会話が始まります。");
}

void DemonCastleState::exit() {
    std::cout << "魔王の城を出ました" << std::endl;
}

void DemonCastleState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
}

void DemonCastleState::render(Graphics& graphics) {
    // 背景色を設定（暗い魔王の城）
    graphics.setDrawColor(20, 0, 20, 255);
    graphics.clear();
    
    drawDemonCastle(graphics);
    drawDemonCastleObjects(graphics);
    drawPlayer(graphics);
    
    // メッセージがある時のみメッセージボードの黒背景を描画
    if (messageBoard && !messageBoard->getText().empty()) {
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(40, 440, 720, 100, true); // メッセージボード背景
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(40, 440, 720, 100); // メッセージボード枠
    }
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void DemonCastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    // 会話中の処理
    if (isTalkingToDemon) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            nextDialogue();
        }
        return;
    }
    
    // プレイヤー移動
    if (moveTimer <= 0) {
        handleMovement(input);
    }
    
    // スペースキーで相互作用またはドアから出る
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (isNearDoor()) {
            std::cout << "魔王の城を出て街に向かいます..." << std::endl;
            exitToTown();
        } else {
            checkInteraction();
        }
    }
}

void DemonCastleState::setupUI() {
    ui.clear();
    
    // プレイヤー情報表示
    auto playerInfoLabel = std::make_unique<Label>(10, 10, "", "default");
    playerInfoLabel->setColor({255, 255, 255, 255});
    playerInfoLabel->setText(player->getName() + " Lv:" + std::to_string(player->getLevel()) + 
                           " HP:" + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp()) +
                           " ゴールド:" + std::to_string(player->getGold()));
    ui.addElement(std::move(playerInfoLabel));
    
    // 操作説明
    auto controlsLabel = std::make_unique<Label>(10, 30, "", "default");
    controlsLabel->setColor({255, 255, 255, 255});
    controlsLabel->setText("移動: 矢印キー/WASD/ゲームパッド | 調べる: スペース/Aボタン | ドアから出る");
    ui.addElement(std::move(controlsLabel));
    
    // メッセージボード（黒背景）
    auto messageBoardLabel = std::make_unique<Label>(50, 450, "", "default");
    messageBoardLabel->setColor({255, 255, 255, 255}); // 白文字
    messageBoardLabel->setText("魔王の城を探索してみましょう");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void DemonCastleState::setupDemonCastle() {
    // 魔王の城のオブジェクト配置
    demonX = 12; demonY = 5;    // 魔王（中央上部）
    doorX = 12; doorY = 18;     // ドア（下部中央）
}

void DemonCastleState::handleMovement(const InputManager& input) {
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

void DemonCastleState::checkInteraction() {
    // 魔王との相互作用
    if (isNearObject(demonX, demonY)) {
        interactWithDemon();
    }
}

void DemonCastleState::interactWithDemon() {
    if (!hasReceivedEvilQuest) {
        startDialogue();
    } else {
        showMessage("魔王: 街を滅ぼすのを忘れるな！");
    }
}

void DemonCastleState::exitToTown() {
    if (stateManager) {
        stateManager->changeState(std::make_unique<TownState>(player));
    }
}

void DemonCastleState::startDialogue() {
    isTalkingToDemon = true;
    dialogueStep = 0;
    showCurrentDialogue();
}

void DemonCastleState::nextDialogue() {
    dialogueStep++;
    if (dialogueStep >= demonDialogues.size()) {
        // 会話終了
        isTalkingToDemon = false;
        hasReceivedEvilQuest = true;
        player->performEvilAction(); // 悪行として記録
        std::cout << "魔王との会話が終了しました。街に向かいます。" << std::endl;
        showMessage("魔王との会話が終了しました。街に向かいます。");
    } else {
        showCurrentDialogue();
    }
}

void DemonCastleState::showCurrentDialogue() {
    if (dialogueStep < demonDialogues.size()) {
        std::cout << "魔王: " << demonDialogues[dialogueStep] << std::endl;
        showMessage("魔王: " + demonDialogues[dialogueStep]);
    }
}

void DemonCastleState::showMessage(const std::string& message) {
    if (messageBoard) {
        messageBoard->setText(message);
        isShowingMessage = true;
        std::cout << "メッセージを表示: " << message << std::endl;
    }
}

void DemonCastleState::clearMessage() {
    if (messageBoard) {
        messageBoard->setText("");
        isShowingMessage = false;
        std::cout << "メッセージをクリアしました" << std::endl;
    }
}

void DemonCastleState::drawDemonCastle(Graphics& graphics) {
    // 床を描画（暗い魔王の城）
    graphics.setDrawColor(40, 0, 40, 255); // 暗い紫
    for (int y = 1; y < ROOM_HEIGHT - 1; y++) {
        for (int x = 1; x < ROOM_WIDTH - 1; x++) {
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // 壁を描画（暗い石造り）
    graphics.setDrawColor(20, 0, 20, 255); // 暗い紫
    // 上下の壁
    for (int x = 0; x < ROOM_WIDTH; x++) {
        graphics.drawRect(x * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(x * TILE_SIZE, (ROOM_HEIGHT - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
    // 左右の壁
    for (int y = 0; y < ROOM_HEIGHT; y++) {
        graphics.drawRect(0, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect((ROOM_WIDTH - 1) * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

void DemonCastleState::drawDemonCastleObjects(Graphics& graphics) {
    // 魔王を描画（赤色）
    graphics.setDrawColor(255, 0, 0, 255);
    graphics.drawRect(demonX * TILE_SIZE, demonY * TILE_SIZE, TILE_SIZE * 3, TILE_SIZE * 2, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(demonX * TILE_SIZE, demonY * TILE_SIZE, TILE_SIZE * 3, TILE_SIZE * 2);
    
    // 魔王の装飾
    graphics.setDrawColor(139, 0, 0, 255);
    graphics.drawRect((demonX + 1) * TILE_SIZE, (demonY + 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    
    // ドアを描画（暗い茶色）
    graphics.setDrawColor(80, 40, 20, 255);
    graphics.drawRect(doorX * TILE_SIZE, doorY * TILE_SIZE, TILE_SIZE * 2, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(doorX * TILE_SIZE, doorY * TILE_SIZE, TILE_SIZE * 2, TILE_SIZE);
}

void DemonCastleState::drawPlayer(Graphics& graphics) {
    int drawX = playerX * TILE_SIZE + 4;
    int drawY = playerY * TILE_SIZE + 4;
    int size = TILE_SIZE - 8;
    
    // プレイヤーを描画（青色）
    graphics.setDrawColor(0, 0, 255, 255);
    graphics.drawRect(drawX, drawY, size, size, true);
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(drawX, drawY, size, size, false);
}

bool DemonCastleState::isValidPosition(int x, int y) const {
    return x >= 1 && x < ROOM_WIDTH - 1 && y >= 1 && y < ROOM_HEIGHT - 1;
}

bool DemonCastleState::isNearObject(int x, int y) const {
    int dx = abs(playerX - x);
    int dy = abs(playerY - y);
    return (dx <= 1 && dy <= 1);
}

bool DemonCastleState::isNearDoor() const {
    int dx = abs(playerX - doorX);
    int dy = abs(playerY - doorY);
    return (dx <= 1 && dy <= 1);
} 