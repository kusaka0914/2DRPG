#include "CastleState.h"
#include "TownState.h"
#include <iostream>

CastleState::CastleState(std::shared_ptr<Player> player) 
    : player(player), playerX(400), playerY(500), 
      kingX(400), kingY(100), isTalkingToKing(false), dialogueIndex(0), 
      hasReceivedQuest(false), moveTimer(0.0f), castleRendered(false) {
    
    setupCastle();
}

void CastleState::enter() {
    setupUI();
    
    // プレイヤー位置を設定（タイル座標→画面座標）
    playerX = 400;  // 画面中央下
    playerY = 500;
    
    std::cout << "🏰 王様の城に入りました..." << std::endl;
}

void CastleState::exit() {
    ui.clear();
}

void CastleState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // UI情報を更新（値が変更された場合のみ）
    if (ui.getElements().size() > 0) {
        // プレイヤー情報（変更検出）
        static std::string lastPlayerInfo = "";
        std::string playerInfo = player->getName() + " Lv:" + std::to_string(player->getLevel()) + 
                                " HP:" + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp()) +
                                " 現在地: 王様の城";
        
        if (playerInfo != lastPlayerInfo) {
            static_cast<Label*>(ui.getElements()[0].get())->setText(playerInfo);
            lastPlayerInfo = playerInfo;
        }
    }
}

void CastleState::render(Graphics& graphics) {
    // 背景をクリア（明るい城の雰囲気）
    graphics.setDrawColor(200, 180, 220, 255); // 明るい薄紫
    graphics.clear();
    
    // シンプルな城描画（FieldStateと同様）
    drawSimpleField(graphics);
    drawKing(graphics);
    drawPlayer(graphics);
    
    ui.render(graphics);
}

void CastleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (isTalkingToKing) {
        // 会話中の処理
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::ENTER)) {
            nextDialogue();
        }
    } else {
        // 通常の移動処理
        handleMovement(input);
        
        // 王様との会話
        if (input.isKeyJustPressed(InputKey::SPACE)) {
            if (isNearKing()) {
                handleKingInteraction();
            }
        }
        
        // 城から出る（下部の出口）
        if (input.isKeyJustPressed(InputKey::DOWN) && playerY >= 520) { // 画面下部
            if (hasReceivedQuest) {
                std::cout << "城から町へ向かいます..." << std::endl;
                stateManager->changeState(std::make_unique<TownState>(player));
            } else {
                if (messageBox) {
                    messageBox->setMessage("まだ王様からの依頼を聞いていません！");
                    messageBox->show();
                }
            }
        }
    }
}

void CastleState::setupUI() {
    ui.clear();
    
    // プレイヤー情報表示
    auto playerInfoLabel = std::make_unique<Label>(10, 10, "", "default");
    playerInfoLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(playerInfoLabel));
    
    // 操作説明
    auto controlsLabel = std::make_unique<Label>(10, 550, "方向キー: 移動, スペース: 王様と会話/城から出る, ↓: 町へ移動", "default");
    controlsLabel->setColor({200, 200, 200, 255});
    ui.addElement(std::move(controlsLabel));
    
    // ストーリーメッセージ用のLabel（StoryMessageBoxの代わり）
    auto storyLabel = std::make_unique<Label>(50, 350, "", "default");
    storyLabel->setColor({255, 255, 255, 255});
    storyLabel->setText("王様の城へようこそ！スペースキーで王様と会話できます。");
    ui.addElement(std::move(storyLabel));
    
    // StoryMessageBoxは無効化
    messageBox = nullptr;
}

void CastleState::setupCastle() {
    // シンプルな座標設定（タイル計算なし）
    playerX = 400;  // 画面中央下
    playerY = 500;
    kingX = 400;    // 画面中央上
    kingY = 100;
}

void CastleState::handleMovement(const InputManager& input) {
    if (moveTimer > 0) return;
    
    const int MOVE_SPEED = 32;
    int newX = playerX;
    int newY = playerY;
    
    if (input.isKeyPressed(InputKey::UP)) {
        newY -= MOVE_SPEED;
    } else if (input.isKeyPressed(InputKey::DOWN)) {
        newY += MOVE_SPEED;
    } else if (input.isKeyPressed(InputKey::LEFT)) {
        newX -= MOVE_SPEED;
    } else if (input.isKeyPressed(InputKey::RIGHT)) {
        newX += MOVE_SPEED;
    } else {
        return; // 移動なし
    }
    
    // 画面境界チェック（シンプル）
    if (newX >= 30 && newX <= 750 && newY >= 30 && newY <= 550) {
        playerX = newX;
        playerY = newY;
        moveTimer = 0.2f; // 移動間隔
    }
}

void CastleState::handleKingInteraction() {
    if (!hasReceivedQuest) {
        // 初回の依頼
        startDialogue();
    } else {
        // 2回目以降
        if (messageBox) {
            messageBox->setMessage("勇者よ、魔王討伐を頼んだぞ！頑張れ！");
            messageBox->show();
        }
    }
}

void CastleState::startDialogue() {
    isTalkingToKing = true;
    dialogueIndex = 0;
    
    // 王様からの依頼メッセージ（Labelで表示）
    if (ui.getElements().size() >= 3) {
        static_cast<Label*>(ui.getElements()[2].get())->setText("王様: 勇者よ、魔物の脅威から我が国を救ってくれ！");
    }
}

void CastleState::nextDialogue() {
    dialogueIndex++;
    
    if (dialogueIndex >= 1) { // 1つのメッセージのみ
        endDialogue();
    }
}

void CastleState::endDialogue() {
    isTalkingToKing = false;
    hasReceivedQuest = true;
    
    if (messageBox) {
        messageBox->setMessage("クエストを受注しました！城から出て町へ向かいましょう。");
        messageBox->show();
    }
    
    std::cout << "🎯 王様からの依頼を受けました！" << std::endl;
}

void CastleState::drawSimpleField(Graphics& graphics) {
    // シンプルなフィールド描画（明るい石の床）
    graphics.setDrawColor(220, 220, 220, 255); // 明るいグレーの石床
    graphics.drawRect(0, 0, 800, 600, true);
    
    // 壁（外枠のみ）
    graphics.setDrawColor(120, 120, 120, 255); // 中程度のグレー
    graphics.drawRect(0, 0, 800, 20, true);      // 上壁
    graphics.drawRect(0, 0, 20, 600, true);      // 左壁
    graphics.drawRect(780, 0, 20, 600, true);    // 右壁
    graphics.drawRect(0, 580, 800, 20, true);    // 下壁
    
    // 城らしさを追加（赤いカーペット）
    graphics.setDrawColor(180, 20, 20, 255); // 赤いカーペット
    graphics.drawRect(380, 100, 40, 400, true); // 中央の通路
}

void CastleState::drawPlayer(Graphics& graphics) {
    // プレイヤー（明るい青）
    graphics.setDrawColor(50, 150, 255, 255); // 明るい青
    graphics.drawRect(playerX, playerY, 24, 24, true);
    graphics.setDrawColor(0, 0, 0, 255); // 黒枠
    graphics.drawRect(playerX, playerY, 24, 24, false);
}

void CastleState::drawKing(Graphics& graphics) {
    // 王様（明るい金色）
    graphics.setDrawColor(255, 215, 0, 255); // 金色
    graphics.drawRect(kingX, kingY, 32, 32, true);
    graphics.setDrawColor(0, 0, 0, 255); // 黒枠
    graphics.drawRect(kingX, kingY, 32, 32, false);
    
    // 王冠（明るい黄色）
    graphics.setDrawColor(255, 255, 100, 255); // 明るい黄色
    graphics.drawRect(kingX + 8, kingY - 8, 16, 8, true);
    graphics.setDrawColor(0, 0, 0, 255); // 黒枠
    graphics.drawRect(kingX + 8, kingY - 8, 16, 8, false);
}

bool CastleState::isNearKing() const {
    // シンプルな距離判定
    int dx = abs(playerX - kingX);
    int dy = abs(playerY - kingY);
    return (dx < 60 && dy < 60); // 60ピクセル以内
} 