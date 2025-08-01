#include "TownState.h"
#include "FieldState.h"
#include "RoomState.h"
#include "MainMenuState.h"
#include "NightState.h"
#include "CastleState.h"
#include <iostream>

TownState::TownState(std::shared_ptr<Player> player)
    : player(player), playerX(12), playerY(15), moveTimer(0), 
      messageBoard(nullptr), isShowingMessage(false),
      isShopOpen(false), isInnOpen(false),
      roomEntranceX(5), roomEntranceY(18), castleX(12), castleY(1), hasVisitedCastle(false) {
    
    // 建物の位置を初期化
    buildings = {
        {8, 8},   // 道具屋
        {15, 8},  // 宿屋
        {22, 8},  // 教会
        {12, 1}   // 城（画面中央上部）
    };
    
    buildingTypes = {"shop", "inn", "church", "castle"};
    
    setupUI();
}

void TownState::enter() {
    setupUI();
    setupNPCs();
    setupShopItems();
    showMessage("街に到着しました！左下の建物が自室です。");
}

void TownState::exit() {
    ui.clear();
    shopItems.clear();
}

void TownState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // ゲームパッド接続状態を更新
    gameControllerConnected = false; // 実際の実装ではInputManagerから取得
}

void TownState::render(Graphics& graphics) {
    // 現在の場所に応じて背景色を設定
    switch (currentLocation) {
        case TownLocation::SQUARE:
            graphics.setDrawColor(128, 128, 128, 255); // 石畳風
            break;
        case TownLocation::SHOP:
            graphics.setDrawColor(139, 69, 19, 255); // 茶色（道具屋）
            break;
        case TownLocation::INN:
            graphics.setDrawColor(70, 130, 180, 255); // 青色（宿屋）
            break;
        case TownLocation::CHURCH:
            graphics.setDrawColor(255, 255, 255, 255); // 白色（教会）
            break;
        default:
            graphics.setDrawColor(128, 128, 128, 255);
            break;
    }
    graphics.clear();
    
    // 場所に応じて描画
    if (currentLocation == TownLocation::SQUARE) {
        drawMap(graphics);
        drawBuildings(graphics);
        drawNPCs(graphics);
    } else {
        drawBuildingInterior(graphics);
    }
    
    drawPlayer(graphics);
    
    // メッセージがある時のみメッセージボードの黒背景を描画
    if (messageBoard && !messageBoard->getText().empty()) {
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(40, 440, 720, 100, true); // メッセージボード背景
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(40, 440, 720, 100); // メッセージボード枠
    }
    
    // UI描画
    ui.render(graphics);
    
    // ゲームパッド接続状態を表示（画面右上）
    if (gameControllerConnected) {
        graphics.setDrawColor(0, 255, 0, 255); // 緑色
        graphics.drawRect(700, 10, 80, 20, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(700, 10, 80, 20, false);
    }
    
    graphics.present();
}

void TownState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            std::cout << "メッセージクリア処理実行" << std::endl;
            clearMessage();
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    // ESCキーでフィールドに戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (isShopOpen || isInnOpen) {
            isShopOpen = false;
            isInnOpen = false;
            setupUI(); // UIをリセット
            return;
        }
        
        // 建物内部にいる場合は広場に戻る
        if (currentLocation != TownLocation::SQUARE) {
            exitBuilding();
            return;
        }
        
        if (stateManager) {
            stateManager->changeState(std::make_unique<FieldState>(player));
        }
        return;
    }
    
    // スペースキーでNPCとの会話、建物への入場、城への入場
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        // 建物内部での特別な操作
        if (currentLocation == TownLocation::INN) {
            player->heal(player->getMaxHp());
            showMessage("宿屋で休みました。HPが全回復しました！");
            return;
        } else if (currentLocation == TownLocation::CHURCH) {
            showMessage("神に祈りを捧げました。心が安らぎます...");
            return;
        } else if (currentLocation == TownLocation::SHOP) {
            openShop();
            return;
        }
        
        // 広場での操作
        if (currentLocation == TownLocation::SQUARE) {
            // まず城のチェック
            int castleDistance = abs(playerX - castleX) + abs(playerY - castleY);
            std::cout << "プレイヤー位置: (" << playerX << ", " << playerY << "), 城位置: (" << castleX << ", " << castleY << "), 距離: " << castleDistance << std::endl;
            if (castleDistance <= 3) {
                checkCastleEntrance();
                return;
            }
            
            // 次に自室への入り口チェック
            checkRoomEntrance();
            
            // 次に建物への入場チェック
            checkBuildingEntrance();
            
            // 最後にNPCとの会話チェック
            checkNPCInteraction();
        }
        return;
    }
    
    // Nキーで夜間モードに入る
    if (input.isKeyJustPressed(InputKey::N)) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<NightState>(player));
        }
        return;
    }
    
    // ショップやイン画面でない場合のみ移動
    if (!isShopOpen && !isInnOpen && currentLocation == TownLocation::SQUARE) {
        if (moveTimer <= 0) {
            handleMovement(input);
        }
    }
}

void TownState::setupUI() {
    ui.clear();
    
    // プレイヤー情報表示
    auto playerInfoLabel = std::make_unique<Label>(10, 10, "", "default");
    playerInfoLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(playerInfoLabel));
    
    // 操作説明（現在の場所に応じて変更）
    std::string controlsText;
    switch (currentLocation) {
        case TownLocation::SQUARE:
            controlsText = "方向キー/WASD: 移動, スペース/Aボタン: 話す/入る, ESC/Bボタン: フィールドへ";
            break;
        case TownLocation::SHOP:
            controlsText = "スペース/Aボタン: 買い物(武器・防具・薬草), ESC/Bボタン: 外に出る";
            break;
        case TownLocation::INN:
            controlsText = "スペース/Aボタン: 休む(HP全回復), ESC/Bボタン: 外に出る";
            break;
        case TownLocation::CHURCH:
            controlsText = "スペース/Aボタン: 祈る(心の回復), ESC/Bボタン: 外に出る";
            break;
        default:
            controlsText = "方向キー/WASD: 移動, スペース/Aボタン: 話す, ESC/Bボタン: フィールドへ";
            break;
    }
    
    auto controlsLabel = std::make_unique<Label>(10, 550, controlsText, "default");
    controlsLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(controlsLabel));
    
    // メッセージボード（黒背景）
    auto messageBoardLabel = std::make_unique<Label>(50, 450, "", "default");
    messageBoardLabel->setColor({255, 255, 255, 255}); // 白文字
    messageBoardLabel->setText("街の中を探索してみましょう");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void TownState::setupNPCs() {
    npcs.clear();
    
    // 道具屋の店主
    npcs.emplace_back(NPCType::SHOPKEEPER, "道具屋のおじさん", 
                      "いらっしゃい！何か買っていくかい？", 8, 5);
    
    // 宿屋の女将
    npcs.emplace_back(NPCType::INNKEEPER, "宿屋のおかみ", 
                      "疲れたでしょう？ゆっくり休んでいって", 16, 5);
    
    // 町の人
    npcs.emplace_back(NPCType::TOWNSPERSON, "町の住人", 
                      "最近魔物が増えて困っているんだ...", 5, 12);
    
    // 衛兵
    npcs.emplace_back(NPCType::GUARD, "衛兵", 
                      "町の平和を守るのが私の仕事だ！", 20, 10);
}

void TownState::setupShopItems() {
    shopItems.clear();
    
    // 消費アイテム
    shopItems.push_back(std::make_unique<ConsumableItem>(ConsumableType::YAKUSOU));
    shopItems.push_back(std::make_unique<ConsumableItem>(ConsumableType::SEISUI));
    shopItems.push_back(std::make_unique<ConsumableItem>(ConsumableType::MAHOU_SEISUI));
    
    // 武器
    shopItems.push_back(std::make_unique<Weapon>(WeaponType::COPPER_SWORD));
    shopItems.push_back(std::make_unique<Weapon>(WeaponType::CLUB));
    
    // 防具
    shopItems.push_back(std::make_unique<Armor>(ArmorType::LEATHER_ARMOR));
    shopItems.push_back(std::make_unique<Armor>(ArmorType::LEATHER_SHIELD));
}

void TownState::handleMovement(const InputManager& input) {
    int newX = playerX;
    int newY = playerY;
    
    // デジタル入力（キーボード）
    if (input.isKeyPressed(InputKey::UP) || input.isKeyPressed(InputKey::W)) {
        newY--;
    } else if (input.isKeyPressed(InputKey::DOWN) || input.isKeyPressed(InputKey::S)) {
        newY++;
    } else if (input.isKeyPressed(InputKey::LEFT) || input.isKeyPressed(InputKey::A)) {
        newX--;
    } else if (input.isKeyPressed(InputKey::RIGHT) || input.isKeyPressed(InputKey::D)) {
        newX++;
    }
    
    // アナログスティック入力（ゲームパッド）
    float leftStickX = input.getLeftStickX();
    float leftStickY = input.getLeftStickY();
    
    // デッドゾーンを設定（小さな動きを無視）
    const float DEADZONE = 0.3f;
    
    if (leftStickX > DEADZONE) {
        newX++;
    } else if (leftStickX < -DEADZONE) {
        newX--;
    }
    
    if (leftStickY > DEADZONE) {
        newY++;
    } else if (leftStickY < -DEADZONE) {
        newY--;
    }
    
    if (isValidPosition(newX, newY)) {
        playerX = newX;
        playerY = newY;
        moveTimer = MOVE_DELAY;
    }
}

void TownState::checkNPCInteraction() {
    NPC* npc = getNearbyNPC(playerX, playerY);
    if (npc) {
        handleNPCDialogue(*npc);
    }
}

void TownState::handleNPCDialogue(const NPC& npc) {
    showMessage(npc.name + ": " + npc.dialogue);
    
    switch (npc.type) {
        case NPCType::SHOPKEEPER:
            openShop();
            break;
        case NPCType::INNKEEPER:
            openInn();
            break;
        case NPCType::TOWNSPERSON:
        case NPCType::GUARD:
            // 普通の会話のみ
            break;
    }
}

void TownState::openShop() {
    isShopOpen = true;
    std::cout << "\n=== 道具屋 ===" << std::endl;
    showShopMenu();
}

void TownState::openInn() {
    isInnOpen = true;
    std::cout << "\n=== 宿屋 ===" << std::endl;
    std::cout << "宿代は10ゴールドです。休みますか？(y/n)" << std::endl;
    
    // HPとMPを全回復
    player->heal(player->getTotalMaxHp());
    player->restoreMp(player->getTotalMaxMp());
    player->gainGold(-10);
    std::cout << "ゆっくり休んで体力が回復しました！" << std::endl;
    
    isInnOpen = false;
}

void TownState::showShopMenu() {
    std::cout << "\n=== 商品一覧 ===" << std::endl;
    for (size_t i = 0; i < shopItems.size(); ++i) {
        std::cout << (i + 1) << ". " << shopItems[i]->getName() 
                  << " - " << shopItems[i]->getPrice() << "G" << std::endl;
    }
    std::cout << "0. やめる" << std::endl;
    std::cout << "現在のゴールド: " << player->getGold() << "G" << std::endl;
}

void TownState::buyItem(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= shopItems.size()) return;
    
    const Item* item = shopItems[itemIndex].get();
    if (player->getGold() >= item->getPrice()) {
        player->gainGold(-item->getPrice());
        auto clonedItem = item->clone();
        if (player->getInventory().addItem(std::move(clonedItem))) {
            std::cout << item->getName() << "を購入しました！" << std::endl;
        } else {
            std::cout << "インベントリがいっぱいです！" << std::endl;
            player->gainGold(item->getPrice()); // ゴールドを返金
        }
    } else {
        std::cout << "ゴールドが足りません！" << std::endl;
    }
}

void TownState::drawMap(Graphics& graphics) {
    // 石畳のタイル描画
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int drawX = x * TILE_SIZE;
            int drawY = y * TILE_SIZE;
            
            // 石畳タイル（グレー）
            graphics.setDrawColor(160, 160, 160, 255);
            graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
            
            // タイルの境界線
            graphics.setDrawColor(100, 100, 100, 255);
            graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, false);
        }
    }
}

void TownState::drawBuildings(Graphics& graphics) {
    // 道具屋（茶色の建物）
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(6 * TILE_SIZE, 3 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(6 * TILE_SIZE, 3 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, false);
    
    // 道具屋の看板（金色で目立つ）
    graphics.setDrawColor(255, 215, 0, 255); // ゴールド色
    graphics.drawRect(6 * TILE_SIZE, 2 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(6 * TILE_SIZE, 2 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, false);
    
    // 宿屋（青い建物、より明るい青）
    graphics.setDrawColor(100, 149, 237, 255); // コーンフラワーブルー
    graphics.drawRect(14 * TILE_SIZE, 3 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(14 * TILE_SIZE, 3 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, false);
    
    // 宿屋の看板（白で清潔感）
    graphics.setDrawColor(255, 255, 255, 255); // 白色
    graphics.drawRect(14 * TILE_SIZE, 2 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(14 * TILE_SIZE, 2 * TILE_SIZE, 4 * TILE_SIZE, TILE_SIZE, false);
    
    // 教会（白い建物、より神聖な印象）
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(10 * TILE_SIZE, 8 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(10 * TILE_SIZE, 8 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, false);
    
    // 教会の十字架（金色で神聖）
    graphics.setDrawColor(255, 215, 0, 255); // ゴールド色
    graphics.drawRect(11 * TILE_SIZE, 7 * TILE_SIZE, 2 * TILE_SIZE, TILE_SIZE, true); // 横棒
    graphics.drawRect(11.5 * TILE_SIZE, 6 * TILE_SIZE, TILE_SIZE, 3 * TILE_SIZE, true); // 縦棒
    
    // 城（金色の大きな建物、画面中央上部）
    graphics.setDrawColor(255, 215, 0, 255); // 金色
    graphics.drawRect(10 * TILE_SIZE, 1 * TILE_SIZE, 6 * TILE_SIZE, 4 * TILE_SIZE, true);
    graphics.setDrawColor(139, 69, 19, 255); // 茶色の境界線
    graphics.drawRect(10 * TILE_SIZE, 1 * TILE_SIZE, 6 * TILE_SIZE, 4 * TILE_SIZE, false);
    
    // 城の旗
    graphics.setDrawColor(255, 0, 0, 255);
    graphics.drawRect(13 * TILE_SIZE, 0 * TILE_SIZE, 1 * TILE_SIZE, 2 * TILE_SIZE, true);
    
    // 城の門
    graphics.setDrawColor(139, 69, 19, 255);
    graphics.drawRect(12 * TILE_SIZE, 4 * TILE_SIZE, 2 * TILE_SIZE, 1 * TILE_SIZE, true);
    
    // 自室の入り口（緑色の小さな建物、家のマーク）
    graphics.setDrawColor(34, 139, 34, 255); // 森林緑
    graphics.drawRect(roomEntranceX * TILE_SIZE, roomEntranceY * TILE_SIZE, 2 * TILE_SIZE, 2 * TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(roomEntranceX * TILE_SIZE, roomEntranceY * TILE_SIZE, 2 * TILE_SIZE, 2 * TILE_SIZE, false);
    
    // 自室のドアマーク
    graphics.setDrawColor(139, 69, 19, 255); // 茶色のドア
    graphics.drawRect((roomEntranceX + 0.5) * TILE_SIZE, (roomEntranceY + 0.5) * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
}

void TownState::drawBuildingInterior(Graphics& graphics) {
    // 建物内部の基本的な内装を描画
    graphics.setDrawColor(0, 0, 0, 255); // 黒い境界線
    
    switch (currentLocation) {
        case TownLocation::SHOP:
            // 道具屋の内装: カウンターと棚、商品
            graphics.setDrawColor(101, 67, 33, 255); // ダークブラウン
            graphics.drawRect(200, 150, 400, 60, true); // カウンター
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(200, 150, 400, 60, false);
            
            // 棚
            graphics.setDrawColor(139, 69, 19, 255);
            graphics.drawRect(100, 100, 80, 200, true);
            graphics.drawRect(620, 100, 80, 200, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(100, 100, 80, 200, false);
            graphics.drawRect(620, 100, 80, 200, false);
            
            // 商品（薬草、武器、防具）
            graphics.setDrawColor(0, 255, 0, 255); // 緑（薬草）
            graphics.drawRect(110, 110, 20, 20, true);
            graphics.drawRect(140, 110, 20, 20, true);
            graphics.drawRect(170, 110, 20, 20, true);
            
            graphics.setDrawColor(192, 192, 192, 255); // 銀色（武器）
            graphics.drawRect(110, 140, 30, 10, true);
            graphics.drawRect(150, 140, 30, 10, true);
            
            graphics.setDrawColor(139, 69, 19, 255); // 茶色（防具）
            graphics.drawRect(110, 160, 25, 15, true);
            graphics.drawRect(145, 160, 25, 15, true);
            break;
            
        case TownLocation::INN:
            // 宿屋の内装: ベッド、テーブル、暖炉
            graphics.setDrawColor(100, 149, 237, 255); // コーンフラワーブルー
            graphics.drawRect(150, 200, 120, 80, true); // ベッド
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(150, 200, 120, 80, false);
            
            graphics.setDrawColor(139, 69, 19, 255);
            graphics.drawRect(450, 220, 100, 60, true); // テーブル
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(450, 220, 100, 60, false);
            
            // 暖炉
            graphics.setDrawColor(139, 69, 19, 255);
            graphics.drawRect(300, 150, 80, 60, true);
            graphics.setDrawColor(255, 69, 0, 255); // オレンジ（火）
            graphics.drawRect(310, 160, 60, 40, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(300, 150, 80, 60, false);
            
            // 椅子
            graphics.setDrawColor(101, 67, 33, 255);
            graphics.drawRect(470, 280, 60, 40, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(470, 280, 60, 40, false);
            break;
            
        case TownLocation::CHURCH:
            // 教会の内装: 祭壇、ベンチ、ステンドグラス風
            graphics.setDrawColor(255, 215, 0, 255); // ゴールド色の祭壇
            graphics.drawRect(350, 100, 100, 80, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(350, 100, 100, 80, false);
            
            // ベンチ
            graphics.setDrawColor(139, 69, 19, 255);
            graphics.drawRect(200, 250, 400, 40, true);
            graphics.drawRect(200, 320, 400, 40, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(200, 250, 400, 40, false);
            graphics.drawRect(200, 320, 400, 40, false);
            
            // ステンドグラス風の窓
            graphics.setDrawColor(135, 206, 235, 255); // スカイブルー
            graphics.drawRect(50, 100, 60, 120, true);
            graphics.drawRect(690, 100, 60, 120, true);
            graphics.setDrawColor(255, 255, 255, 255); // 白い枠
            graphics.drawRect(50, 100, 60, 120, false);
            graphics.drawRect(690, 100, 60, 120, false);
            
            // 十字架
            graphics.setDrawColor(255, 215, 0, 255);
            graphics.drawRect(395, 110, 10, 60, true); // 縦棒
            graphics.drawRect(375, 130, 50, 10, true); // 横棒
            break;
            
        default:
            break;
    }
}

void TownState::drawNPCs(Graphics& graphics) {
    for (const auto& npc : npcs) {
        int drawX = npc.x * TILE_SIZE + 8;
        int drawY = npc.y * TILE_SIZE + 8;
        int size = TILE_SIZE - 16;
        
        // NPCタイプによって色分け
        switch (npc.type) {
            case NPCType::SHOPKEEPER:
                graphics.setDrawColor(255, 165, 0, 255); // オレンジ
                break;
            case NPCType::INNKEEPER:
                graphics.setDrawColor(255, 20, 147, 255); // ピンク
                break;
            case NPCType::TOWNSPERSON:
                graphics.setDrawColor(128, 0, 128, 255); // 紫
                break;
            case NPCType::GUARD:
                graphics.setDrawColor(255, 0, 0, 255); // 赤
                break;
        }
        
        graphics.drawRect(drawX, drawY, size, size, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(drawX, drawY, size, size, false);
    }
}

void TownState::drawPlayer(Graphics& graphics) {
    int drawX = playerX * TILE_SIZE + 4;
    int drawY = playerY * TILE_SIZE + 4;
    int size = TILE_SIZE - 8;
    
    // プレイヤーを青い四角で描画
    graphics.setDrawColor(0, 0, 255, 255);
    graphics.drawRect(drawX, drawY, size, size, true);
    
    // 境界線
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(drawX, drawY, size, size, false);
}

bool TownState::isValidPosition(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return false;
    }
    
    // 建物との衝突判定
    // 道具屋
    if (x >= 6 && x < 10 && y >= 3 && y < 7) return false;
    // 宿屋
    if (x >= 14 && x < 18 && y >= 3 && y < 7) return false;
    // 教会
    if (x >= 10 && x < 14 && y >= 8 && y < 12) return false;
    
    return true;
}

bool TownState::isNearNPC(int x, int y) const {
    for (const auto& npc : npcs) {
        int dx = abs(x - npc.x);
        int dy = abs(y - npc.y);
        if (dx <= 1 && dy <= 1) {
            return true;
        }
    }
    return false;
}

NPC* TownState::getNearbyNPC(int x, int y) {
    for (auto& npc : npcs) {
        int dx = abs(x - npc.x);
        int dy = abs(y - npc.y);
        if (dx <= 1 && dy <= 1) {
            return &npc;
        }
    }
    return nullptr;
}

// メッセージボード関連
void TownState::showMessage(const std::string& message) {
    if (messageBoard) {
        messageBoard->setText(message);
        isShowingMessage = true;
        std::cout << "メッセージを表示: " << message << std::endl;
    }
}

void TownState::clearMessage() {
    if (messageBoard) {
        messageBoard->setText("");
        isShowingMessage = false;
        std::cout << "メッセージをクリアしました" << std::endl;
    }
}

// 自室への入り口関連
void TownState::checkRoomEntrance() {
    int dx = abs(playerX - roomEntranceX);
    int dy = abs(playerY - roomEntranceY);
    
    if (dx <= 1 && dy <= 1) {
        enterRoom();
    }
}

void TownState::enterRoom() {
    showMessage("自室に戻ります...");
    if (stateManager) {
        stateManager->changeState(std::make_unique<RoomState>(player));
    }
}

// 建物への入場関連
void TownState::checkBuildingEntrance() {
    // 建物の近くにいるかチェック
    for (size_t i = 0; i < buildings.size(); ++i) {
        int dx = abs(playerX - buildings[i].first);
        int dy = abs(playerY - buildings[i].second);
        
        if (dx <= 1 && dy <= 1) {
            if (buildingTypes[i] == "shop") {
                enterShop();
                return;
            } else if (buildingTypes[i] == "inn") {
                enterInn();
                return;
            } else if (buildingTypes[i] == "church") {
                enterChurch();
                return;
            }
        }
    }
    
    std::cout << "ここには建物の入り口がありません。" << std::endl;
}

void TownState::enterShop() {
    currentLocation = TownLocation::SHOP;
    setupUI(); // UIを更新
    showMessage("道具屋に入りました。武器、防具、薬草などが売っています。スペースで買い物、ESCで外に出ます。");
}

void TownState::enterInn() {
    currentLocation = TownLocation::INN;
    setupUI(); // UIを更新
    showMessage("宿屋に入りました。暖炉の火が心地よく、疲れた体を休めることができます。スペースで休むとHPが全回復します。ESCで外に出ます。");
}

void TownState::enterChurch() {
    currentLocation = TownLocation::CHURCH;
    setupUI(); // UIを更新
    showMessage("教会に入りました。ステンドグラスから差し込む光が神聖な雰囲気を作り出しています。スペースで祈りを捧げることができます。ESCで外に出ます。");
}

void TownState::exitBuilding() {
    currentLocation = TownLocation::SQUARE;
    setupUI(); // UIを更新
    showMessage("建物を出ました。");
} 

void TownState::checkCastleEntrance() {
    // 城の近くにいるかチェック（城の位置: 12,1）
    int distance = abs(playerX - castleX) + abs(playerY - castleY);
    if (distance <= 3) { // 距離を3に拡大
        std::cout << "城に入ります..." << std::endl;
        if (stateManager) {
            stateManager->changeState(std::make_unique<CastleState>(player));
        }
    } else {
        std::cout << "ここには城の入り口がありません。" << std::endl;
    }
} 