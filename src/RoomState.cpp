#include "RoomState.h"
#include "Graphics.h"
#include "InputManager.h"
#include "TownState.h"
#include "CastleState.h"
#include "NightState.h"
#include "CommonUI.h"
#include <iostream>

// 静的変数（ファイル内で共有）
static bool s_roomFirstTime = true;

RoomState::RoomState(std::shared_ptr<Player> player)
    : player(player), playerX(3), playerY(2), 
      messageBoard(nullptr), howtooperateBoard(nullptr), isShowingMessage(false),
      hasOpenedChest(false), bedTexture(nullptr), houseTileTexture(nullptr),
      moveTimer(0), nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer) {
    if (s_roomFirstTime) {
        playerX = 3;
        playerY = 2;
    } else {
        playerX = 3;
        playerY = 3;
    }
    setupRoom();
    setupUI();
}

void RoomState::enter() {
    setupUI();
    
    // テクスチャを読み込み
    // 注意：Graphicsオブジェクトが必要なので、render時に読み込む
    
    if (s_roomFirstTime) {
        showWelcomeMessage();
    } else {
        showMessage("自室に戻りました。");
    }
}

void RoomState::exit() {
    ui.clear();
}

void RoomState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // 夜のタイマーを更新
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        // 静的変数に状態を保存
        TownState::s_nightTimerActive = true;
        TownState::s_nightTimer = nightTimer;
        
        if (nightTimer <= 0.0f) {
            // 5分経過したら夜の街に移動
            nightTimerActive = false;
            TownState::s_nightTimerActive = false;
            TownState::s_nightTimer = 0.0f;
            if (stateManager) {
                stateManager->changeState(std::make_unique<NightState>(player));
                player->setCurrentNight(player->getCurrentNight() + 1);
            }
        }
    } else {
        // タイマーが非アクティブな場合、静的変数もクリア
        TownState::s_nightTimerActive = false;
        TownState::s_nightTimer = 0.0f;
    }
    
    // 移動処理
}

void RoomState::render(Graphics& graphics) {
    // 初回のみテクスチャを読み込み
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    // 背景色を設定（温かい部屋の色）
    graphics.setDrawColor(139, 69, 19, 255); // 茶色
    graphics.clear();
    
    drawRoom(graphics);
    drawRoomObjects(graphics);
    drawPlayer(graphics);
    
    // メッセージがある時のみメッセージボードの黒背景を描画
    if (messageBoard && !messageBoard->getText().empty()) {
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(190, 480, 720, 100, true); // メッセージボード背景（画面中央）
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(190, 480, 720, 100); // メッセージボード枠（画面中央）
    }

    if (howtooperateBoard && !howtooperateBoard->getText().empty()) {
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(190, 10, 395, 70, true); // メッセージボード背景（画面中央）
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(190, 10, 395, 70); // メッセージボード枠（画面中央）
    }
    
    // UI描画
    ui.render(graphics);
    
    // 夜のタイマーを表示
    if (nightTimerActive) {
        CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    }
        
    graphics.present();
}

void RoomState::handleInput(const InputManager& input) {
    GameState::handleInputTemplate(input, ui, isShowingMessage, moveTimer,
        [this, &input]() { handleMovement(input); },
        [this]() { checkInteraction(); },
        [this]() { 
            
            if (s_roomFirstTime) {
                stateManager->changeState(std::make_unique<CastleState>(player));
                s_roomFirstTime = false;
            }
            else {
                stateManager->changeState(std::make_unique<TownState>(player));
            }
        },
        [this]() { return isNearObject(doorX, doorY); },
        [this]() { clearMessage(); });
}

void RoomState::setupUI() {
    ui.clear();
    
    // メッセージボード（画面中央下部）
    auto messageBoardLabel = std::make_unique<Label>(210, 500, "", "default"); // メッセージボード背景の左上付近
    messageBoardLabel->setColor({255, 255, 255, 255}); // 白文字
    messageBoardLabel->setText("");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));

    auto howtooperateLabel = std::make_unique<Label>(205, 25, "", "default"); // メッセージボード背景の左上付近
    howtooperateLabel->setColor({255, 255, 255, 255}); // 白文字
    howtooperateLabel->setText("移動: 矢印キー\n調べる/ドアに入る/次のメッセージ: スペースキー");
    howtooperateBoard = howtooperateLabel.get(); // ポインタを保存
    ui.addElement(std::move(howtooperateLabel));
}

void RoomState::setupRoom() {
    // 部屋のオブジェクト配置（7x5の部屋、画面中央に配置）
    bedX = 1; bedY = 1;      // ベッド（左上）
    deskX = 5; deskY = 1;    // 机（右上）
    chestX = 5; chestY = 2;  // 宝箱（机の下）
    doorX = 3; doorY = 4;    // ドア（下部中央）
}

void RoomState::loadTextures(Graphics& graphics) {
    // 画像を読み込み
    playerTexture = graphics.loadTexture("assets/characters/player.png", "player");
    deskTexture = graphics.loadTexture("assets/objects/desk.png", "desk");
    chestClosedTexture = graphics.loadTexture("assets/objects/closed_box.png", "chest_closed");
    chestOpenTexture = graphics.loadTexture("assets/objects/open_box.png", "chest_open");
    bedTexture = graphics.loadTexture("assets/objects/bed.png", "bed");
    houseTileTexture = graphics.loadTexture("assets/tiles/housetile.png", "house_tile");
}

void RoomState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
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
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    // 背景を黒で塗りつぶし
    graphics.setDrawColor(0, 0, 0, 255); // 黒色
    graphics.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
    
    // 自室の建物タイル画像を描画（床のみ）
    SDL_Texture* houseTileTexture = graphics.getTexture("house_tile");
    if (houseTileTexture) {
        // 建物タイル画像を部屋の床に敷き詰める（壁は除く）
        for (int y = 1; y < ROOM_HEIGHT - 1; y++) {
            for (int x = 1; x < ROOM_WIDTH - 1; x++) {
                graphics.drawTexture(houseTileTexture, ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        // 画像がない場合は色で描画（フォールバック）
        graphics.setDrawColor(160, 82, 45, 255); // ライトブラウン
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT, true);
    }
    
    // 部屋の壁を描画（色で）
    graphics.setDrawColor(101, 67, 33, 255); // ダークブラウン
    // 上下の壁
    for (int x = 0; x < ROOM_WIDTH; x++) {
        graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + (ROOM_HEIGHT - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
    // 左右の壁
    for (int y = 0; y < ROOM_HEIGHT; y++) {
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(ROOM_OFFSET_X + (ROOM_WIDTH - 1) * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

void RoomState::drawRoomObjects(Graphics& graphics) {
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    // ベッドを描画
    SDL_Texture* bedTexture = graphics.getTexture("bed");
    if (bedTexture) {
        graphics.drawTexture(bedTexture, ROOM_OFFSET_X + bedX * TILE_SIZE, ROOM_OFFSET_Y + bedY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(100, 149, 237, 255); // コーンフラワーブルー
        graphics.drawRect(ROOM_OFFSET_X + bedX * TILE_SIZE, ROOM_OFFSET_Y + bedY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + bedX * TILE_SIZE, ROOM_OFFSET_Y + bedY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
    
    // 机を描画
    SDL_Texture* deskTexture = graphics.getTexture("desk");
    if (deskTexture) {
        graphics.drawTexture(deskTexture, ROOM_OFFSET_X + deskX * TILE_SIZE, ROOM_OFFSET_Y + deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(139, 69, 19, 255);
        graphics.drawRect(ROOM_OFFSET_X + deskX * TILE_SIZE, ROOM_OFFSET_Y + deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + deskX * TILE_SIZE, ROOM_OFFSET_Y + deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
    
    // 宝箱を描画
    if (hasOpenedChest) {
        SDL_Texture* openBoxTexture = graphics.getTexture("open_box");
        if (openBoxTexture) {
            graphics.drawTexture(openBoxTexture, ROOM_OFFSET_X + chestX * TILE_SIZE, ROOM_OFFSET_Y + chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        } else {
            graphics.setDrawColor(160, 160, 160, 255); // グレー（開いた状態）
            graphics.drawRect(ROOM_OFFSET_X + chestX * TILE_SIZE, ROOM_OFFSET_Y + chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(ROOM_OFFSET_X + chestX * TILE_SIZE, ROOM_OFFSET_Y + chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    } else {
        SDL_Texture* closedBoxTexture = graphics.getTexture("closed_box");
        if (closedBoxTexture) {
            graphics.drawTexture(closedBoxTexture, ROOM_OFFSET_X + chestX * TILE_SIZE, ROOM_OFFSET_Y + chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        } else {
            graphics.setDrawColor(255, 215, 0, 255); // ゴールド色
            graphics.drawRect(ROOM_OFFSET_X + chestX * TILE_SIZE, ROOM_OFFSET_Y + chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(ROOM_OFFSET_X + chestX * TILE_SIZE, ROOM_OFFSET_Y + chestY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }
    
    // ドアを描画（茶色、少し明るめ）
    graphics.setDrawColor(160, 82, 45, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void RoomState::drawPlayer(Graphics& graphics) {
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    // プレイヤーを描画（画像または緑色の四角）
    if (playerTexture) {
        graphics.drawTexture(playerTexture, ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        // フォールバック：緑色の四角で描画
        graphics.setDrawColor(0, 255, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
}

bool RoomState::isValidPosition(int x, int y) const {
    // 7x5の部屋の境界チェック
    if (!GameState::isValidPosition(x, y, 1, 1, ROOM_WIDTH - 1, ROOM_HEIGHT - 1)) {
        return false;
    }
    
    // オブジェクトとの衝突チェック
    if ((x >= bedX && x < bedX + 1 && y == bedY) ||  // ベッド（1マス分）
        (x == deskX && y == deskY) ||                // 机
        (x == chestX && y == chestY)) {              // 宝箱
        return false;
    }
    
    return true;
}

bool RoomState::isNearObject(int objX, int objY) const {
    return GameState::isNearObject(playerX, playerY, objX, objY);
}

void RoomState::showWelcomeMessage() {
    showMessage("王様からの手紙\n勇者よ、お主に頼みがある。一度我が城に来てくれないか。\nそこで詳しい話をしよう。では、待っておるぞ。");
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
    if (s_roomFirstTime) {
        showMessage("城に向かいます...");
        stateManager->changeState(std::make_unique<CastleState>(player, false));
        s_roomFirstTime = false;
    } else {
        showMessage("町に向かいます...");
        stateManager->changeState(std::make_unique<TownState>(player));
    }
}

StateType RoomState::getType() const {
    return StateType::ROOM;
} 

void RoomState::showMessage(const std::string& message) {
    GameState::showMessage(message, messageBoard, isShowingMessage);
}

void RoomState::clearMessage() {
    GameState::clearMessage(messageBoard, isShowingMessage);
} 