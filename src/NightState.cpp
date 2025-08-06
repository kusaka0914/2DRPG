#include "NightState.h"
#include "TownState.h"
#include "GameOverState.h"
#include "Graphics.h"
#include "InputManager.h"
#include "CommonUI.h"
#include <iostream>
#include <random>
#include <algorithm>

// 静的変数の初期化
int NightState::totalResidentsKilled = 0;

NightState::NightState(std::shared_ptr<Player> player)
    : player(player), playerX(TownLayout::PLAYER_START_X), playerY(TownLayout::PLAYER_START_Y), 
      isStealthMode(true), stealthLevel(1), residentsKilled(0),
      moveTimer(0), playerTexture(nullptr), guardTexture(nullptr),
      shopTexture(nullptr), weaponShopTexture(nullptr), houseTexture(nullptr), castleTexture(nullptr),
      stoneTileTexture(nullptr), residentHomeTexture(nullptr), toriiTexture(nullptr),
      messageLabel(nullptr), isShowingMessage(false), castleX(TownLayout::CASTLE_X), castleY(TownLayout::CASTLE_Y),
      guardMoveTimer(0), guardTargetHomeIndices(), guardStayTimers(), guardsInitialized(false) {
    
    // 住民画像配列を明示的にnullptrで初期化
    for (int i = 0; i < 6; ++i) {
        residentTextures[i] = nullptr;
    }
    
    // 住民配置データを初期化
    residents = {
        {3, 7},   // 町の住人1
        {7, 6},   // 町の住人2
        {11, 8},  // 町の住人3
        {5, 10},  // 町の住人4
        {19, 8},  // 町の住人5
        {21, 13}  // 町の住人6
    };
    
    // 建物配置データを初期化
    buildings = {
        {8, 10},   // 道具屋（2x2）
        {18, 10},  // 武器屋（2x2）
        {24, 10},  // 自室（2x2）
        {13, 2}    // 城（画面中央上部、2x2）
    };
    
    buildingTypes = {
        "shop", "weapon_shop", "house", "castle"
    };
    
    residentHomes = {
        {1, 6}, {5, 5}, {9, 7}, {3, 9}, {17, 7}, {21, 5}, {25, 6}, {4, 12}, {22, 12}
    };
    
    // 衛兵配置データを初期化（residentHomesの初期化後に実行）
    guards = {
        {12, 2},  // 衛兵1
        {12, 3},  // 衛兵2
        {15, 2},   // 衛兵3
        {15, 3}   // 衛兵4
    };
    
    // 衛兵の初期位置を住人の家の前に設定
    if (residentHomes.size() >= 4) {
        guards[0] = {residentHomes[0].first - 1, residentHomes[0].second}; // 1番目の家の左
        guards[1] = {residentHomes[1].first, residentHomes[1].second - 1}; // 2番目の家の上
        guards[2] = {residentHomes[2].first + 1, residentHomes[2].second}; // 3番目の家の右
        guards[3] = {residentHomes[3].first, residentHomes[3].second + 1}; // 4番目の家の下
    }
    
    // 衛兵の移動方向を初期化（4人分）
    guardDirections = {
        {1, 1},   // 右下方向
        {-1, 1},  // 左下方向
        {1, -1},  // 右上方向
        {-1, -1}  // 左上方向
    };
    
    // 衛兵の移動管理用ベクターを適切に初期化
    guardTargetHomeIndices.resize(4, 0);
    guardStayTimers.resize(4, 0.0f);
    
    // UIの初期化
    setupUI();
}

NightState::~NightState() {
    // UIのクリーンアップ
    ui.clear();
    
    // テクスチャのクリーンアップ（SDL_TextureはGraphicsクラスで管理されるため、
    // ここではポインタをnullptrに設定するのみ）
    playerTexture = nullptr;
    guardTexture = nullptr;
    shopTexture = nullptr;
    weaponShopTexture = nullptr;
    houseTexture = nullptr;
    castleTexture = nullptr;
    stoneTileTexture = nullptr;
    residentHomeTexture = nullptr;
    toriiTexture = nullptr;
    
    for (int i = 0; i < 6; ++i) {
        residentTextures[i] = nullptr;
    }
    
    messageLabel = nullptr;
    
    // ベクターのクリーンアップ
    residents.clear();
    guards.clear();
    guardDirections.clear();
    buildings.clear();
    buildingTypes.clear();
    residentHomes.clear();
    guardTargetHomeIndices.clear();
    guardStayTimers.clear();
}

void NightState::enter() {
    try {
        std::cout << "夜間モードに入りました" << std::endl;
        
        // プレイヤーの状態を設定
        if (player) {
            player->setNightTime(true);
        }
        
        // UIの再初期化
        setupUI();
        
        // メッセージ表示
        showMessage("夜の街に潜入しました。住民を襲撃して街を壊滅させましょう。");
        
        std::cout << "NightState: 初期化が完了しました" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "NightState: 初期化エラー: " << e.what() << std::endl;
    }
}

void NightState::exit() {
    std::cout << "夜間モードを終了しました" << std::endl;
    player->setNightTime(false);
    
    // 街に戻る時にタイマーを再起動
    TownState::s_nightTimerActive = true;
    TownState::s_nightTimer = 300.0f; // 5分 = 300秒
}

void NightState::update(float deltaTime) {
    try {
        moveTimer -= deltaTime;
        ui.update(deltaTime);
        
        // 衛兵の更新
        updateGuards(deltaTime);
    } catch (const std::exception& e) {
        std::cout << "NightState update error: " << e.what() << std::endl;
    }
}

void NightState::render(Graphics& graphics) {
    try {
        // 初回のみテクスチャを読み込み
        if (!playerTexture) {
            loadTextures(graphics);
        }
        
        // 夜の街を描画（街と同じ配置）
        drawMap(graphics);
        drawBuildings(graphics);
        
        // 住民を描画
        for (size_t i = 0; i < residents.size(); ++i) {
            const auto& resident = residents[i];
            SDL_Texture* texture = (i < 6) ? residentTextures[i] : nullptr;
            
            if (texture) {
                graphics.drawTexture(texture, resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            } else {
                // テクスチャがない場合は赤い四角で表示
                graphics.setDrawColor(255, 0, 0, 255);
                graphics.drawRect(resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            }
        }
        
        // 見張りを描画
        for (auto& guard : guards) {
            if (guardTexture) {
                graphics.drawTexture(guardTexture, guard.first * TILE_SIZE, guard.second * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            } else {
                // テクスチャがない場合は青い四角で表示
                graphics.setDrawColor(0, 0, 255, 255);
                graphics.drawRect(guard.first * TILE_SIZE, guard.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            }
        }
        
        // プレイヤーを描画
        drawPlayer(graphics);
        
        // 共通UIを描画
        CommonUI::drawTrustLevels(graphics, player, true, false);
        
        // UI更新
        updateUI();
        
        // UI描画
        ui.render(graphics);
        
        graphics.present();
    } catch (const std::exception& e) {
        std::cout << "NightState render error: " << e.what() << std::endl;
    }
}

void NightState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
            // 3人倒した後のメッセージの場合は街に戻る
            if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
                if (stateManager) {
                    // 夜の回数を増加
                    TownState::s_nightCount++;
                    // タイマーを再起動して街に戻る
                    TownState::s_nightTimerActive = true;
                    TownState::s_nightTimer = 300.0f; // 5分 = 300秒
                    stateManager->changeState(std::make_unique<TownState>(player));
                }
            }
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    // ESCキーで街に戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            // 夜の回数を増加
            TownState::s_nightCount++;
            // タイマーを再起動して街に戻る
            TownState::s_nightTimerActive = true;
            TownState::s_nightTimer = 300.0f; // 5分 = 300秒
            stateManager->changeState(std::make_unique<TownState>(player));
        }
        return;
    }
    
    // スペースキーで住民を襲撃
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        checkResidentInteraction();
        return;
    }
    
    // プレイヤー移動
    handleMovement(input);
}

void NightState::setupUI() {
    try {
        ui.clear();
        
        // メッセージ表示用
        auto messageLabelPtr = std::make_unique<Label>(50, 450, "", "default");
        if (messageLabelPtr) {
            messageLabelPtr->setColor({255, 255, 255, 255});
            messageLabel = messageLabelPtr.get();
            ui.addElement(std::move(messageLabelPtr));
        }
        
        std::cout << "NightState: UIの初期化が完了しました" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "NightState: UI初期化エラー: " << e.what() << std::endl;
    }
}

void NightState::handleMovement(const InputManager& input) {
    // 共通の移動処理を使用
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void NightState::checkResidentInteraction() {
    // 近くの住民をチェック
    for (auto& resident : residents) {
        if (GameUtils::isNearPositionEuclidean(playerX, playerY, resident.first, resident.second, 1.5f)) {
            attackResident(resident.first, resident.second);
            return;
        }
    }
    
    showMessage("近くに住民がいません。");
}

void NightState::attackResident(int x, int y) {
    if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
        showMessage("今夜はもう十分です。これ以上は危険です。\n街に戻ります。");
        // メッセージをクリアしたら街に戻る
        return;
    }
    
    // 衛兵の近くで襲撃した場合のチェック
    if (isNearGuard(x, y)) {
        showMessage("衛兵に見つかりました！王様からの信頼度が0になりました。");
        player->setKingTrust(0); // 王様からの信頼度を0に設定
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "衛兵に見つかりました。王様からの信頼度が0になりました。"));
        }
        return;
    }
    
    // メンタルに応じた成功率を計算
    int mental = player->getMental();
    int successRate = 50; // 基本成功率50%
    
    // メンタルが低いほど成功率が下がる
    if (mental < 30) {
        successRate = 20;
    } else if (mental < 50) {
        successRate = 35;
    } else if (mental > 80) {
        successRate = 70; // メンタルが高いと成功率が上がる
    }
    
    // ランダム判定
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    if (dis(gen) <= successRate) {
        // 成功：住民を倒した
        residentsKilled++;
        
        // 信頼度の変更
        player->changeKingTrust(-10); // 王様からの信頼度を10減少
        player->changeDemonTrust(10); // 魔王からの信頼度を10上昇
        
        // メンタルの変更
        totalResidentsKilled++; // 合計倒した人数を増加
        if (totalResidentsKilled >= 6) { // 合計6人以上倒した場合
            player->changeMental(20); // メンタルを20上昇
        } else {
            player->changeMental(-20); // メンタルを20減少
        }
        
        // 住民を削除
        residents.erase(std::remove_if(residents.begin(), residents.end(),
            [x, y](const std::pair<int, int>& pos) {
                return pos.first == x && pos.second == y;
            }), residents.end());
        
        showMessage("住民を倒しました。\n王様からの信頼度-10\n魔王からの信頼度+10\nメンタル" + 
                   std::string(totalResidentsKilled >= 6 ? "+20" : "-20"));
        
        // 3人倒した場合の処理
        if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
            showMessage("今夜はもう十分です。これ以上は危険です。\n街に戻ります。");
        }
    } else {
        // 失敗
        player->changeDemonTrust(-10); // 魔王からの信頼度を10減少
        showMessage("住民を倒すのに失敗しました。\n魔王からの信頼度-10");
    }
}

void NightState::hideEvidence() {
    player->performEvilAction();
    showMessage("証拠を隠滅しました。");
}

void NightState::showMessage(const std::string& message) {
    GameState::showMessage(message, messageLabel, isShowingMessage);
}

void NightState::clearMessage() {
    GameState::clearMessage(messageLabel, isShowingMessage);
}

void NightState::drawNightTown(Graphics& graphics) {
    // 夜の街を描画（暗い背景）
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 30; x++) {
            int drawX = x * TILE_SIZE;
            int drawY = y * TILE_SIZE;
            
            // 暗い街の背景
            graphics.setDrawColor(30, 30, 50, 255);
            graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // 住民を描画（赤い点）
    graphics.setDrawColor(255, 100, 100, 255);
    for (auto& resident : residents) {
        int drawX = resident.first * TILE_SIZE + 8;
        int drawY = resident.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
    
    // 見張りを描画（黄色い点）
    graphics.setDrawColor(255, 255, 100, 255);
    for (auto& guard : guards) {
        int drawX = guard.first * TILE_SIZE + 8;
        int drawY = guard.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
}

void NightState::drawPlayer(Graphics& graphics) {
    if (playerTexture) {
        graphics.drawTexture(playerTexture, playerX * TILE_SIZE, playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        // テクスチャがない場合は緑色の四角で表示
        graphics.setDrawColor(0, 255, 0, 255);
        graphics.drawRect(playerX * TILE_SIZE, playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

bool NightState::isValidPosition(int x, int y) const {
    // 基本的な境界チェック
    if (x < 0 || x >= 28 || y < 0 || y >= 16) {
        return false;
    }
    
    // 建物との衝突チェック
    if (isCollidingWithBuilding(x, y)) {
        return false;
    }
    
    return true;
}

bool NightState::isCollidingWithBuilding(int x, int y) const {
    for (const auto& building : buildings) {
        int buildingX = building.first;
        int buildingY = building.second;
        
        // プレイヤー（1x1）と建物（2x2）の衝突チェック
        if (GameUtils::isColliding(x, y, 1, 1, buildingX, buildingY, 2, 2)) {
            return true;
        }
    }
    
    // 住人の家との衝突チェック
    for (const auto& home : residentHomes) {
        int homeX = home.first;
        int homeY = home.second;
        
        // プレイヤー（1x1）と住人の家（2x2）の衝突チェック
        if (GameUtils::isColliding(x, y, 1, 1, homeX, homeY, 2, 2)) {
            return true;
        }
    }
    
    return false;
} 

void NightState::loadTextures(Graphics& graphics) {
    try {
        if (!playerTexture) {
            playerTexture = graphics.loadTexture("assets/characters/player.png", "player");
        }
        
        // 住民画像を個別に読み込み
        for (int i = 0; i < 6; ++i) {
            if (!residentTextures[i]) {
                std::string textureName = "resident_" + std::to_string(i + 1);
                residentTextures[i] = graphics.getTexture(textureName);
                if (!residentTextures[i]) {
                    std::cout << "住民テクスチャ " << textureName << " の読み込みに失敗しました" << std::endl;
                }
            }
        }
        
        if (!guardTexture) {
            guardTexture = graphics.loadTexture("assets/characters/guard.png", "guard");
        }
        
        // 建物画像を読み込み
        shopTexture = graphics.getTexture("shop");
        weaponShopTexture = graphics.getTexture("weapon_shop");
        houseTexture = graphics.getTexture("house");
        castleTexture = graphics.getTexture("castle");
        stoneTileTexture = graphics.getTexture("night_stone_tile");
        residentHomeTexture = graphics.getTexture("resident_home");
        toriiTexture = graphics.getTexture("torii");
        
        std::cout << "NightState: テクスチャの読み込みが完了しました" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "NightState: テクスチャ読み込みエラー: " << e.what() << std::endl;
    }
}

void NightState::updateGuards(float deltaTime) {
    try {
        // 初回のみ各衛兵の目標家をランダムに設定
        if (!guardsInitialized) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, residentHomes.size() - 1);
            
            for (size_t i = 0; i < guards.size(); ++i) {
                guardTargetHomeIndices[i] = dis(gen);
            }
            guardsInitialized = true;
            std::cout << "衛兵の初期化が完了しました" << std::endl;
        }
        
        guardMoveTimer += deltaTime;
        
        // 衛兵の移動間隔（1秒ごと）
        if (guardMoveTimer >= 1.0f) {
            guardMoveTimer = 0;
            
            for (size_t i = 0; i < guards.size(); ++i) {
                // 現在の衛兵の位置
                int currentX = guards[i].first;
                int currentY = guards[i].second;
                
                // 目標の住人の家の位置
                int targetHomeIndex = guardTargetHomeIndices[i];
                if (targetHomeIndex >= 0 && targetHomeIndex < static_cast<int>(residentHomes.size())) {
                    int targetX = residentHomes[targetHomeIndex].first;
                    int targetY = residentHomes[targetHomeIndex].second;
                    
                    // 目標の家に到着しているかチェック
                    int distanceToHome = std::max(abs(currentX - targetX), abs(currentY - targetY));
                    
                    if (distanceToHome <= 1) {
                        // 家に到着した場合、滞在タイマーを開始
                        guardStayTimers[i] += deltaTime;
                        
                        // 3秒間滞在したら次の家をランダムに選択
                        if (guardStayTimers[i] >= 3.0f) {
                            guardStayTimers[i] = 0.0f;
                            
                            // ランダムに次の家を選択（現在の家以外）
                            std::random_device rd;
                            std::mt19937 gen(rd());
                            std::uniform_int_distribution<> dis(0, residentHomes.size() - 1);
                            
                            int newTargetIndex;
                            do {
                                newTargetIndex = dis(gen);
                            } while (newTargetIndex == targetHomeIndex);
                            
                            guardTargetHomeIndices[i] = newTargetIndex;
                            
                            std::cout << "衛兵" << (i+1) << "が新しい家(" << newTargetIndex << ")に移動します" << std::endl;
                        }
                    } else {
                        // 家の前の位置を計算
                        int guardX, guardY;
                        
                        // 家の位置に応じて衛兵の配置を決定（家の前1マス）
                        if (targetX > 0) {
                            guardX = targetX - 1; // 家の左側
                        } else {
                            guardX = targetX + 1; // 家の右側
                        }
                        
                        if (targetY > 0) {
                            guardY = targetY - 1; // 家の上側
                        } else {
                            guardY = targetY + 1; // 家の下側
                        }
                        
                        // 境界チェック
                        if (guardX < 0 || guardX >= 28 || guardY < 0 || guardY >= 16) {
                            guardX = targetX;
                            guardY = targetY;
                        }
                        
                        // 建物との衝突チェック
                        bool collision = false;
                        for (const auto& building : buildings) {
                            if (GameUtils::isColliding(guardX, guardY, 1, 1, building.first, building.second, 2, 2)) {
                                collision = true;
                                break;
                            }
                        }
                        
                        // 他の住人の家との衝突チェック
                        if (!collision) {
                            for (size_t j = 0; j < residentHomes.size(); ++j) {
                                if (j != static_cast<size_t>(targetHomeIndex)) {
                                    if (GameUtils::isColliding(guardX, guardY, 1, 1, residentHomes[j].first, residentHomes[j].second, 2, 2)) {
                                        collision = true;
                                        break;
                                    }
                                }
                            }
                        }
                        
                        // 衝突がある場合は家の位置に配置
                        if (collision) {
                            guardX = targetX;
                            guardY = targetY;
                        }
                        
                        // 位置を更新
                        guards[i].first = guardX;
                        guards[i].second = guardY;
                        
                        std::cout << "衛兵" << (i+1) << "が家(" << targetHomeIndex << ")の前(" << guardX << "," << guardY << ")に移動しました" << std::endl;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "NightState updateGuards error: " << e.what() << std::endl;
    }
}

bool NightState::isNearGuard(int x, int y) const {
    // 衛兵を中心とした正方形の視界（最大2マス先まで）
    for (const auto& guard : guards) {
        int dx = abs(x - guard.first);
        int dy = abs(y - guard.second);
        
        // 正方形の視界（マンハッタン距離で2マス以内）
        if (dx <= 2 && dy <= 2) {
            return true;
        }
    }
    return false;
}

void NightState::updateUI() {
    // 信頼度チェック
    checkTrustLevels();
}

void NightState::checkTrustLevels() {
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

void NightState::drawMap(Graphics& graphics) {
    // 石タイルの画像描画（夜の街用）
    if (stoneTileTexture) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 28; x++) {
                int drawX = x * TILE_SIZE;
                int drawY = y * TILE_SIZE;
                
                // 石タイル画像を描画
                graphics.drawTexture(stoneTileTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        // フォールバック：石畳のタイル描画（グレー）
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 28; x++) {
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

void NightState::drawBuildings(Graphics& graphics) {
    for (size_t i = 0; i < buildings.size(); ++i) {
        int x = buildings[i].first;
        int y = buildings[i].second;
        std::string type = buildingTypes[i];
        
        // 建物を2タイルサイズで描画
        if (type == "shop" && shopTexture) {
            graphics.drawTexture(shopTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else if (type == "weapon_shop" && weaponShopTexture) {
            graphics.drawTexture(weaponShopTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else if (type == "house" && houseTexture) {
            graphics.drawTexture(houseTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else if (type == "castle" && castleTexture) {
            graphics.drawTexture(castleTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
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
                            TILE_SIZE * 2, TILE_SIZE * 2, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2);
        }
    }
    
    // 住人の家を描画
    for (const auto& home : residentHomes) {
        int x = home.first;
        int y = home.second;
        
        if (residentHomeTexture) {
            // 住人の家画像を描画（2タイルサイズ）
            graphics.drawTexture(residentHomeTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else {
            // フォールバック：茶色の四角
            graphics.setDrawColor(139, 69, 19, 255); // 茶色
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2);
        }
    }
}

void NightState::drawGate(Graphics& graphics) {
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