#include "RoomState.h"
#include "Graphics.h"
#include "InputManager.h"
#include "TownState.h"
#include "CastleState.h"
#include <iostream>

RoomState::RoomState(std::shared_ptr<Player> player)
    : player(player), playerX(7), playerY(9), moveTimer(0), 
      messageBoard(nullptr), isShowingMessage(false),
      bedX(5), bedY(5), deskX(12), deskY(5), chestX(5), chestY(12), doorX(10), doorY(15) {
    setupRoom();
}

void RoomState::enter() {
    setupUI();
    if (isFirstTime) {
        showWelcomeMessage();
        isFirstTime = false;
    } else {
        showMessage("自分の部屋にいます。");
    }
}

void RoomState::exit() {
    ui.clear();
}

void RoomState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
}

void RoomState::render(Graphics& graphics) {
    // 背景色を設定（温かい部屋の色）
    graphics.setDrawColor(139, 69, 19, 255); // 茶色
    graphics.clear();
    
    drawRoom(graphics);
    drawRoomObjects(graphics);
    drawPlayer(graphics);
    
    // メッセージがある時のみメッセージボードの黒背景を描画
    if (messageBoard && !messageBoard->getText().empty()) {
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(40, 440, 720, 100, true); // メッセージボード背景 (filled)
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(40, 440, 720, 100); // メッセージボード枠
    }
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void RoomState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            std::cout << "RoomState: メッセージクリア処理実行" << std::endl;
            clearMessage();
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (moveTimer <= 0) {
        handleMovement(input);
    }
    
    // スペースキーで相互作用またはドアから出る
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        std::cout << "プレイヤー位置: (" << playerX << ", " << playerY << "), ドア位置: (" << doorX << ", " << doorY << ")" << std::endl;
        if (isNearDoor()) {
            std::cout << "自室を出て城に向かいます..." << std::endl;
            if (stateManager) {
                stateManager->changeState(std::make_unique<CastleState>(player));
            }
        } else {
            std::cout << "ドアの近くにいません" << std::endl;
            checkInteraction();
        }
    }
}

void RoomState::setupUI() {
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
    messageBoardLabel->setText("部屋を探索してみましょう");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void RoomState::setupRoom() {
    // 部屋のオブジェクト配置
    bedX = 2; bedY = 2;          // ベッド（左上）
    deskX = 12; deskY = 2;       // 机（右上）
    chestX = 12; chestY = 9;     // 宝箱（右下）
    doorX = 7; doorY = 11;       // ドア（下中央）
}

void RoomState::handleMovement(const InputManager& input) {
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

void RoomState::checkInteraction() {
    // ベッドとの相互作用
    if (isNearObject(bedX, bedY)) {
        interactWithBed();
    }
    // 机との相互作用
    else if (isNearObject(deskX, deskY)) {
        interactWithDesk();
    }
    // 宝箱との相互作用
    else if (isNearObject(chestX, chestY)) {
        interactWithChest();
    }
    // ドアとの相互作用
    else if (isNearObject(doorX, doorY)) {
        exitToTown();
    }
}

void RoomState::drawRoom(Graphics& graphics) {
    // 床を描画
    graphics.setDrawColor(160, 82, 45, 255); // ライトブラウン
    for (int y = 1; y < ROOM_HEIGHT - 1; y++) {
        for (int x = 1; x < ROOM_WIDTH - 1; x++) {
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // 壁を描画
    graphics.setDrawColor(101, 67, 33, 255); // ダークブラウン
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

void RoomState::drawRoomObjects(Graphics& graphics) {
    // ベッドを描画（青色）
    graphics.setDrawColor(100, 149, 237, 255);
    graphics.drawRect(bedX * TILE_SIZE, bedY * TILE_SIZE, TILE_SIZE * 2, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(bedX * TILE_SIZE, bedY * TILE_SIZE, TILE_SIZE * 2, TILE_SIZE);
    
    // 机を描画（茶色）
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(deskX * TILE_SIZE, deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(deskX * TILE_SIZE, deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    
    // 宝箱を描画（ゴールド色または開いた色）
    if (hasOpenedChest) {
        graphics.setDrawColor(160, 160, 160, 255); // グレー（開いた状態）
    } else {
        graphics.setDrawColor(255, 215, 0, 255); // ゴールド色
    }
    graphics.drawRect(chestX * TILE_SIZE, chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(chestX * TILE_SIZE, chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    
    // ドアを描画（茶色、少し明るめ）
    graphics.setDrawColor(160, 82, 45, 255);
    graphics.drawRect(doorX * TILE_SIZE, doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(doorX * TILE_SIZE, doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void RoomState::drawPlayer(Graphics& graphics) {
    // プレイヤーを描画（緑色）
    graphics.setDrawColor(0, 255, 0, 255);
    graphics.drawRect(playerX * TILE_SIZE + 4, playerY * TILE_SIZE + 4, TILE_SIZE - 8, TILE_SIZE - 8, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(playerX * TILE_SIZE + 4, playerY * TILE_SIZE + 4, TILE_SIZE - 8, TILE_SIZE - 8);
}

bool RoomState::isValidPosition(int x, int y) const {
    // 部屋の境界チェック
    if (x < 1 || x >= ROOM_WIDTH - 1 || y < 1 || y >= ROOM_HEIGHT - 1) {
        return false;
    }
    
    // オブジェクトとの衝突チェック
    if ((x >= bedX && x < bedX + 2 && y == bedY) ||  // ベッド（2マス分）
        (x == deskX && y == deskY) ||                // 机
        (x == chestX && y == chestY)) {              // 宝箱
        return false;
    }
    
    return true;
}

bool RoomState::isNearObject(int objX, int objY) const {
    int dx = abs(playerX - objX);
    int dy = abs(playerY - objY);
    return (dx <= 1 && dy <= 1);
}

bool RoomState::isNearDoor() const {
    int dx = abs(playerX - doorX);
    int dy = abs(playerY - doorY);
    return (dx <= 1 && dy <= 1); // 距離1以内ならドアの近く
}

void RoomState::showWelcomeMessage() {
    showMessage("=== 冒険の始まり ===\n目が覚めると、あなたは自分の部屋にいました。\n今日から冒険者として旅立つ日です！");
}

void RoomState::interactWithBed() {
    showMessage("ふかふかのベッドです。\nここでぐっすり眠っていました。\nHPが全回復しました！");
    player->heal(player->getMaxHp());
}

void RoomState::interactWithDesk() {
    showMessage("勉強机です。\n冒険者の心得: 「危険な時は町に逃げ込むこと。\nアイテムを有効活用すること。」");
}

void RoomState::interactWithChest() {
    if (!hasOpenedChest) {
        showMessage("宝箱を開けました！\n冒険用のお金が入っていました。\n100ゴールドを手に入れた！");
        player->gainGold(100);
        hasOpenedChest = true;
    } else {
        showMessage("空の宝箱です。すでに中身は取り出しました。");
    }
}

void RoomState::exitToTown() {
    showMessage("町に向かいます...");
    stateManager->changeState(std::make_unique<TownState>(player));
}

StateType RoomState::getType() const {
    return StateType::ROOM;
} 

void RoomState::showMessage(const std::string& message) {
    if (messageBoard) {
        messageBoard->setText(message);
        isShowingMessage = true;
        std::cout << "RoomState: メッセージを表示: " << message << std::endl;
    }
}

void RoomState::clearMessage() {
    if (messageBoard) {
        messageBoard->setText("");
        isShowingMessage = false;
        std::cout << "RoomState: メッセージをクリアしました" << std::endl;
    }
} 