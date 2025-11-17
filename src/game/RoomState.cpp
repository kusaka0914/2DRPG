#include "RoomState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "TownState.h"
#include "CastleState.h"
#include "NightState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>
#include <nlohmann/json.hpp>

static bool s_roomFirstTime = true;

RoomState::RoomState(std::shared_ptr<Player> player)
    : player(player), playerX(3), playerY(2), 
      messageBoard(nullptr), howtooperateBoard(nullptr), isShowingMessage(false),
      hasOpenedChest(false), bedTexture(nullptr), houseTileTexture(nullptr),
      moveTimer(0), nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      pendingWelcomeMessage(false), pendingMessage("") {
    isFadingOut = false;
    isFadingIn = false;
    fadeTimer = 0.0f;
    fadeDuration = 0.5f;
    if (s_roomFirstTime) {
        playerX = 3;
        playerY = 2;
    } else {
        playerX = 3;
        playerY = 3;
    }
    setupRoom();
}

void RoomState::enter() {
    startFadeIn(0.5f);
    
    // 現在のStateの状態を保存
    saveCurrentState(player);
    
    if (s_roomFirstTime) {
        pendingWelcomeMessage = true;
    } else {
        pendingMessage = "自室に戻りました。";
    }
}

void RoomState::exit() {
    ui.clear();
}

void RoomState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // フェード更新処理
    updateFade(deltaTime);
    
    // フェードアウト完了後、状態遷移
    if (isFadingOut && fadeTimer >= fadeDuration) {
        if (s_roomFirstTime) {
            if (stateManager) {
                stateManager->changeState(std::make_unique<CastleState>(player, false));
                s_roomFirstTime = false;
            }
        } else {
            if (stateManager) {
                stateManager->changeState(std::make_unique<TownState>(player));
            }
        }
    }
    
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        TownState::s_nightTimerActive = true;
        TownState::s_nightTimer = nightTimer;
        
        if (nightTimer <= 0.0f) {
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
    
}

void RoomState::render(Graphics& graphics) {
    if (!playerTexture) {
        loadTextures(graphics);
    }
    bool uiJustInitialized = false;
    if (!messageBoard) {
    setupUI(graphics);
        uiJustInitialized = true;
    }
    
    if (uiJustInitialized) {
        if (pendingWelcomeMessage) {
            showWelcomeMessage();
            pendingWelcomeMessage = false;
        } else if (!pendingMessage.empty()) {
            showMessage(pendingMessage);
            pendingMessage.clear();
        }
    }
    
    graphics.setDrawColor(139, 69, 19, 255); // 茶色
    graphics.clear();
    
    drawRoom(graphics);
    drawRoomObjects(graphics);
    drawPlayer(graphics);
    
    if (messageBoard && !messageBoard->getText().empty()) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto roomConfig = config.getRoomConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, roomConfig.messageBoard.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(bgX, bgY, roomConfig.messageBoard.background.width, roomConfig.messageBoard.background.height, true);
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(bgX, bgY, roomConfig.messageBoard.background.width, roomConfig.messageBoard.background.height);
    }

    if (howtooperateBoard && !howtooperateBoard->getText().empty()) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto roomConfig = config.getRoomConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, roomConfig.howToOperateBackground.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(bgX, bgY, roomConfig.howToOperateBackground.width, roomConfig.howToOperateBackground.height, true);
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(bgX, bgY, roomConfig.howToOperateBackground.width, roomConfig.howToOperateBackground.height);
    }
    
    ui.render(graphics);
    
    if (nightTimerActive) {
        CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    }
    
    // フェードオーバーレイを描画
    renderFade(graphics);
    
    graphics.present();
}

void RoomState::handleInput(const InputManager& input) {
    GameState::handleInputTemplate(input, ui, isShowingMessage, moveTimer,
        [this, &input]() { handleMovement(input); },
        [this]() { checkInteraction(); },
        [this]() { 
            // フェードアウト開始（完了時に状態遷移）
            if (!isFading()) {
                startFadeOut(0.5f);
            }
        },
        [this]() { return isNearObject(doorX, doorY); },
        [this]() { clearMessage(); });
}

void RoomState::setupUI(Graphics& graphics) {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto roomConfig = config.getRoomConfig();
    
    int messageX, messageY;
    config.calculatePosition(messageX, messageY, roomConfig.messageBoard.text.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto messageBoardLabel = std::make_unique<Label>(messageX, messageY, "", "default");
    messageBoardLabel->setColor(roomConfig.messageBoard.text.color);
    messageBoardLabel->setText("");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));

    int howToX, howToY;
    config.calculatePosition(howToX, howToY, roomConfig.howToOperateText.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto howtooperateLabel = std::make_unique<Label>(howToX, howToY, "", "default");
    howtooperateLabel->setColor(roomConfig.howToOperateText.color);
    howtooperateLabel->setText("移動: 矢印キー\n調べる/ドアに入る/次のメッセージ: Enter");
    howtooperateBoard = howtooperateLabel.get(); // ポインタを保存
    ui.addElement(std::move(howtooperateLabel));
}

void RoomState::setupRoom() {
    bedX = 1; bedY = 1;      // ベッド（左上）
    deskX = 5; deskY = 1;    // 机（右上）
    chestX = 5; chestY = 2;  // 宝箱（机の下）
    doorX = 3; doorY = 4;    // ドア（下部中央）
}

void RoomState::loadTextures(Graphics& graphics) {
    playerTexture = graphics.loadTexture("assets/textures/characters/player.png", "player");
    deskTexture = graphics.loadTexture("assets/textures/objects/desk.png", "desk");
    chestClosedTexture = graphics.loadTexture("assets/textures/objects/closed_box.png", "chest_closed");
    chestOpenTexture = graphics.loadTexture("assets/textures/objects/open_box.png", "chest_open");
    bedTexture = graphics.loadTexture("assets/textures/objects/bed.png", "bed");
    houseTileTexture = graphics.loadTexture("assets/textures/tiles/housetile.png", "house_tile");
}

void RoomState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void RoomState::checkInteraction() {
    if (isNearObject(bedX, bedY)) {
        interactWithBed();
    }
    else if (isNearObject(deskX, deskY)) {
        interactWithDesk();
    }
    else if (isNearObject(chestX, chestY)) {
        interactWithChest();
    }
    else if (isNearObject(doorX, doorY)) {
        exitToTown();
    }
}

void RoomState::drawRoom(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    graphics.setDrawColor(0, 0, 0, 255); // 黒色
    graphics.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
    
    SDL_Texture* houseTileTexture = graphics.getTexture("house_tile");
    if (houseTileTexture) {
        for (int y = 1; y < ROOM_HEIGHT - 1; y++) {
            for (int x = 1; x < ROOM_WIDTH - 1; x++) {
                graphics.drawTexture(houseTileTexture, ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        graphics.setDrawColor(160, 82, 45, 255); // ライトブラウン
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT, true);
    }
    
    graphics.setDrawColor(101, 67, 33, 255); // ダークブラウン
    for (int x = 0; x < ROOM_WIDTH; x++) {
        graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + (ROOM_HEIGHT - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
    for (int y = 0; y < ROOM_HEIGHT; y++) {
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(ROOM_OFFSET_X + (ROOM_WIDTH - 1) * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

void RoomState::drawRoomObjects(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    SDL_Texture* bedTexture = graphics.getTexture("bed");
    if (bedTexture) {
        graphics.drawTexture(bedTexture, ROOM_OFFSET_X + bedX * TILE_SIZE, ROOM_OFFSET_Y + bedY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(100, 149, 237, 255); // コーンフラワーブルー
        graphics.drawRect(ROOM_OFFSET_X + bedX * TILE_SIZE, ROOM_OFFSET_Y + bedY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + bedX * TILE_SIZE, ROOM_OFFSET_Y + bedY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
    
    SDL_Texture* deskTexture = graphics.getTexture("desk");
    if (deskTexture) {
        graphics.drawTexture(deskTexture, ROOM_OFFSET_X + deskX * TILE_SIZE, ROOM_OFFSET_Y + deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(139, 69, 19, 255);
        graphics.drawRect(ROOM_OFFSET_X + deskX * TILE_SIZE, ROOM_OFFSET_Y + deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + deskX * TILE_SIZE, ROOM_OFFSET_Y + deskY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
    
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
    
    graphics.setDrawColor(160, 82, 45, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void RoomState::drawPlayer(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    if (playerTexture) {
        graphics.drawTexture(playerTexture, ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(0, 255, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
}

bool RoomState::isValidPosition(int x, int y) const {
    if (!GameState::isValidPosition(x, y, 1, 1, ROOM_WIDTH - 1, ROOM_HEIGHT - 1)) {
        return false;
    }
    
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
        // フェードアウト開始（完了時に状態遷移）
        if (!isFading()) {
            startFadeOut(0.5f);
        }
    } else {
        showMessage("町に向かいます...");
        // フェードアウト開始（完了時に状態遷移）
        if (!isFading()) {
            startFadeOut(0.5f);
        }
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

nlohmann::json RoomState::toJson() const {
    nlohmann::json j;
    j["stateType"] = static_cast<int>(StateType::ROOM);
    j["playerX"] = playerX;
    j["playerY"] = playerY;
    j["roomFirstTime"] = s_roomFirstTime;
    return j;
}

void RoomState::fromJson(const nlohmann::json& j) {
    if (j.contains("playerX")) playerX = j["playerX"];
    if (j.contains("playerY")) playerY = j["playerY"];
    if (j.contains("roomFirstTime")) s_roomFirstTime = j["roomFirstTime"];
} 