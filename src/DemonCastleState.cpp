#include "DemonCastleState.h"
#include "TownState.h"
#include "Graphics.h"
#include "InputManager.h"
#include "BattleState.h"
#include "Enemy.h"
#include <iostream>

// 静的変数（ファイル内で共有）
static bool s_demonCastleFirstTime = true;

DemonCastleState::DemonCastleState(std::shared_ptr<Player> player, bool fromCastleState)
    : player(player), playerX(4), playerY(4), moveTimer(0), // 魔王の目の前に配置
      messageBoard(nullptr), isShowingMessage(false),
      isTalkingToDemon(false), dialogueStep(0), hasReceivedEvilQuest(false),
      playerTexture(nullptr), demonTexture(nullptr), fromCastleState(fromCastleState) {
    
    // 魔王の会話を初期化
    if (fromCastleState) {
        // CastleStateから来た場合（街を滅ぼした後）
        demonDialogues = {
            "ついに街を滅ぼせたか。よくやった、勇者よ。これで我ら2人が世界を支配できる。",
            "どうした、なぜ喜ばない？もうお前の好きなようにしていいのだぞ？",
            "誰がお前を生かしておくと言った？最後はお前だ。覚悟しろ！"
        };
    } else if (s_demonCastleFirstTime) {
        // 初回の場合
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
        // 2回目以降の場合
        demonDialogues = {
            "魔王: また来たな、勇者。"
        };
    }
    
    setupDemonCastle();
    setupUI();
}

void DemonCastleState::enter() {
    // プレイヤーを魔王の目の前に配置
    playerX = 4;
    playerY = 4; // 魔王の目の前
    
    // 自動で魔王との会話を開始（初回のみ）
    if (s_demonCastleFirstTime) {
        startDialogue();
    }
}

void DemonCastleState::exit() {
    
}

void DemonCastleState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
}

void DemonCastleState::render(Graphics& graphics) {
    // 初回のみテクスチャを読み込み
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    // 背景色を設定（暗い魔王の城）
    graphics.setDrawColor(20, 0, 20, 255);
    graphics.clear();
    
    drawDemonCastle(graphics);
    drawDemonCastleObjects(graphics);
    drawPlayer(graphics);
    
    // メッセージがある時のみメッセージボードの黒背景を描画
    if (messageBoard && !messageBoard->getText().empty()) {
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(190, 480, 720, 100, true); // メッセージボード背景（画面中央）
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(190, 480, 720, 100); // メッセージボード枠（画面中央）
    }
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void DemonCastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーで次の会話に進む
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            nextDialogue(); // メッセージクリアではなく次の会話に進む
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
    
    // 共通の入力処理テンプレートを使用
    GameState::handleInputTemplate(input, ui, isShowingMessage, moveTimer,
        [this, &input]() { handleMovement(input); },
        [this]() { checkInteraction(); },
        [this]() { 
            exitToTown();
        },
        [this]() { return isNearObject(doorX, doorY); },
        [this]() { clearMessage(); });
}

void DemonCastleState::setupUI() {
    ui.clear();
    
    // メッセージボード（画面中央下部）
    auto messageBoardLabel = std::make_unique<Label>(210, 500, "", "default"); // メッセージボード背景の左上付近
    messageBoardLabel->setColor({255, 255, 255, 255}); // 白文字
    messageBoardLabel->setText("魔王の城を探索してみましょう");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void DemonCastleState::setupDemonCastle() {
    // 魔王の城のオブジェクト配置（9x11の部屋、画面中央に配置）
    demonX = 4; demonY = 2;        // 魔王（中央上部）
    doorX = 4; doorY = 10;          // ドア（下部中央）
}

void DemonCastleState::loadTextures(Graphics& graphics) {
    // 画像を読み込み
    playerTexture = GameState::loadPlayerTexture(graphics);
    demonTexture = GameState::loadDemonTexture(graphics);
    demonCastleTileTexture = graphics.loadTexture("assets/tiles/demoncastletile.png", "demon_castle_tile");
}

void DemonCastleState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
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
        // TownStateにDemonCastleStateから来たことを通知
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
        
        // メッセージをクリア
        clearMessage();
        
        if (fromCastleState) {
            // CastleStateから来た場合（街を滅ぼした後）は戦闘画面に移行
            if (stateManager) {
                // 魔王とのバトル用のEnemyを作成
                auto demon = std::make_unique<Enemy>(EnemyType::DEMON_LORD);
                
                // BattleStateに移動
                stateManager->changeState(std::make_unique<BattleState>(player, std::move(demon)));
            }
        } else {
            // 通常の場合は街に移動
            if (stateManager) {
                // TownStateにDemonCastleStateから来たことを通知
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
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 9 * 38 = 342
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 342) / 2 = 379
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    // 背景を黒で塗りつぶし
    graphics.setDrawColor(0, 0, 0, 255); // 黒色
    graphics.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
    
    // 魔王の城の建物タイル画像を描画（床のみ）
    SDL_Texture* demonCastleTileTexture = graphics.getTexture("demon_castle_tile");
    if (demonCastleTileTexture) {
        // 建物タイル画像を部屋の床に敷き詰める（壁は除く）
        for (int y = 1; y < ROOM_HEIGHT - 1; y++) {
            for (int x = 1; x < ROOM_WIDTH - 1; x++) {
                graphics.drawTexture(demonCastleTileTexture, ROOM_OFFSET_X + x * TILE_SIZE, ROOM_OFFSET_Y + y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        // 画像がない場合は色で描画（フォールバック）
        graphics.setDrawColor(32, 32, 32, 255); // 暗いグレー
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT, true);
    }
    
    // 部屋の壁を描画（色で）
    graphics.setDrawColor(32, 32, 32, 255); // 暗いグレー
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

void DemonCastleState::drawDemonCastleObjects(Graphics& graphics) {
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 9 * 38 = 342
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 342) / 2 = 379
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    // 魔王の座を描画（暗い金色）
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    
    // 魔王の座の装飾
    graphics.setDrawColor(105, 0, 0, 255);
    graphics.drawRect((ROOM_OFFSET_X + demonX + 0.25) * TILE_SIZE, (ROOM_OFFSET_Y + demonY + 0.25) * TILE_SIZE, TILE_SIZE * 0.5, TILE_SIZE * 0.5, true);
    
    // 魔王を描画（画像または暗い赤色の四角）
    if (demonTexture) {
        graphics.drawTexture(demonTexture, ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        graphics.setDrawColor(139, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(ROOM_OFFSET_X + demonX * TILE_SIZE, ROOM_OFFSET_Y + demonY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    }
    
    // ドアを描画（暗い茶色）
    graphics.setDrawColor(101, 67, 33, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void DemonCastleState::drawPlayer(Graphics& graphics) {
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 9 * 38 = 342
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 342) / 2 = 379
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    // プレイヤーを描画（画像または青色の四角）
    if (playerTexture) {
        graphics.drawTexture(playerTexture, ROOM_OFFSET_X + playerX * TILE_SIZE, ROOM_OFFSET_Y + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        // フォールバック：青色の四角で描画
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