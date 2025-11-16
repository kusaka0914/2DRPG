#include "DemonCastleState.h"
#include "TownState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "BattleState.h"
#include "../entities/Enemy.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>

static bool s_demonCastleFirstTime = true;

DemonCastleState::DemonCastleState(std::shared_ptr<Player> player, bool fromCastleState)
    : player(player), playerX(4), playerY(4), moveTimer(0), // 魔王の目の前に配置
      messageBoard(nullptr), isShowingMessage(false),
      isTalkingToDemon(false), dialogueStep(0), hasReceivedEvilQuest(false),
      playerTexture(nullptr), demonTexture(nullptr), fromCastleState(fromCastleState),
      pendingDialogue(false) {
    
    if (fromCastleState) {
        demonDialogues = {
            "ついに街を滅ぼせたか。よくやった、勇者よ。これで我ら2人が世界を支配できる。",
            "どうした、なぜ喜ばない？もうお前の好きなようにしていいのだぞ？",
            "誰がお前を生かしておくと言った？最後はお前だ。覚悟しろ！"
        };
    } else if (s_demonCastleFirstTime) {
        demonDialogues = {
            "天の声: ん？なにやら勇者と魔王が話しているみたいですよ。",
            "魔王: 王様に魔王討伐を頼まれただと？",
            "魔王: ハハハ、実に愉快だ。よくやったぞ、" + player->getName() + "。",
            "魔王: お前は完全に王様に信用されている。このまま王様に気づかれないように街を滅ぼすのだ。", 
            "魔王: だが、、なにも功績を上げずに街を滅ぼすのはさすがに王様に勘付かれるであろう。",
            "魔王: そこでお前は我が手下たちを倒すことで王様からの信頼を増やすのだ。",
            "魔王: その合間に街の住人を倒していくことで、勘付かれずに街を滅ぼすことができる。",
            "魔王: 頼んだぞ、勇者よ。我々2人で最高の世界を作り上げようじゃないか。",
            "魔王: もしも街を滅ぼせなかった時は、、分かっているだろうな？",
            "天の声: 勇者は魔王より弱いため逆らえないみたいですね、、",
            "天の声: それでは、街を滅ぼすために頑張っていきましょう。",
        };
    } else {
        demonDialogues = {
            "魔王: また来たな、勇者。"
        };
    }
    
    setupDemonCastle();
}

void DemonCastleState::enter() {
    playerX = 4;
    playerY = 4; // 魔王の目の前
    
    if (s_demonCastleFirstTime || fromCastleState) {
        pendingDialogue = true;
    }
}

void DemonCastleState::exit() {
    
}

void DemonCastleState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
}

void DemonCastleState::render(Graphics& graphics) {
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
    
    graphics.setDrawColor(20, 0, 20, 255);
    graphics.clear();
    
    drawDemonCastle(graphics);
    drawDemonCastleObjects(graphics);
    drawPlayer(graphics);
    
    if (messageBoard && !messageBoard->getText().empty()) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto demonCastleConfig = config.getDemonCastleConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, demonCastleConfig.messageBoard.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(bgX, bgY, demonCastleConfig.messageBoard.background.width, demonCastleConfig.messageBoard.background.height, true);
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(bgX, bgY, demonCastleConfig.messageBoard.background.width, demonCastleConfig.messageBoard.background.height);
    }
    
    ui.render(graphics);
    
    graphics.present();
}

void DemonCastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            nextDialogue(); // メッセージクリアではなく次の会話に進む
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (isTalkingToDemon) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
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

void DemonCastleState::setupUI(Graphics& graphics) {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto demonCastleConfig = config.getDemonCastleConfig();
    
    int textX, textY;
    config.calculatePosition(textX, textY, demonCastleConfig.messageBoard.text.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto messageBoardLabel = std::make_unique<Label>(textX, textY, "", "default");
    messageBoardLabel->setColor(demonCastleConfig.messageBoard.text.color);
    messageBoardLabel->setText("魔王の城を探索してみましょう");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void DemonCastleState::setupDemonCastle() {
    demonX = 4; demonY = 2;        // 魔王（中央上部）
    doorX = 4; doorY = 10;          // ドア（下部中央）
}

void DemonCastleState::loadTextures(Graphics& graphics) {
    playerTexture = GameState::loadPlayerTexture(graphics);
    demonTexture = GameState::loadDemonTexture(graphics);
    demonCastleTileTexture = graphics.loadTexture("assets/textures/tiles/demoncastletile.png", "demon_castle_tile");
}

void DemonCastleState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void DemonCastleState::checkInteraction() {
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
        TownState::s_fromDemonCastle = true;
        auto townState = std::make_unique<TownState>(player);
        stateManager->changeState(std::move(townState));
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
        
        clearMessage();
        
        if (fromCastleState) {
            if (stateManager) {
                auto demon = std::make_unique<Enemy>(EnemyType::DEMON_LORD);
                
                stateManager->changeState(std::make_unique<BattleState>(player, std::move(demon)));
            }
        } else {
            if (stateManager) {
                TownState::s_fromDemonCastle = true;
                auto townState = std::make_unique<TownState>(player);
                stateManager->changeState(std::move(townState));
            }
        }
    } else {
        showCurrentDialogue();
    }
}

void DemonCastleState::showCurrentDialogue() {
    if (dialogueStep < demonDialogues.size()) {
        showMessage(demonDialogues[dialogueStep]);
    }
}

void DemonCastleState::showMessage(const std::string& message) {
    GameState::showMessage(message, messageBoard, isShowingMessage);
}

void DemonCastleState::clearMessage() {
    GameState::clearMessage(messageBoard, isShowingMessage);
}

void DemonCastleState::drawDemonCastle(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 9 * 38 = 342
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 342) / 2 = 379
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    graphics.setDrawColor(0, 0, 0, 255); // 黒色
    graphics.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
    
    SDL_Texture* demonCastleTileTexture = graphics.getTexture("demon_castle_tile");
    if (demonCastleTileTexture) {
        for (int y = 1; y < ROOM_HEIGHT - 1; y++) {
            for (int x = 1; x < ROOM_WIDTH - 1; x++) {
                graphics.drawTexture(demonCastleTileTexture, ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        graphics.setDrawColor(32, 32, 32, 255); // 暗いグレー
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT, true);
    }
    
    graphics.setDrawColor(32, 32, 32, 255); // 暗いグレー
    for (int x = 0; x < ROOM_WIDTH; x++) {
        graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + (ROOM_HEIGHT - 1) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
    for (int y = 0; y < ROOM_HEIGHT; y++) {
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.drawRect(ROOM_OFFSET_X + (ROOM_WIDTH - 1) * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

void DemonCastleState::drawDemonCastleObjects(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 9 * 38 = 342
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 342) / 2 = 379
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    
    graphics.setDrawColor(105, 0, 0, 255);
    graphics.drawRect((ROOM_OFFSET_X + demonX + 0.25) * TILE_SIZE, (ROOM_OFFSET_Y + demonY + 0.25) * TILE_SIZE, TILE_SIZE * 0.5, TILE_SIZE * 0.5, true);
    
    if (demonTexture) {
        graphics.drawTexture(demonTexture, ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(139, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
    
    graphics.setDrawColor(101, 67, 33, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void DemonCastleState::drawPlayer(Graphics& graphics) {
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 9 * 38 = 342
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 342) / 2 = 379
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

bool DemonCastleState::isValidPosition(int x, int y) const {
    return GameState::isValidPosition(x, y, 1, 1, ROOM_WIDTH - 1, ROOM_HEIGHT - 1);
}

bool DemonCastleState::isNearObject(int x, int y) const {
    return GameState::isNearObject(playerX, playerY, x, y);
} 