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
      bedTexture(nullptr), houseTileTexture(nullptr),
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
    
    // ストーリーメッセージUIを既に見た場合は表示しない
    if (s_roomFirstTime && !player->hasSeenRoomStory) {
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
    
    // ホットリロード対応
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    bool uiJustInitialized = false;
    if (!messageBoard || (!lastReloadState && currentReloadState)) {
        setupUI(graphics);
        uiJustInitialized = true;
    }
    lastReloadState = currentReloadState;
    
    if (uiJustInitialized) {
        if (pendingWelcomeMessage) {
            showWelcomeMessage();
            pendingWelcomeMessage = false;
            // ストーリーメッセージUIが表示されたことを記録（メッセージが閉じられた時に完了とみなす）
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
    
    // インタラクション可能な時に「ENTER」を表示
    if (!isShowingMessage && (isNearObject(bedX, bedY) || isNearObject(deskX, deskY) || isNearObject(doorX, doorY))) {
        const int SCREEN_WIDTH = 1100;
        const int SCREEN_HEIGHT = 650;
        const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;
        const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE;
        const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;
        const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2;
        
        int textX = ROOM_OFFSET_X + playerX * TILE_SIZE + TILE_SIZE / 2;
        int textY = ROOM_OFFSET_Y + (playerY + 1) * TILE_SIZE+5;
        
        // テキストを中央揃えにするため、テキストの幅を取得して調整
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Texture* enterTexture = graphics.createTextTexture("ENTER", "default", textColor);
        if (enterTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(enterTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            // テキストサイズを小さくする（70%に縮小）
            const float SCALE = 0.8f;
            int scaledWidth = static_cast<int>(textWidth * SCALE);
            int scaledHeight = static_cast<int>(textHeight * SCALE);
            
            // パディングを追加
            const int PADDING = 4;
            int bgWidth = scaledWidth + PADDING * 2;
            int bgHeight = scaledHeight + PADDING * 2;
            
            // 背景の位置を計算（中央揃え）
            int bgX = textX - bgWidth / 2;
            int bgY = textY;
            
            // 黒背景を描画
            graphics.setDrawColor(0, 0, 0, 200); // 半透明の黒
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
            
            // 白い枠線を描画
            graphics.setDrawColor(255, 255, 255, 255); // 白
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, false);
            
            // テキストを小さく描画（中央揃え）
            int drawX = textX - scaledWidth / 2;
            int drawY = textY + PADDING;
            graphics.drawTexture(enterTexture, drawX, drawY, scaledWidth, scaledHeight);
            SDL_DestroyTexture(enterTexture);
        } else {
            // フォールバック: drawTextを使用
            graphics.drawText("ENTER", textX, textY, "default", textColor);
        }
    }
    
    if (messageBoard && !messageBoard->getText().empty()) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto roomConfig = config.getRoomConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, roomConfig.messageBoard.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        auto mbConfig = config.getMessageBoardConfig();
        
        graphics.setDrawColor(mbConfig.backgroundColor.r, mbConfig.backgroundColor.g, mbConfig.backgroundColor.b, mbConfig.backgroundColor.a);
        graphics.drawRect(bgX, bgY, roomConfig.messageBoard.background.width, roomConfig.messageBoard.background.height, true);
        graphics.setDrawColor(mbConfig.borderColor.r, mbConfig.borderColor.g, mbConfig.borderColor.b, mbConfig.borderColor.a);
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
    howtooperateLabel->setText("移動: W/A/S/D\n調べる/外に出る/次のメッセージ: ENTER");
    howtooperateBoard = howtooperateLabel.get(); // ポインタを保存
    ui.addElement(std::move(howtooperateLabel));
}

void RoomState::setupRoom() {
    bedX = 1; bedY = 1;      // ベッド（左上）
    deskX = 5; deskY = 1;    // 机（右上）
    doorX = 3; doorY = 4;    // ドア（下部中央）
}

void RoomState::loadTextures(Graphics& graphics) {
    playerTexture = graphics.loadTexture("assets/textures/characters/player.png", "player");
    deskTexture = graphics.loadTexture("assets/textures/objects/desk.png", "desk");
    bedTexture = graphics.loadTexture("assets/textures/objects/bed.png", "bed");
    houseTileTexture = graphics.loadTexture("assets/textures/tiles/housetile.png", "house_tile");
    graphics.loadTexture("assets/textures/objects/door.png", "door");
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
    
    SDL_Texture* doorTexture = graphics.getTexture("door");
    if (doorTexture) {
        graphics.drawTexture(doorTexture, ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(160, 82, 45, 255);
        graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
}

void RoomState::drawPlayer(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 7 * 38 = 266
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 5 * 38 = 190
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 266) / 2 = 417
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 190) / 2 = 230
    
    if (playerTexture) {
        // アスペクト比を保持して縦幅に合わせて描画
        int centerX = ROOM_OFFSET_X + playerX * TILE_SIZE + TILE_SIZE / 2;
        int centerY = ROOM_OFFSET_Y + playerY * TILE_SIZE + TILE_SIZE / 2;
        graphics.drawTextureAspectRatio(playerTexture, centerX, centerY, TILE_SIZE, true, true);
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
        (x == deskX && y == deskY)) {                // 机
        return false;
    }
    
    return true;
}

bool RoomState::isNearObject(int objX, int objY) const {
    // 上下左右のみに限定（斜めは除外）
    int dx = abs(playerX - objX);
    int dy = abs(playerY - objY);
    // 上下左右のみ：dx==0かつdy<=1、またはdx<=1かつdy==0
    return (dx == 0 && dy <= 1) || (dx <= 1 && dy == 0);
}

void RoomState::showWelcomeMessage() {
    showMessage("王様からの手紙\n勇者よ、お主に頼みがある。一度我が城に来てくれないか。\nそこで詳しい話をしよう。では、待っておるぞ。");
}

void RoomState::interactWithBed() {
    showMessage("ふかふかのベッドです。\nここでぐっすり眠っていました。");
    player->heal(player->getMaxHp());
}

void RoomState::interactWithDesk() {
    showMessage("冒険者の心得: 「敵の行動の傾向をしっかりと見極めること！」");
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
    // ストーリーメッセージUIが表示されていた場合、完了したことを記録
    if (s_roomFirstTime && !player->hasSeenRoomStory) {
        player->hasSeenRoomStory = true;
    }
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