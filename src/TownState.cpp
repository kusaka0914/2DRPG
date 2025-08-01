#include "TownState.h"
#include "FieldState.h"
#include "MainMenuState.h"
#include <iostream>

TownState::TownState(std::shared_ptr<Player> player) 
    : player(player), playerX(12), playerY(15), moveTimer(0), 
      currentLocation(TownLocation::SQUARE), isShopOpen(false), isInnOpen(false) {
}

void TownState::enter() {
    setupUI();
    setupNPCs();
    setupShopItems();
    std::cout << "街に到着しました！" << std::endl;
}

void TownState::exit() {
    ui.clear();
    shopItems.clear();
}

void TownState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
}

void TownState::render(Graphics& graphics) {
    // 背景色を設定（石畳風）
    graphics.setDrawColor(128, 128, 128, 255);
    graphics.clear();
    
    drawMap(graphics);
    drawBuildings(graphics);
    drawNPCs(graphics);
    drawPlayer(graphics);
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void TownState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // ESCキーでフィールドに戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE)) {
        if (isShopOpen || isInnOpen) {
            isShopOpen = false;
            isInnOpen = false;
            setupUI(); // UIをリセット
            return;
        }
        
        if (stateManager) {
            stateManager->changeState(std::make_unique<FieldState>(player));
        }
        return;
    }
    
    // スペースキーでNPCとの会話
    if (input.isKeyJustPressed(InputKey::SPACE)) {
        checkNPCInteraction();
        return;
    }
    
    // ショップやイン画面でない場合のみ移動
    if (!isShopOpen && !isInnOpen) {
        handleMovement(input);
    }
}

void TownState::setupUI() {
    ui.clear();
    
    // プレイヤー情報表示
    auto playerInfoLabel = std::make_unique<Label>(10, 10, "", "default");
    playerInfoLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(playerInfoLabel));
    
    // 操作説明
    auto controlsLabel = std::make_unique<Label>(10, 550, "方向キー: 移動（街では安全）, スペース: 話す, ESC: フィールドへ", "default");
    controlsLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(controlsLabel));
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
    if (moveTimer > 0) return;
    
    int newX = playerX;
    int newY = playerY;
    
    if (input.isKeyPressed(InputKey::UP)) {
        newY--;
    } else if (input.isKeyPressed(InputKey::DOWN)) {
        newY++;
    } else if (input.isKeyPressed(InputKey::LEFT)) {
        newX--;
    } else if (input.isKeyPressed(InputKey::RIGHT)) {
        newX++;
    } else {
        return; // 移動なし
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
    std::cout << npc.name << ": " << npc.dialogue << std::endl;
    
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
    
    // 宿屋（青い建物）
    graphics.setDrawColor(70, 130, 180, 255);
    graphics.drawRect(14 * TILE_SIZE, 3 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(14 * TILE_SIZE, 3 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, false);
    
    // 教会（白い建物）
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(10 * TILE_SIZE, 8 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(10 * TILE_SIZE, 8 * TILE_SIZE, 4 * TILE_SIZE, 4 * TILE_SIZE, false);
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