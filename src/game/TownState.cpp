#include "TownState.h"
#include "FieldState.h"
#include "RoomState.h"
#include "MainMenuState.h"
#include "NightState.h"
#include "CastleState.h"
#include "GameOverState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>

// 静的変数の初期化
bool TownState::s_nightTimerActive = false;
float TownState::s_nightTimer = 0.0f;
int TownState::s_targetLevel = 0;
bool TownState::s_levelGoalAchieved = false;
bool TownState::s_fromDemonCastle = false;
int TownState::s_nightCount = 0; // 夜の回数を追跡
bool TownState::saved = false; // セーブ状態の管理
static bool s_townFirstTime = true; // 初回街入場フラグ
std::vector<std::pair<int, int>> residentHomes = {
    {1, 6}, {5, 5}, {9, 7}, {3, 9}, {17, 7}, {21, 5}, {25, 6},{4, 12},{22,12}
};

TownState::TownState(std::shared_ptr<Player> player)
    : player(player), playerX(TownLayout::PLAYER_START_X), playerY(TownLayout::PLAYER_START_Y), 
      messageBoard(nullptr), isShowingMessage(false), 
      moveTimer(0), nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      showGameExplanation(false), explanationStep(0),
      pendingWelcomeMessage(false), pendingMessage("") {
    
    // 共通配置データを使用
    buildings = TownLayout::BUILDINGS;
    buildingTypes = TownLayout::BUILDING_TYPES;
    residentHomes = TownLayout::RESIDENT_HOMES;
    
    // setupUI()はrender()でGraphicsが利用可能になってから呼ばれる
    s_targetLevel = 25 * (s_nightCount+1);
    // s_targetLevel = 1;
}

void TownState::enter() {
    // setupUI()はrender()でGraphicsが利用可能になってから呼ばれる
    setupNPCs();
    setupShopItems();
    
    // タイマーの状態を復元
    nightTimerActive = s_nightTimerActive;
    nightTimer = s_nightTimer;
    
    // DemonCastleStateから来た場合、夜のタイマーを起動
    if (s_fromDemonCastle) {
        startNightTimer();
        s_fromDemonCastle = false; // フラグをリセット
    }
    
    // 初回メッセージはsetupUI()後に表示するため、フラグを設定
    if (s_townFirstTime) {
        pendingWelcomeMessage = true;
        s_townFirstTime = false;
    }
}

void TownState::exit() {
    try {
        // UIのクリーンアップ
        ui.clear();
        
        // ショップアイテムのクリーンアップ
        shopItems.clear();
        
        // NPCのクリーンアップ
        npcs.clear();
        
        // 建物データのクリーンアップ
        buildings.clear();
        buildingTypes.clear();
        residentHomes.clear();
        
        // テクスチャポインタのクリーンアップ
        playerTexture = nullptr;
        shopTexture = nullptr;
        weaponShopTexture = nullptr;
        houseTexture = nullptr;
        castleTexture = nullptr;
        stoneTileTexture = nullptr;
        guardTexture = nullptr;
        toriiTexture = nullptr;
        residentHomeTexture = nullptr;
        
        for (int i = 0; i < 6; ++i) {
            residentTextures[i] = nullptr;
        }
        
        // メッセージボードのクリーンアップ
        messageBoard = nullptr;
    } catch (const std::exception& e) {
        // エラーハンドリング（必要に応じてログ出力）
    }
}

void TownState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // 信頼度チェック
    checkTrustLevels();
    
    // ゲームパッド接続状態を更新
    gameControllerConnected = false; // 実際の実装ではInputManagerから取得
    
    // 夜のタイマーを更新
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        // 静的変数に状態を保存
        s_nightTimerActive = true;
        s_nightTimer = nightTimer;
        
        // 目標レベルチェック
        if (player->getLevel() >= s_targetLevel && !s_levelGoalAchieved) {
            s_levelGoalAchieved = true;
            showMessage("目標レベル" + std::to_string(s_targetLevel) + "を達成しました！");
        }
        
        if (nightTimer <= 0.0f) {
            // 5分経過したら夜の街に移動
            nightTimerActive = false;
            s_nightTimerActive = false;
            s_nightTimer = 0.0f;
            
            // 目標達成していない場合はゲームオーバー
            if (!s_levelGoalAchieved) {
                if (stateManager) {
                    stateManager->changeState(std::make_unique<GameOverState>(player, "目標レベルに達しませんでした。街に戻る途中でモンスターに襲われました。"));
                }
            } else {
                if (stateManager) {
                    // 現在の状態をクリーンアップしてから新しい状態に移行
                    exit();
                    stateManager->changeState(std::make_unique<NightState>(player));
                    player->setCurrentNight(player->getCurrentNight() + 1);
                    s_nightCount++;
                    s_levelGoalAchieved = false;
                }
            }
        }
    } else {
        // タイマーが非アクティブな場合、静的変数もクリア
        s_nightTimerActive = false;
        s_nightTimer = 0.0f;
    }
    
    // 広場にいる時のみ入り口チェック
    if (currentLocation == TownLocation::SQUARE) {
        checkRoomEntrance();
        checkFieldEntrance();
    }
}

void TownState::render(Graphics& graphics) {
    // 初回のみテクスチャを読み込み
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    // UIが未初期化の場合は初期化
    bool uiJustInitialized = false;
    if (!messageBoard) {
        setupUI(graphics);
        uiJustInitialized = true;
    }
    
    // 初回のみ保留中のメッセージを表示
    if (uiJustInitialized) {
        if (pendingWelcomeMessage) {
            showMessage("街に到着しました。\nここでは道具屋、宿屋、教会など様々な施設を利用できます。\n城に向かって冒険を始めましょう。");
            pendingWelcomeMessage = false;
        } else if (!pendingMessage.empty()) {
            showMessage(pendingMessage);
            pendingMessage.clear();
        }
    }
    
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
        // ゲートはdrawMap内で描画されるため、ここでは削除
    } else {
        drawBuildingInterior(graphics);
    }
    
    drawPlayer(graphics);
    
    // メッセージがある時のみメッセージボードの黒背景を描画（JSONから座標を取得）
    if (messageBoard && !messageBoard->getText().empty()) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto mbConfig = config.getMessageBoardConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, mbConfig.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        graphics.setDrawColor(0, 0, 0, 255); // 黒色
        graphics.drawRect(bgX, bgY, mbConfig.background.width, mbConfig.background.height, true); // メッセージボード背景
        graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
        graphics.drawRect(bgX, bgY, mbConfig.background.width, mbConfig.background.height); // メッセージボード枠
    }
    
    // UI描画
    ui.render(graphics);
    
    // 共通UIを描画
    CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, showGameExplanation);
    CommonUI::drawTargetLevel(graphics, s_targetLevel, s_levelGoalAchieved, player->getLevel());
    CommonUI::drawTrustLevels(graphics, player, nightTimerActive, showGameExplanation);
    CommonUI::drawGameControllerStatus(graphics, gameControllerConnected);
    
    graphics.present();
}

void TownState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // ゲーム説明表示中の処理
    if (showGameExplanation) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            explanationStep++;
            
            if (explanationStep >= gameExplanationTexts.size()) {
                // 説明終了、タイマー開始
                showGameExplanation = false;
                explanationStep = 0;
                clearMessage();
                
                // タイマーを開始
                nightTimerActive = true;
                nightTimer = NIGHT_TIMER_DURATION;
                s_nightTimerActive = true;
                s_nightTimer = NIGHT_TIMER_DURATION;
                
                // showMessage("タイマーが開始されました。5分後に夜の街に移動します。\n目標レベル: " + std::to_string(s_targetLevel));
            } else {
                // 次の説明テキストを表示
                showMessage(gameExplanationTexts[explanationStep]);
            }
        }
        return; // 説明中は他の操作を無効化
    }
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    // ESCキーでフィールドに戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (isShopOpen || isInnOpen) {
            isShopOpen = false;
            isInnOpen = false;
            // setupUI()はrender()で呼ばれるため、ここではスキップ
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
            if (GameUtils::isNearPositionEuclidean(playerX, playerY, castleX, castleY, 3.0f)) {
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
            // 現在の状態をクリーンアップしてから新しい状態に移行
            exit();
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

void TownState::setupUI(Graphics& graphics) {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    
    // プレイヤー情報表示（JSONから座標を取得）
    auto townConfig = config.getTownConfig();
    int playerInfoX, playerInfoY;
    config.calculatePosition(playerInfoX, playerInfoY, townConfig.playerInfo.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto playerInfoLabel = std::make_unique<Label>(playerInfoX, playerInfoY, "", "default");
    playerInfoLabel->setColor(townConfig.playerInfo.color);
    ui.addElement(std::move(playerInfoLabel));
    
    // 操作説明（現在の場所に応じて変更、JSONから座標を取得）
    std::string controlsText;
    switch (currentLocation) {
        case TownLocation::SHOP:
            controlsText = "スペース/Aボタン: 買い物(武器・防具・薬草), ESC/Bボタン: 外に出る";
            break;
        case TownLocation::INN:
            controlsText = "スペース/Aボタン: 休む(HP全回復), ESC/Bボタン: 外に出る";
            break;
        case TownLocation::CHURCH:
            controlsText = "スペース/Aボタン: 祈る(心の回復), ESC/Bボタン: 外に出る";
            break;
    }
    
    int controlsX, controlsY;
    config.calculatePosition(controlsX, controlsY, townConfig.controls.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto controlsLabel = std::make_unique<Label>(controlsX, controlsY, controlsText, "default");
    controlsLabel->setColor(townConfig.controls.color);
    ui.addElement(std::move(controlsLabel));
    
    // メッセージボード（JSONから座標を取得）
    auto mbConfig = config.getMessageBoardConfig();
    
    int textX, textY;
    config.calculatePosition(textX, textY, mbConfig.text.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    
    auto messageBoardLabel = std::make_unique<Label>(textX, textY, "", "default");
    messageBoardLabel->setColor(mbConfig.text.color);
    messageBoardLabel->setText("");
    messageBoard = messageBoardLabel.get(); // ポインタを保存
    ui.addElement(std::move(messageBoardLabel));
}

void TownState::setupNPCs() {
    npcs.clear();
    
    // 住民の位置をTownLayoutから取得
    std::vector<std::string> residentNames = {
        "町の住人1", "町の住人2", "町の住人3", "町の住人4", "町の住人5", "町の住人6", "町の住人7", "町の住人8", "町の住人9", "町の住人10", "町の住人11", "町の住人12"
    };
    
    std::vector<std::string> residentDialogues = {
        "最近魔物が増えて困っているんだ...",
        "今日は良い天気だね。",
        "王様の城は立派だね。",
        "冒険者さん、頑張ってね！",
        "街は平和でいいね。",
        "今日も気持ちがいい天気！",
        "最近魔物が増えて困っているんだ...",
        "今日は良い天気だね。",
        "王様の城は立派だね。",
        "冒険者さん、頑張ってね！",
        "街は平和でいいね。",
        "今日も気持ちがいい天気！"
    };
    
    // 住民をTownLayoutの位置に配置（倒された住民は除外）
    for (size_t i = 0; i < TownLayout::RESIDENTS.size() && i < residentNames.size(); ++i) {
        const auto& pos = TownLayout::RESIDENTS[i];
        
        // 倒された住民の位置かチェック
        bool isKilled = false;
        const auto& killedPositions = NightState::getKilledResidentPositions();
        for (const auto& killedPos : killedPositions) {
            if (killedPos.first == pos.first && killedPos.second == pos.second) {
                isKilled = true;
                break;
            }
        }
        
        // 倒されていない住民のみ配置
        if (!isKilled) {
            npcs.emplace_back(NPCType::TOWNSPERSON, residentNames[i], 
                              residentDialogues[i], pos.first, pos.second);
        }
    }
    
    // 衛兵の位置をTownLayoutから取得
    std::vector<std::string> guardNames = {
        "衛兵1", "衛兵2", "衛兵3", "衛兵4"
    };
    
    std::vector<std::string> guardDialogues = {
        "町の平和を守るのが私の仕事だ！",
        "何か困ったことがあれば声をかけてくれ。",
        "街の見回りは大切な仕事だ。",
        "町の平和を守るのが私の仕事だ！"
    };
    
    // 衛兵をTownLayoutの位置に配置
    for (size_t i = 0; i < TownLayout::GUARDS.size() && i < guardNames.size(); ++i) {
        const auto& pos = TownLayout::GUARDS[i];
        npcs.emplace_back(NPCType::GUARD, guardNames[i], 
                          guardDialogues[i], pos.first, pos.second);
    }
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
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
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
    showShopMenu();
}

void TownState::openInn() {
    isInnOpen = true;
    
    // HPとMPを全回復
    player->heal(player->getTotalMaxHp());
    player->restoreMp(player->getTotalMaxMp());
    player->gainGold(-10);
    
    isInnOpen = false;
}

void TownState::showShopMenu() {
    
}

void TownState::buyItem(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= shopItems.size()) return;
    
    const Item* item = shopItems[itemIndex].get();
    if (player->getGold() >= item->getPrice()) {
        player->gainGold(-item->getPrice());
        auto clonedItem = item->clone();
        if (player->getInventory().addItem(std::move(clonedItem))) {

        } else {
            
            player->gainGold(item->getPrice()); // ゴールドを返金
        }
    } else {
        
    }
}

void TownState::loadTextures(Graphics& graphics) {
    // 画像を読み込み
    playerTexture = GameState::loadPlayerTexture(graphics);
    shopTexture = graphics.loadTexture("assets/textures/buildings/shop.png", "shop");
    weaponShopTexture = graphics.loadTexture("assets/textures/buildings/weaponshop.png", "weapon_shop");
    houseTexture = graphics.loadTexture("assets/textures/buildings/house.png", "house");
    castleTexture = graphics.loadTexture("assets/textures/buildings/castle.png", "castle");    
    stoneTileTexture = graphics.loadTexture("assets/textures/tiles/stonetile.png", "stone_tile");
    
    // 住人画像を読み込み
    residentTextures[0] = graphics.getTexture("resident_1");
    residentTextures[1] = graphics.getTexture("resident_2");
    residentTextures[2] = graphics.getTexture("resident_3");
    residentTextures[3] = graphics.getTexture("resident_4");
    residentTextures[4] = graphics.getTexture("resident_5");
    residentTextures[5] = graphics.getTexture("resident_6");
    
    // 衛兵画像を読み込み
    guardTexture = graphics.getTexture("guard");
    
    // 鳥居画像を読み込み
    toriiTexture = graphics.getTexture("torii");
    
    // 住人の家画像を読み込み
    residentHomeTexture = graphics.getTexture("resident_home");
}

void TownState::drawMap(Graphics& graphics) {
    // 石タイルの画像描画
    if (stoneTileTexture) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                int drawX = x * TILE_SIZE;
                int drawY = y * TILE_SIZE;
                
                // 石タイル画像を描画
                graphics.drawTexture(stoneTileTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        // フォールバック：石畳のタイル描画（グレー）
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
    
    // ゲート（出口）の描画
    drawGate(graphics);
}

void TownState::drawBuildings(Graphics& graphics) {
    for (size_t i = 0; i < buildings.size(); ++i) {
        int x = buildings[i].first;
        int y = buildings[i].second;
        std::string type = buildingTypes[i];
        
        // 建物を2タイルサイズで描画
        if (type == "shop" && shopTexture) {
            graphics.drawTexture(shopTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        } else if (type == "weapon_shop" && weaponShopTexture) {
            graphics.drawTexture(weaponShopTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        } else if (type == "house" && houseTexture) {
            graphics.drawTexture(houseTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        } else if (type == "castle" && castleTexture) {
            graphics.drawTexture(castleTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        } else {
            // フォールバック：色付きの四角
            Uint8 r, g, b;
            if (type == "shop") {
                r = 139; g = 69; b = 19; // 茶色
            } else if (type == "weapon_shop") {
                r = 192; g = 192; b = 192; // 銀色
            } else if (type == "house") {
                r = 34; g = 139; b = 34; // 緑色
            } else if (type == "castle") {
                r = 255; g = 215; b = 0; // 金色
            } else {
                r = 128; g = 128; b = 128; // グレー
            }
            
            graphics.setDrawColor(r, g, b, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        }
    }
    
    for (const auto& home : residentHomes) {
        int x = home.first;
        int y = home.second;
        
        if (residentHomeTexture) {
            // 住人の家画像を描画（2タイルサイズ）
            graphics.drawTexture(residentHomeTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        } else {
            // フォールバック：茶色の四角
            graphics.setDrawColor(139, 69, 19, 255); // 茶色
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            BUILDING_SIZE * TILE_SIZE, BUILDING_SIZE * TILE_SIZE);
        }
    }
}

void TownState::drawGate(Graphics& graphics) {
    // ゲートの位置（出口）
    int gateX = TownLayout::GATE_X;
    int gateY = TownLayout::GATE_Y;
    
    // 石のタイルを描画（背景）
    if (stoneTileTexture) {
        int drawX = gateX * TILE_SIZE;
        int drawY = gateY * TILE_SIZE;
        graphics.drawTexture(stoneTileTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
    }
    
    // 鳥居画像を描画（前景）
    if (toriiTexture) {
        int drawX = gateX * TILE_SIZE;
        int drawY = gateY * TILE_SIZE;
        // 鳥居は少し大きく描画（2タイルサイズ）
        graphics.drawTexture(toriiTexture, drawX - TILE_SIZE/2, drawY - TILE_SIZE, 
                           TILE_SIZE * 2, TILE_SIZE * 2);
    } else {
        // フォールバック：鳥居の代わりに赤い四角を描画
        int drawX = gateX * TILE_SIZE + TILE_SIZE/4;
        int drawY = gateY * TILE_SIZE - TILE_SIZE/2;
        graphics.setDrawColor(255, 0, 0, 255); // 赤色
        graphics.drawRect(drawX, drawY, TILE_SIZE * 1.5, TILE_SIZE * 1.5, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(drawX, drawY, TILE_SIZE * 1.5, TILE_SIZE * 1.5, false);
    }
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
    for (size_t i = 0; i < npcs.size(); i++) {
        const auto& npc = npcs[i];
        int drawX = npc.x * TILE_SIZE;
        int drawY = npc.y * TILE_SIZE;
        int size = TILE_SIZE;  // 38pxで描画
        
        // NPCタイプによって画像または色分け
        switch (npc.type) {
            case NPCType::SHOPKEEPER:
                graphics.setDrawColor(255, 165, 0, 255); // オレンジ
                break;
            case NPCType::INNKEEPER:
                graphics.setDrawColor(255, 20, 147, 255); // ピンク
                break;
            case NPCType::TOWNSPERSON:
                // 住人画像を使用（インデックスを循環使用）
                {
                    int residentIndex = (i - 2) % 5; // 最初の2つ（店主と女将）を除く
                    if (residentIndex >= 0 && residentIndex < 5 && residentTextures[residentIndex]) {
                        graphics.drawTexture(residentTextures[residentIndex], drawX, drawY, size, size);
                        continue; // 画像を描画したら色の描画はスキップ
                    } else {
                        graphics.setDrawColor(128, 0, 128, 255); // 紫（フォールバック）
                    }
                }
                break;
            case NPCType::GUARD:
                // 衛兵画像を使用
                if (guardTexture) {
                    graphics.drawTexture(guardTexture, drawX, drawY, size, size);
                    continue; // 画像を描画したら色の描画はスキップ
                } else {
                    graphics.setDrawColor(255, 0, 0, 255); // 赤（フォールバック）
                }
                break;
        }
        
        // フォールバック用の色付き四角（画像がない場合）
        int fallbackX = npc.x * TILE_SIZE + 4;
        int fallbackY = npc.y * TILE_SIZE + 4;
        int fallbackSize = TILE_SIZE - 8;
        graphics.drawRect(fallbackX, fallbackY, fallbackSize, fallbackSize, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(fallbackX, fallbackY, fallbackSize, fallbackSize, false);
    }
}

void TownState::drawPlayer(Graphics& graphics) {
    // プレイヤーを描画（画像または青色の四角）
    GameState::drawPlayerWithTexture(graphics, playerTexture, playerX, playerY, TILE_SIZE);
}

bool TownState::isValidPosition(int x, int y) const {
    // 基本的な境界チェック
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return false;
    }
    
    // 建物との衝突チェック
    if (isCollidingWithBuilding(x, y)) {
        return false;
    }
    
    // 住人との衝突チェック
    if (isCollidingWithNPC(x, y)) {
        return false;
    }
    
    return true;
}

bool TownState::isCollidingWithBuilding(int x, int y) const {
    for (const auto& building : buildings) {
        int buildingX = building.first;
        int buildingY = building.second;
        
        // プレイヤー（1x1）と建物（2x2）の衝突チェック
        if (GameUtils::isColliding(x, y, 1, 1, buildingX, buildingY, BUILDING_SIZE, BUILDING_SIZE)) {
            return true;
        }
    }
    
    for (const auto& home : residentHomes) {
        int homeX = home.first;
        int homeY = home.second;
        
        // プレイヤー（1x1）と住人の家（2x2）の衝突チェック
        if (GameUtils::isColliding(x, y, 1, 1, homeX, homeY, BUILDING_SIZE, BUILDING_SIZE)) {
            return true;
        }
    }
    
    return false;
}

bool TownState::isCollidingWithNPC(int x, int y) const {
    for (const auto& npc : npcs) {
        // プレイヤー（1x1）とNPC（1x1）の衝突チェック
        if (GameUtils::isColliding(x, y, 1, 1, npc.x, npc.y, 1, 1)) {
            return true;
        }
    }
    return false;
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
    GameState::showMessage(message, messageBoard, isShowingMessage);
}

void TownState::clearMessage() {
    GameState::clearMessage(messageBoard, isShowingMessage);
}

// 自室への入り口関連
void TownState::checkRoomEntrance() {
    if (GameUtils::isNearPositionEuclidean(playerX, playerY, roomEntranceX, roomEntranceY, 1.5f)) {
        enterRoom();
    }
}

void TownState::enterRoom() {
    showMessage("自室に戻ります...");
    if (stateManager) {
        stateManager->changeState(std::make_unique<RoomState>(player));
    }
}

// フィールドへの入り口関連
void TownState::checkFieldEntrance() {
    // プレイヤーが出口の上にいる場合のみ遷移
    if (playerX == 14 && playerY == 15) {
        enterField();
    }
}

void TownState::enterField() {
    showMessage("フィールドに出ます...");
    
    if (stateManager) {
        stateManager->changeState(std::make_unique<FieldState>(player));
    }
}

void TownState::startNightTimer() {
    // ゲーム説明を表示
    setupGameExplanation();
    showGameExplanation = true;
    explanationStep = 0;
    
    // タイマーは説明が終わってから開始
    // nightTimerActive = true;
    // nightTimer = NIGHT_TIMER_DURATION;
    // 静的変数も更新
    // s_nightTimerActive = true;
    // s_nightTimer = NIGHT_TIMER_DURATION;
    
    // 夜の回数を増加
    
    // 目標レベルを動的に設定（25, 50, 75, 100...）
    s_levelGoalAchieved = false;
    
    // 最初の説明テキストを自動で表示
    if (!gameExplanationTexts.empty()) {
        showMessage(gameExplanationTexts[0]);
        isShowingMessage = true; // メッセージ表示状態を強制設定
    }
}

// 建物への入場関連
void TownState::checkBuildingEntrance() {
    // 建物の上にいるかチェック
    for (size_t i = 0; i < buildings.size(); ++i) {
        if (playerX == buildings[i].first && playerY == buildings[i].second) {
            if (buildingTypes[i] == "shop") {
                enterShop();
                return;
            } else if (buildingTypes[i] == "weapon_shop") {
                enterShop(); // 武器屋も道具屋として扱う
                return;
            } else if (buildingTypes[i] == "house") {
                enterRoom();
                return;
            } else if (buildingTypes[i] == "castle") {
                checkCastleEntrance();
                return;
            }
        }
    }
}

void TownState::enterShop() {
    currentLocation = TownLocation::SHOP;
    // setupUI()はrender()で呼ばれるため、ここではスキップ
    showMessage("道具屋に入りました。武器、防具、薬草などが売っています。スペースで買い物、ESCで外に出ます。");
}

void TownState::enterInn() {
    currentLocation = TownLocation::INN;
    // setupUI()はrender()で呼ばれるため、ここではスキップ
    showMessage("宿屋に入りました。暖炉の火が心地よく、疲れた体を休めることができます。スペースで休むとHPが全回復します。ESCで外に出ます。");
}



void TownState::enterChurch() {
    currentLocation = TownLocation::CHURCH;
    // setupUI()はrender()で呼ばれるため、ここではスキップ
    showMessage("教会に入りました。ステンドグラスから差し込む光が神聖な雰囲気を作り出しています。スペースで祈りを捧げることができます。ESCで外に出ます。");
}

void TownState::exitBuilding() {
    currentLocation = TownLocation::SQUARE;
    // setupUI()はrender()で呼ばれるため、ここではスキップ
    showMessage("建物を出ました。");
} 

void TownState::checkCastleEntrance() {
    // 城の上にいるかチェック
    if (playerX == castleX && playerY == castleY) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<CastleState>(player));
        }
    }
} 

void TownState::setupGameExplanation() {
    gameExplanationTexts.clear();    
    gameExplanationTexts.push_back("これからゲーム説明を始めます。よく聞いてくださいね。");
    gameExplanationTexts.push_back("まず時間経過により、夜時間が訪れます。");
    gameExplanationTexts.push_back("夜時間では住人を倒すことができ、1夜につき最大3人まで倒せます。");
    gameExplanationTexts.push_back("衛兵が住人の家を徘徊しているので、衛兵がいない時に倒しましょう。");
    gameExplanationTexts.push_back("住人を全員倒すことで街を滅ぼすことができます。どんどん倒しましょう。");
    gameExplanationTexts.push_back("しかし住人を倒すと勇者のメンタルが下がってしまいます。");
    gameExplanationTexts.push_back("メンタルが下がると勇者が住人を倒すのをためらって、失敗してしまいます。");
    gameExplanationTexts.push_back("（倒すのになれてしまうと逆にメンタルが上がるかもしれませんね。）");
    gameExplanationTexts.push_back("住人を倒せば魔王からの信頼度が上がり王様からの信頼度が下がります。");
    gameExplanationTexts.push_back("住人を倒さなければ魔王からの信頼度が下がりますがメンタルが回復します。");
    gameExplanationTexts.push_back("もしどちらかの信頼度が0になってしまったらその時点で処刑されゲームオーバーです。");
    gameExplanationTexts.push_back("処刑されないように、昼はモンスターを倒して王様からの信頼度を上げ、");
    gameExplanationTexts.push_back("夜は住人を倒して魔王からの信頼度を上げるようにしましょう。");
    gameExplanationTexts.push_back("また、夜になる前に目標のレベルに達しないと、");
    gameExplanationTexts.push_back("街に帰るまでにモンスターにやられてしまうのでゲームオーバーになります。");
    gameExplanationTexts.push_back("以上で説明は終わりです。早速下にあるゲートからフィールドに行ってみましょう。");
}

void TownState::checkTrustLevels() {
    // 信頼度が0になった場合のゲームオーバー
    if (player->getDemonTrust() <= 0) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "魔王からの信頼度が0になりました。処刑されました。"));
        }
    } else if (player->getKingTrust() <= 0) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "王様からの信頼度が0になりました。処刑されました。"));
        }
    }
} 