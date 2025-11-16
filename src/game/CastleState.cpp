#include "CastleState.h"
#include "DemonCastleState.h"
#include "TownState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "NightState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>

static bool s_castleFirstTime = true;

CastleState::CastleState(std::shared_ptr<Player> player, bool fromNightState)
    : player(player), playerX(6), playerY(4), 
      messageBoard(nullptr), isShowingMessage(false),
      moveTimer(0), dialogueStep(0), isTalkingToKing(false),
      nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      fromNightState(fromNightState), pendingDialogue(false) {
    
    if (fromNightState) {
        kingDialogues = {
            "住人を襲っていた犯人はやはりお主であったか、、我が街の完敗である。\nさあ、お主の好きにするがいい。"
        };
        kingDefeated = false;
        guardLeftDefeated = false;
        guardRightDefeated = false;
        allDefeated = false;
        playerX = 4;
        playerY = 4;
    }
    else if (s_castleFirstTime) {
        kingDialogues = {
        "よく来てくれた勇者よ",
        "実はな、最近街の住民が夜間に失踪する事件が多発しているんだ。",
        "この件、わしは魔王の仕業なのではないかと睨んでおる。",
        "どうか魔王を倒し、この街を守ってくれないか。",
        "勇者よ、よろしく頼む。"
        };
        playerX = 4;
        playerY = 4;
    } else {
        kingDialogues = {};
        playerX = 4;
        playerY = 9;
    }
    
    setupCastle();
}

void CastleState::enter() {
    if (s_castleFirstTime || fromNightState) {
        pendingDialogue = true;
    }
}

void CastleState::exit() {
    
}

void CastleState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
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

void CastleState::render(Graphics& graphics) {
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    bool uiJustInitialized = false;
    if (!messageBoard) {
        setupUI(graphics);
        uiJustInitialized = true;
    }
    
    if (uiJustInitialized && pendingDialogue) {
        startDialogue();
        pendingDialogue = false;
    }
    
    graphics.setDrawColor(240, 240, 220, 255);
    graphics.clear();
    
    drawCastle(graphics);
    drawCastleObjects(graphics);
    drawPlayer(graphics);
    
    if (messageBoard && !messageBoard->getText().empty()) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto castleConfig = config.getCastleConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, castleConfig.messageBoard.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(bgX, bgY, castleConfig.messageBoard.background.width, castleConfig.messageBoard.background.height, true);
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(bgX, bgY, castleConfig.messageBoard.background.width, castleConfig.messageBoard.background.height);
    }
    
    ui.render(graphics);
    
    if (nightTimerActive) {
        CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    }
    graphics.present();
}

void CastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            if (fromNightState && allDefeated) {
                clearMessage();
                if (stateManager) {
                    stateManager->changeState(std::make_unique<DemonCastleState>(player, true));
                }
                return;
            }
            nextDialogue(); // メッセージクリアではなく次の会話に進む
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (isTalkingToKing) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            nextDialogue();
        }
        return;
    }
    
    GameState::handleInputTemplate(input, ui, isShowingMessage, moveTimer,
        [this, &input]() { handleMovement(input); },
        [this]() { checkInteraction(); },
        [this]() { 
            
            exitToTown();
        },
        [this]() { return isNearObject(doorX, doorY); },
        [this]() { clearMessage(); });
}

void CastleState::setupUI(Graphics& graphics) {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto castleConfig = config.getCastleConfig();
    
    int textX, textY;
    config.calculatePosition(textX, textY, castleConfig.messageBoard.text.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto messageBoardLabel = std::make_unique<Label>(textX, textY, "", "default");
    messageBoardLabel->setColor(castleConfig.messageBoard.text.color);
    messageBoardLabel->setText("");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void CastleState::setupCastle() {
    throneX = 4; throneY = 2;        // 王座（中央上部）
    guardLeftX = 2; guardLeftY = 6;   // 左衛兵
    guardRightX = 6; guardRightY = 6; // 右衛兵
    doorX = 4; doorY = 10;            // ドア（下部中央）
}

void CastleState::loadTextures(Graphics& graphics) {
    playerTexture = GameState::loadPlayerTexture(graphics);
    kingTexture = GameState::loadKingTexture(graphics);
    guardTexture = GameState::loadGuardTexture(graphics);
    castleTileTexture = graphics.loadTexture("assets/textures/tiles/castletile.png", "castle_tile");
    
}

void CastleState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void CastleState::checkInteraction() {
    if (fromNightState && !isTalkingToKing) {
        if (isNearObject(throneX, throneY) && !kingDefeated) {
            attackKing();
        }
        else if (isNearObject(guardLeftX, guardLeftY) && !guardLeftDefeated) {
            attackGuardLeft();
        }
        else if (isNearObject(guardRightX, guardRightY) && !guardRightDefeated) {
            attackGuardRight();
        }
    } else {
        if (isNearObject(throneX, throneY)) {
            interactWithThrone();
        }
        else if (isNearObject(guardLeftX, guardLeftY)) {
            interactWithGuard();
        }
        else if (isNearObject(guardRightX, guardRightY)) {
            interactWithGuard();
        }
    }
}

void CastleState::interactWithThrone() {
    if (s_castleFirstTime) {
        startDialogue();
    } else {
        showMessage("王様: 勇者よ、魔王討伐を頼んだぞ！頑張れ！");
    }
}

void CastleState::interactWithGuard() {
    showMessage("衛兵: 王様の城を守るのが私の使命です。");
}

void CastleState::attackKing() {
    kingDefeated = true;
    showMessage("王様を倒しました！");
    checkAllDefeated();
}

void CastleState::attackGuardLeft() {
    guardLeftDefeated = true;
    showMessage("左衛兵を倒しました！");
    checkAllDefeated();
}

void CastleState::attackGuardRight() {
    guardRightDefeated = true;
    showMessage("右衛兵を倒しました！");
    checkAllDefeated();
}

void CastleState::checkAllDefeated() {
    if (kingDefeated && guardLeftDefeated && guardRightDefeated && !allDefeated) {
        allDefeated = true;
        showMessage("ついに街を滅ぼすことに成功しましたね。早速魔王の元へ行って報告しましょう！");
    }
}

void CastleState::exitToTown() {
    if (stateManager) {
        stateManager->changeState(std::make_unique<TownState>(player));
    }
}

void CastleState::startDialogue() {
    isTalkingToKing = true;
    dialogueStep = 0;
    showCurrentDialogue();
}

void CastleState::nextDialogue() {
    dialogueStep++;
    if (dialogueStep >= kingDialogues.size()) {
        // 会話終了
        isTalkingToKing = false;
        hasReceivedQuest = true;
        shouldGoToDemonCastle = true;
        
        clearMessage();

        
        if (stateManager && s_castleFirstTime) {
            stateManager->changeState(std::make_unique<DemonCastleState>(player));
            s_castleFirstTime = false; // 静的変数を更新
        }
    } else {
        showCurrentDialogue();
    }
}

void CastleState::showCurrentDialogue() {
    if (dialogueStep < kingDialogues.size() && (s_castleFirstTime || fromNightState)) {
        showMessage("王様: " + kingDialogues[dialogueStep]);
    }
}

void CastleState::showMessage(const std::string& message) {
    GameState::showMessage(message, messageBoard, isShowingMessage);
}

void CastleState::clearMessage() {
    GameState::clearMessage(messageBoard, isShowingMessage);
}

void CastleState::drawCastle(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    graphics.setDrawColor(0, 0, 0, 255); // 黒色
    graphics.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
    
    SDL_Texture* castleTexture = graphics.getTexture("castle_tile");
    if (castleTexture) {
        graphics.drawTexture(castleTexture, ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT);
    } else {
        graphics.setDrawColor(220, 220, 220, 255); // ライトグレー
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT, true);
        
        graphics.setDrawColor(105, 105, 105, 255); // ディムグレー
        for (int x = 0; x < ROOM_WIDTH; x++) {
            graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y, TILE_SIZE, TILE_SIZE, true);
            graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + (ROOM_HEIGHT - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        }
        for (int y = 0; y < ROOM_HEIGHT; y++) {
            graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            graphics.drawRect(ROOM_OFFSET_X + (ROOM_WIDTH - 1) * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        }
    }
}

void CastleState::drawCastleObjects(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    graphics.setDrawColor(255, 215, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + throneX * TILE_SIZE, ROOM_OFFSET_Y + throneY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + throneX * TILE_SIZE, ROOM_OFFSET_Y + throneY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect((ROOM_OFFSET_X + throneX + 0.25) * TILE_SIZE, (ROOM_OFFSET_Y + throneY + 0.25) * TILE_SIZE, TILE_SIZE * 0.5, TILE_SIZE * 0.5, true);
    
    if (!kingDefeated) {
        if (kingTexture) {
            graphics.drawTexture(kingTexture, ROOM_OFFSET_X + throneX * TILE_SIZE, ROOM_OFFSET_Y + throneY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }
    
    if (!guardLeftDefeated) {
        if (guardTexture) {
            graphics.drawTexture(guardTexture, ROOM_OFFSET_X + guardLeftX * TILE_SIZE, ROOM_OFFSET_Y + guardLeftY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        } else {
            graphics.setDrawColor(192, 192, 192, 255);
            graphics.drawRect(ROOM_OFFSET_X + guardLeftX * TILE_SIZE, ROOM_OFFSET_Y + guardLeftY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(ROOM_OFFSET_X + guardLeftX * TILE_SIZE, ROOM_OFFSET_Y + guardLeftY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }
    
    if (!guardRightDefeated) {
        if (guardTexture) {
            graphics.drawTexture(guardTexture, ROOM_OFFSET_X + guardRightX * TILE_SIZE, ROOM_OFFSET_Y + guardRightY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        } else {
            graphics.setDrawColor(192, 192, 192, 255);
            graphics.drawRect(ROOM_OFFSET_X + guardRightX * TILE_SIZE, ROOM_OFFSET_Y + guardRightY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(ROOM_OFFSET_X + guardRightX * TILE_SIZE, ROOM_OFFSET_Y + guardRightY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }
    
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void CastleState::drawPlayer(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    if (playerTexture) {
        graphics.drawTexture(playerTexture, ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(0, 0, 255, 255);
        graphics.drawRect(ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
}

bool CastleState::isValidPosition(int x, int y) const {
    // 基本的な境界チェック
    if (x < 1 || x >= ROOM_WIDTH - 1 || y < 1 || y >= ROOM_HEIGHT - 1) {
        return false;
    }
    
    if (GameUtils::isColliding(x, y, 1, 1, throneX, throneY, 1, 1)) {
        return false;
    }
    
    if (GameUtils::isColliding(x, y, 1, 1, guardLeftX, guardLeftY, 1, 1)) {
        return false;
    }
    
    if (GameUtils::isColliding(x, y, 1, 1, guardRightX, guardRightY, 1, 1)) {
        return false;
    }
    
    if (GameUtils::isColliding(x, y, 1, 1, doorX, doorY, 2, 1)) {
        return false;
    }
    
    return true;
}

bool CastleState::isNearObject(int x, int y) const {
    return GameState::isNearObject(playerX, playerY, x, y);
} 