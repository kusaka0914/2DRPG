#include "CastleState.h"
#include "DemonCastleState.h"
#include "TownState.h"
#include "BattleState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "NightState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>
#include <nlohmann/json.hpp>

static bool s_castleFirstTime = true;

CastleState::CastleState(std::shared_ptr<Player> player, bool fromNightState)
    : player(player), playerX(6), playerY(4), 
      messageBoard(nullptr), isShowingMessage(false),
      playerTexture(nullptr), kingTexture(nullptr), guardTexture(nullptr), castleTileTexture(nullptr),
      moveTimer(0), dialogueStep(0), isTalkingToKing(false),
      nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      fromNightState(fromNightState), pendingDialogue(false),
      kingDefeated(false), guardLeftDefeated(false), guardRightDefeated(false), allDefeated(false) {
    isFadingOut = false;
    isFadingIn = false;
    fadeTimer = 0.0f;
    fadeDuration = 0.5f;
    
    if (fromNightState) {
        kingDialogues = {
            "住民を襲っていた犯人はやはりお主であったか・・・我が街の完敗である。\nだが、そのおかげでわしの真の力を発揮する時が来た。\nさあ、わしと2人の側近にコテンパンにされるがよい。　"
        };
        playerX = 4;
        playerY = 4;
    }
    else if (s_castleFirstTime) {
        kingDialogues = {
        "よく来てくれた勇者よ",
        "実はな、最近夜間に街の住民が失踪する事件が多発しているんだ。",
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
    startFadeIn(0.5f);
    // 保存された状態を復元（BattleStateから戻ってきた場合など）
    const nlohmann::json* savedState = player->getSavedGameState();
    if (savedState && !savedState->is_null() && 
        savedState->contains("stateType") && 
        static_cast<StateType>((*savedState)["stateType"]) == StateType::CASTLE) {
        fromJson(*savedState);
    }
    
    // fromNightStateがfalseの場合（通常の初回訪問時）、倒したフラグをリセット
    if (!fromNightState) {
        kingDefeated = false;
        guardLeftDefeated = false;
        guardRightDefeated = false;
        allDefeated = false;
    }
    
    // 全員を倒したかどうかを再確認（状態復元後にメッセージが表示されていない場合に備える）
    if (kingDefeated && guardLeftDefeated && guardRightDefeated) {
        // 全員倒されている場合、allDefeatedをtrueに設定
        if (!allDefeated) {
            allDefeated = true;
        }
        // メッセージはrender()でsetupUI()の後に表示する（messageBoardが初期化されるため）
        
        // 全員倒された場合は、王様との会話は不要なのでpendingDialogueをfalseにする
        pendingDialogue = false;
    } else {
        // まだ全員倒されていない場合、checkAllDefeated()を呼ぶ
        checkAllDefeated();
    }
    
    // ストーリーメッセージUIが完了していない場合は最初から始める（dialogueStepをリセット）
    if (!player->hasSeenCastleStory && isTalkingToKing) {
        dialogueStep = 0;
    }
    
    // 現在のStateの状態を保存
    saveCurrentState(player);
    
    // ストーリーメッセージUIを既に見た場合は表示しない
    // 全員倒されていない場合のみpendingDialogueを設定（全員倒された場合は既にfalseに設定済み）
    if (!allDefeated && (s_castleFirstTime || fromNightState) && !player->hasSeenCastleStory) {
        pendingDialogue = true;
    }
}

void CastleState::exit() {
    
}

void CastleState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // フェード更新処理
    updateFade(deltaTime);
    
    // フェードアウト完了後、状態遷移
    if (isFadingOut && fadeTimer >= fadeDuration) {
        if (fromNightState && allDefeated) {
            if (stateManager) {
                stateManager->changeState(std::make_unique<DemonCastleState>(player, true));
            }
        } else if (s_castleFirstTime) {
            if (stateManager) {
                // 最初の方のイベントで王様の城から魔王の城に入る際はdemon.oggを流す
                stateManager->changeState(std::make_unique<DemonCastleState>(player, true));
                s_castleFirstTime = false;
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

void CastleState::render(Graphics& graphics) {
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    // ホットリロード対応
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    bool uiJustInitialized = false;
    // フォントが読み込まれている場合のみUIをセットアップ
    if (graphics.getFont("default")) {
        if (!messageBoard || (!lastReloadState && currentReloadState)) {
            setupUI(graphics);
            uiJustInitialized = true;
        }
    } else {
        // フォントが読み込まれていない場合は警告を出す（初回のみ）
        static bool fontWarningShown = false;
        if (!fontWarningShown) {
            std::cerr << "警告: フォントが読み込まれていません。UIの表示をスキップします。" << std::endl;
            fontWarningShown = true;
        }
    }
    lastReloadState = currentReloadState;
    
    // 全員倒された場合のメッセージを表示（setupUI()の後、messageBoardが初期化されているため）
    if (messageBoard && kingDefeated && guardLeftDefeated && guardRightDefeated && allDefeated && !isShowingMessage) {
        showMessage("ついに街を滅ぼすことに成功しましたね。早速魔王の元へ行って報告しましょう！");
    }
    
    // 会話を開始（pendingDialogueがtrueの場合、または初回の夜の街からの入場で会話がまだ開始されていない場合）
    // 全員倒された場合は会話を開始しない
    if (!allDefeated && (pendingDialogue || (fromNightState && !player->hasSeenCastleStory && !isTalkingToKing && !isShowingMessage))) {
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
        auto mbConfig = config.getMessageBoardConfig();
        
        graphics.setDrawColor(mbConfig.backgroundColor.r, mbConfig.backgroundColor.g, mbConfig.backgroundColor.b, mbConfig.backgroundColor.a);
        graphics.drawRect(bgX, bgY, castleConfig.messageBoard.background.width, castleConfig.messageBoard.background.height, true);
        graphics.setDrawColor(mbConfig.borderColor.r, mbConfig.borderColor.g, mbConfig.borderColor.b, mbConfig.borderColor.a);
        graphics.drawRect(bgX, bgY, castleConfig.messageBoard.background.width, castleConfig.messageBoard.background.height);
    }
    
    // インタラクション可能な時に「ENTER」を表示
    if (!isShowingMessage && !isTalkingToKing) {
        bool canInteract = false;
        
        // NightStateから来た場合、攻撃可能な対象の近くにいるかチェック
        if (fromNightState) {
            if (isNearObject(throneX, throneY) && !kingDefeated) {
                canInteract = true;
            } else if (isNearObject(guardLeftX, guardLeftY) && !guardLeftDefeated) {
                canInteract = true;
            } else if (isNearObject(guardRightX, guardRightY) && !guardRightDefeated) {
                canInteract = true;
            }
        }
        
        if (canInteract) {
            // ROOM_OFFSETを考慮して座標を計算
            const int SCREEN_WIDTH = 1100;
            const int SCREEN_HEIGHT = 650;
            const int ROOM_PIXEL_WIDTH = ROOM_WIDTH * TILE_SIZE;   // 13 * 38 = 494
            const int ROOM_PIXEL_HEIGHT = ROOM_HEIGHT * TILE_SIZE; // 11 * 38 = 418
            const int ROOM_OFFSET_X = (SCREEN_WIDTH - ROOM_PIXEL_WIDTH) / 2;   // (1100 - 494) / 2 = 303
            const int ROOM_OFFSET_Y = (SCREEN_HEIGHT - ROOM_PIXEL_HEIGHT) / 2; // (650 - 418) / 2 = 116
            
            int textX = ROOM_OFFSET_X + playerX * TILE_SIZE + TILE_SIZE / 2;
            int textY = ROOM_OFFSET_Y + playerY * TILE_SIZE + TILE_SIZE + 5;
            
            // テキストを中央揃えにするため、テキストの幅を取得して調整
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Texture* enterTexture = graphics.createTextTexture("ENTER", "default", textColor);
            if (enterTexture) {
                int textWidth, textHeight;
                SDL_QueryTexture(enterTexture, nullptr, nullptr, &textWidth, &textHeight);
                
                // テキストサイズを小さくする（80%に縮小）
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

void CastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            // 全員を倒した後のメッセージの場合
            if (fromNightState && allDefeated) {
                clearMessage();
                // フェードアウト開始（完了時に状態遷移）
                if (!isFading()) {
                    startFadeOut(0.5f);
                }
                return;
            }
            // 王様との会話中の場合は次の会話に進む
            if (isTalkingToKing) {
                nextDialogue();
            } else {
                // その他のメッセージの場合はクリア
                clearMessage();
            }
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (isTalkingToKing) {
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
    
    // デバッグ: テクスチャの読み込み状況を確認
    if (!playerTexture) {
        std::cerr << "警告: player.pngの読み込みに失敗しました" << std::endl;
    }
    if (!kingTexture) {
        std::cerr << "警告: king.pngの読み込みに失敗しました" << std::endl;
    }
    if (!guardTexture) {
        std::cerr << "警告: guard.pngの読み込みに失敗しました" << std::endl;
    }
    if (!castleTileTexture) {
        std::cerr << "警告: castletile.pngの読み込みに失敗しました" << std::endl;
    }
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
    if (stateManager) {
        // 王様を敵として戦闘を開始（レベル1）
        Enemy kingEnemy(EnemyType::KING);
        kingEnemy.setLevel(1);
        
        // 戦闘に入る前にCastleStateの状態を保存
        saveCurrentState(player);
        
        stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(kingEnemy)));
    }
}

void CastleState::attackGuardLeft() {
    if (stateManager) {
        // 左衛兵を敵として戦闘を開始（レベル110）
        Enemy guardEnemy(EnemyType::GUARD);
        guardEnemy.setLevel(110);
        
        // 戦闘に入る前にCastleStateの状態を保存
        saveCurrentState(player);
        
        stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(guardEnemy)));
    }
}

void CastleState::attackGuardRight() {
    if (stateManager) {
        // 右衛兵を敵として戦闘を開始（レベル110）
        Enemy guardEnemy(EnemyType::GUARD);
        guardEnemy.setLevel(110);
        
        // 戦闘に入る前にCastleStateの状態を保存
        saveCurrentState(player);
        
        stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(guardEnemy)));
    }
}

void CastleState::checkAllDefeated() {
    if (kingDefeated && guardLeftDefeated && guardRightDefeated) {
        if (!allDefeated) {
            allDefeated = true;
            // メッセージはrender()でsetupUI()の後に表示する（messageBoardが初期化されるため）
        }
    }
}

void CastleState::onGuardDefeated(bool isLeft) {
    if (isLeft) {
        guardLeftDefeated = true;
    } else {
        guardRightDefeated = true;
    }
    checkAllDefeated();
}

void CastleState::onKingDefeated() {
    kingDefeated = true;
    checkAllDefeated();
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
        
        // ストーリーメッセージUIが完全に終わったことを記録
        if ((s_castleFirstTime || fromNightState) && !player->hasSeenCastleStory) {
            player->hasSeenCastleStory = true;
        }
        
        clearMessage();
        
        // fromNightStateの場合は、魔王の城への遷移を開始しない
        // 通常の初回訪問の場合のみ、魔王の城に遷移
        if (!fromNightState && s_castleFirstTime) {
            shouldGoToDemonCastle = true;
            // フェードアウト開始（完了時に状態遷移）
            if (!isFading()) {
                startFadeOut(0.5f);
            }
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

nlohmann::json CastleState::toJson() const {
    nlohmann::json j;
    j["stateType"] = static_cast<int>(StateType::CASTLE);
    j["playerX"] = playerX;
    j["playerY"] = playerY;
    j["dialogueStep"] = dialogueStep;
    j["hasReceivedQuest"] = hasReceivedQuest;
    j["isTalkingToKing"] = isTalkingToKing;
    j["shouldGoToDemonCastle"] = shouldGoToDemonCastle;
    j["fromNightState"] = fromNightState;
    j["kingDefeated"] = kingDefeated;
    j["guardLeftDefeated"] = guardLeftDefeated;
    j["guardRightDefeated"] = guardRightDefeated;
    j["allDefeated"] = allDefeated;
    j["castleFirstTime"] = s_castleFirstTime;
    return j;
}

void CastleState::fromJson(const nlohmann::json& j) {
    if (j.contains("playerX")) playerX = j["playerX"];
    if (j.contains("playerY")) playerY = j["playerY"];
    if (j.contains("dialogueStep")) dialogueStep = j["dialogueStep"];
    if (j.contains("hasReceivedQuest")) hasReceivedQuest = j["hasReceivedQuest"];
    if (j.contains("isTalkingToKing")) isTalkingToKing = j["isTalkingToKing"];
    if (j.contains("shouldGoToDemonCastle")) shouldGoToDemonCastle = j["shouldGoToDemonCastle"];
    if (j.contains("fromNightState")) fromNightState = j["fromNightState"];
    if (j.contains("kingDefeated")) kingDefeated = j["kingDefeated"];
    if (j.contains("guardLeftDefeated")) guardLeftDefeated = j["guardLeftDefeated"];
    if (j.contains("guardRightDefeated")) guardRightDefeated = j["guardRightDefeated"];
    if (j.contains("allDefeated")) allDefeated = j["allDefeated"];
    if (j.contains("castleFirstTime")) s_castleFirstTime = j["castleFirstTime"];
    
    // ストーリーメッセージUIが完了していない場合は最初から始める（dialogueStepをリセット）
    if (!player->hasSeenCastleStory && isTalkingToKing) {
        dialogueStep = 0;
    }
    
    // 会話の状態を復元
    if (isTalkingToKing && dialogueStep < kingDialogues.size()) {
        pendingDialogue = true;
    }
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
            // アスペクト比を保持して縦幅に合わせて描画
            int centerX = ROOM_OFFSET_X + throneX * TILE_SIZE + TILE_SIZE / 2;
            int centerY = ROOM_OFFSET_Y + throneY * TILE_SIZE + TILE_SIZE / 2;
            graphics.drawTextureAspectRatio(kingTexture, centerX, centerY, TILE_SIZE, true, true);
        }
    }
    
    if (!guardLeftDefeated) {
        if (guardTexture) {
            // アスペクト比を保持して縦幅に合わせて描画
            int centerX = ROOM_OFFSET_X + guardLeftX * TILE_SIZE + TILE_SIZE / 2;
            int centerY = ROOM_OFFSET_Y + guardLeftY * TILE_SIZE + TILE_SIZE / 2;
            graphics.drawTextureAspectRatio(guardTexture, centerX, centerY, TILE_SIZE, true, true);
        } else {
            graphics.setDrawColor(192, 192, 192, 255);
            graphics.drawRect(ROOM_OFFSET_X + guardLeftX * TILE_SIZE, ROOM_OFFSET_Y + guardLeftY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(ROOM_OFFSET_X + guardLeftX * TILE_SIZE, ROOM_OFFSET_Y + guardLeftY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        }
    }
    
    if (!guardRightDefeated) {
        if (guardTexture) {
            // アスペクト比を保持して縦幅に合わせて描画
            int centerX = ROOM_OFFSET_X + guardRightX * TILE_SIZE + TILE_SIZE / 2;
            int centerY = ROOM_OFFSET_Y + guardRightY * TILE_SIZE + TILE_SIZE / 2;
            graphics.drawTextureAspectRatio(guardTexture, centerX, centerY, TILE_SIZE, true, true);
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
        // アスペクト比を保持して縦幅に合わせて描画
        int centerX = ROOM_OFFSET_X + playerX * TILE_SIZE + TILE_SIZE / 2;
        int centerY = ROOM_OFFSET_Y + playerY * TILE_SIZE + TILE_SIZE / 2;
        graphics.drawTextureAspectRatio(playerTexture, centerX, centerY, TILE_SIZE, true, true);
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
    // 上下左右のみに限定（斜めは除外）
    int dx = abs(playerX - x);
    int dy = abs(playerY - y);
    // 上下左右のみ：dx==0かつdy<=1、またはdx<=1かつdy==0
    return (dx == 0 && dy <= 1) || (dx <= 1 && dy == 0);
} 