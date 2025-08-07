#include "CastleState.h"
#include "DemonCastleState.h"
#include "TownState.h"
#include "Graphics.h"
#include "InputManager.h"
#include "NightState.h"
#include <iostream>

// 静的変数（ファイル内で共有）
static bool s_castleFirstTime = true;

CastleState::CastleState(std::shared_ptr<Player> player, bool fromNightState)
    : player(player), playerX(6), playerY(4), 
      messageBoard(nullptr), isShowingMessage(false),
      moveTimer(0), dialogueStep(0), isTalkingToKing(false),
      nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      fromNightState(fromNightState) {
    
    // NightStateから来た場合の処理
    if (fromNightState) {
        kingDialogues = {
            "住人を襲っていた犯人はやはりお主であったか、、我が街の完敗である。さあ、お主の好きにするがいい。"
        };
        kingDefeated = false;
        guardLeftDefeated = false;
        guardRightDefeated = false;
        allDefeated = false;
        playerX = 4;
        playerY = 4;
    }
    // 通常の王様の会話を初期化
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
    setupUI();
}

void CastleState::enter() {
    // 自動で王様との会話を開始（初回のみ、またはNightStateから来た場合）
    if (s_castleFirstTime || fromNightState) {
        startDialogue();
    }
}

void CastleState::exit() {
    
}

void CastleState::update(float deltaTime) {
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
            }
        }
    } else {
        // タイマーが非アクティブな場合、静的変数もクリア
        TownState::s_nightTimerActive = false;
        TownState::s_nightTimer = 0.0f;
    }
    
    // 移動処理
}

void CastleState::render(Graphics& graphics) {
    // 初回のみテクスチャを読み込み
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    // 背景色を設定（明るい城）
    graphics.setDrawColor(240, 240, 220, 255);
    graphics.clear();
    
    drawCastle(graphics);
    drawCastleObjects(graphics);
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
    
    // 夜のタイマーを表示
    if (nightTimerActive) {
        int remainingMinutes = static_cast<int>(nightTimer) / 60;
        int remainingSeconds = static_cast<int>(nightTimer) % 60;
        
        // タイマー背景
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(10, 10, 200, 40, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(10, 10, 200, 40, false);
        
        // タイマーテキスト
        std::string timerText = "夜の街まで: " + std::to_string(remainingMinutes) + ":" + 
                               (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        graphics.drawText(timerText, 20, 20, "default", {255, 255, 255, 255});
    }
    
    // 新しいパラメータを表示（タイマーがアクティブな場合のみ）
    if (nightTimerActive) {
        // パラメータ背景
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(800, 10, 300, 80, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(800, 10, 300, 80, false);
        
        // パラメータテキスト
        std::string mentalText = "メンタル: " + std::to_string(player->getMental());
        std::string demonTrustText = "魔王からの信頼: " + std::to_string(player->getDemonTrust());
        std::string kingTrustText = "王様からの信頼: " + std::to_string(player->getKingTrust());
        
        graphics.drawText(mentalText, 810, 20, "default", {255, 255, 255, 255});
        graphics.drawText(demonTrustText, 810, 40, "default", {255, 100, 100, 255}); // 赤色
        graphics.drawText(kingTrustText, 810, 60, "default", {100, 100, 255, 255}); // 青色
    }
    
    graphics.present();
}

void CastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            // NightStateから来て全員倒した後のメッセージをクリアした場合
            if (fromNightState && allDefeated) {
                clearMessage();
                // 魔王の城に移動
                if (stateManager) {
                    stateManager->changeState(std::make_unique<DemonCastleState>(player, true));
                }
                return;
            }
            nextDialogue(); // メッセージクリアではなく次の会話に進む
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    // 会話中の処理
    if (isTalkingToKing) {
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

void CastleState::setupUI() {
    ui.clear();
    
    // メッセージボード（画面中央下部）
    auto messageBoardLabel = std::make_unique<Label>(210, 500, "", "default"); // メッセージボード背景の左上付近
    messageBoardLabel->setColor({255, 255, 255, 255}); // 白文字
    messageBoardLabel->setText("");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void CastleState::setupCastle() {
    // 城のオブジェクト配置（13x11の部屋、画面中央に配置）
    throneX = 4; throneY = 2;        // 王座（中央上部）
    guardLeftX = 2; guardLeftY = 6;   // 左衛兵
    guardRightX = 6; guardRightY = 6; // 右衛兵
    doorX = 4; doorY = 10;            // ドア（下部中央）
}

void CastleState::loadTextures(Graphics& graphics) {
    // 画像を読み込み
    playerTexture = GameState::loadPlayerTexture(graphics);
    kingTexture = GameState::loadKingTexture(graphics);
    guardTexture = GameState::loadGuardTexture(graphics);
    castleTileTexture = graphics.loadTexture("assets/tiles/castletile.png", "castle_tile");
    
}

void CastleState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void CastleState::checkInteraction() {
    // NightStateから来た場合の戦闘処理
    if (fromNightState && !isTalkingToKing) {
        // 王様との戦闘
        if (isNearObject(throneX, throneY) && !kingDefeated) {
            attackKing();
        }
        // 左衛兵との戦闘
        else if (isNearObject(guardLeftX, guardLeftY) && !guardLeftDefeated) {
            attackGuardLeft();
        }
        // 右衛兵との戦闘
        else if (isNearObject(guardRightX, guardRightY) && !guardRightDefeated) {
            attackGuardRight();
        }
    } else {
        // 通常の相互作用
        // 王座との相互作用
        if (isNearObject(throneX, throneY)) {
            interactWithThrone();
        }
        // 左衛兵との相互作用
        else if (isNearObject(guardLeftX, guardLeftY)) {
            interactWithGuard();
        }
        // 右衛兵との相互作用
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
        
        // メッセージをクリア
        clearMessage();

        
        // 自動で魔王の城に移動
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
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    // 背景を黒で塗りつぶし
    graphics.setDrawColor(0, 0, 0, 255); // 黒色
    graphics.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
    
    // 城の画像を描画
    SDL_Texture* castleTexture = graphics.getTexture("castle_tile");
    if (castleTexture) {
        // 画像がある場合は画像を描画
        graphics.drawTexture(castleTexture, ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT);
    } else {
        // 画像がない場合は色で描画（フォールバック）
        // 部屋の床を描画（大理石風）
        graphics.setDrawColor(220, 220, 220, 255); // ライトグレー
        graphics.drawRect(ROOM_OFFSET_X, ROOM_OFFSET_Y, ROOM_PIXEL_WIDTH, ROOM_PIXEL_HEIGHT, true);
        
        // 部屋の壁を描画（石造り）
        graphics.setDrawColor(105, 105, 105, 255); // ディムグレー
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
}

void CastleState::drawCastleObjects(Graphics& graphics) {
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
    const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
    
    // 王座を描画（金色）
    graphics.setDrawColor(255, 215, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + throneX * TILE_SIZE, ROOM_OFFSET_Y + throneY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + throneX * TILE_SIZE, ROOM_OFFSET_Y + throneY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    
    // 王座の装飾
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect((ROOM_OFFSET_X + throneX + 0.25) * TILE_SIZE, (ROOM_OFFSET_Y + throneY + 0.25) * TILE_SIZE, TILE_SIZE * 0.5, TILE_SIZE * 0.5, true);
    
    // 王様を描画（画像または金色の四角）- 倒していない場合のみ
    if (!kingDefeated) {
        if (kingTexture) {
            graphics.drawTexture(kingTexture, ROOM_OFFSET_X + throneX * TILE_SIZE, ROOM_OFFSET_Y + throneY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }
    
    // 左衛兵を描画（画像または銀色の四角）- 倒していない場合のみ
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
    
    // 右衛兵を描画（画像または銀色の四角）- 倒していない場合のみ
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
    
    // ドアを描画（茶色）
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(ROOM_OFFSET_X + doorX * TILE_SIZE, ROOM_OFFSET_Y + doorY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void CastleState::drawPlayer(Graphics& graphics) {
    // 画面サイズを取得（1100x650）
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // 部屋のサイズを計算
    const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
    const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
    
    // 部屋を画面中央に配置するためのオフセットを計算
    const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
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

bool CastleState::isValidPosition(int x, int y) const {
    // 基本的な境界チェック
    if (x < 1 || x >= ROOM_WIDTH - 1 || y < 1 || y >= ROOM_HEIGHT - 1) {
        return false;
    }
    
    // GameUtilsを使用したオブジェクトとの衝突チェック
    // プレイヤー（1x1）と王座（1x1）の衝突
    if (GameUtils::isColliding(x, y, 1, 1, throneX, throneY, 1, 1)) {
        return false;
    }
    
    // プレイヤー（1x1）と左衛兵（1x1）の衝突
    if (GameUtils::isColliding(x, y, 1, 1, guardLeftX, guardLeftY, 1, 1)) {
        return false;
    }
    
    // プレイヤー（1x1）と右衛兵（1x1）の衝突
    if (GameUtils::isColliding(x, y, 1, 1, guardRightX, guardRightY, 1, 1)) {
        return false;
    }
    
    // プレイヤー（1x1）とドア（2x1）の衝突
    if (GameUtils::isColliding(x, y, 1, 1, doorX, doorY, 2, 1)) {
        return false;
    }
    
    return true;
}

bool CastleState::isNearObject(int x, int y) const {
    return GameState::isNearObject(playerX, playerY, x, y);
} 