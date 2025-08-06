#include "FieldState.h"
#include "MainMenuState.h"
#include "BattleState.h"
#include "TownState.h"
#include "RoomState.h"
#include "CastleState.h"
#include "DemonCastleState.h"
#include "MapTerrain.h"
#include "NightState.h"
#include <iostream>
#include <random>

// 静的変数（ファイル内で共有）
static int s_staticPlayerX = 25;  // 街の入り口の近く：25
static int s_staticPlayerY = 8;   // 16の中央：8
static bool s_positionInitialized = false;

FieldState::FieldState(std::shared_ptr<Player> player)
    : player(player), storyBox(nullptr), hasMoved(false),
      moveTimer(0), nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      shouldRelocateMonster(false), lastBattleX(0), lastBattleY(0) {
    
    // 固定マップの作成（28x16）
    static std::vector<std::vector<MapTile>> staticTerrainMap;
    static bool mapGenerated = false;
    
    if (!mapGenerated) {
        // 28x16の固定マップを作成
        staticTerrainMap.resize(16, std::vector<MapTile>(28));
        
        // マップの初期化（すべて草原）
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 28; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::GRASS);
            }
        }
        
        // 川を配置（蛇行する川）
        for (int y = 3; y < 13; y++) {
            staticTerrainMap[y][12] = MapTile(TerrainType::WATER);
        }
        for (int y = 4; y < 12; y++) {
            staticTerrainMap[y][18] = MapTile(TerrainType::WATER);
        }
        for (int y = 5; y < 11; y++) {
            staticTerrainMap[y][24] = MapTile(TerrainType::WATER);
        }
        
        // 橋を配置（川を横切る）
        staticTerrainMap[8][12] = MapTile(TerrainType::BRIDGE);
        staticTerrainMap[8][18] = MapTile(TerrainType::BRIDGE);
        staticTerrainMap[8][24] = MapTile(TerrainType::BRIDGE);
        
        // 森林を配置（分散した配置）
        // 左上の森
        for (int y = 1; y < 6; y++) {
            for (int x = 1; x < 6; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::FOREST);
            }
        }
        // 右上の森
        for (int y = 1; y < 5; y++) {
            for (int x = 22; x < 27; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::FOREST);
            }
        }
        // 左下の森
        for (int y = 11; y < 15; y++) {
            for (int x = 2; x < 7; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::FOREST);
            }
        }
        
        // 岩を配置（散らばった配置）
        staticTerrainMap[2][8] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[5][15] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[7][22] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[10][10] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[12][19] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[14][25] = MapTile(TerrainType::ROCK, true, 1);
        
        // 建物は削除（街から直接アクセス）
        
        // 街の入り口を追加（右端中央付近）
        staticTerrainMap[8][26] = MapTile(TerrainType::TOWN_ENTRANCE);
        
        std::cout << "固定フィールドマップを作成しました！" << std::endl;
        std::cout << "マップサイズ: " << staticTerrainMap.size() << "x" << (staticTerrainMap[0].size()) << std::endl;
        mapGenerated = true;
    }
    
    if (!s_positionInitialized) {
        std::cout << "フィールドの初期位置を設定しました！" << std::endl;
        s_positionInitialized = true;
    }
    
    terrainMap = staticTerrainMap;
    playerX = s_staticPlayerX;
    playerY = s_staticPlayerY;
    
    // モンスター出現場所を生成
    generateMonsterSpawnPoints();
    
    std::cout << "terrainMapサイズ: " << terrainMap.size() << "x" << (terrainMap.empty() ? 0 : terrainMap[0].size()) << std::endl;
    std::cout << "プレイヤー初期位置: (" << playerX << ", " << playerY << ")" << std::endl;
}

void FieldState::enter() {
    setupUI();
    loadFieldImages();
    
    // 戦闘終了時の処理
    if (shouldRelocateMonster) {
        relocateMonsterSpawnPoint(lastBattleX, lastBattleY);
        shouldRelocateMonster = false;
    }
    
    // オープニングストーリーは王様の城で表示されるため、フィールドでは表示しない
    // static bool openingShown = false;
    // if (!openingShown) {
    //     showOpeningStory();
    //     openingShown = true;
    // }
}

void FieldState::exit() {
    ui.clear();
}

void FieldState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // 夜のタイマーを更新
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        // 静的変数に状態を保存
        TownState::s_nightTimerActive = true;
        TownState::s_nightTimer = nightTimer;
        
        // 目標レベルチェック
        if (player->getLevel() >= TownState::s_targetLevel && !TownState::s_levelGoalAchieved) {
            TownState::s_levelGoalAchieved = true;
            // メッセージ表示はTownStateで行うため、ここではコンソール出力のみ
            std::cout << "目標レベル" << TownState::s_targetLevel << "を達成しました！" << std::endl;
        }
        
        if (nightTimer <= 0.0f) {
            // 5分経過したら夜の街に移動
            nightTimerActive = false;
            TownState::s_nightTimerActive = false;
            TownState::s_nightTimer = 0.0f;
            
            // 目標達成していない場合はゲームオーバー
            if (!TownState::s_levelGoalAchieved) {
                // ゲームオーバー処理（メイン画面に戻る）
                if (stateManager) {
                    auto newPlayer = std::make_shared<Player>("勇者");
                    stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
                }
            } else {
                if (stateManager) {
                    stateManager->changeState(std::make_unique<NightState>(player));
                }
            }
        }
    } else {
        // タイマーが非アクティブな場合、静的変数もクリア
        TownState::s_nightTimerActive = false;
        TownState::s_nightTimer = 0.0f;
    }
    
    // プレイヤーの移動処理
    
    // UI情報を更新（値が変更された場合のみ）
    if (ui.getElements().size() >= 5) {
        // プレイヤー情報（変更検出）
        static std::string lastPlayerInfo = "";
        std::string playerInfo = player->getName() + " Lv:" + std::to_string(player->getLevel()) + 
                                " HP:" + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp()) +
                                " Gold:" + std::to_string(player->getGold());
        
        if (playerInfo != lastPlayerInfo) {
            static_cast<Label*>(ui.getElements()[0].get())->setText(playerInfo);
            lastPlayerInfo = playerInfo;
        }
        
        // 現在レベル表示（変更検出）
        static std::string lastCurrentLevel = "";
        std::string currentLevelInfo = "現在レベル: " + std::to_string(player->getLevel());
        
        if (currentLevelInfo != lastCurrentLevel) {
            static_cast<Label*>(ui.getElements()[1].get())->setText(currentLevelInfo);
            lastCurrentLevel = currentLevelInfo;
        }
        
        // 目標レベル表示（変更検出）
        static std::string lastTargetLevel = "";
        std::string targetLevelInfo = "目標レベル: " + std::to_string(TownState::s_targetLevel);
        
        if (targetLevelInfo != lastTargetLevel) {
            static_cast<Label*>(ui.getElements()[2].get())->setText(targetLevelInfo);
            lastTargetLevel = targetLevelInfo;
        }
        
        // 現在の地形情報（変更検出）
        static std::string lastTerrainInfo = "";
        TerrainType currentTerrain = getCurrentTerrain();
        std::string terrainName;
        switch (currentTerrain) {
            case TerrainType::GRASS: terrainName = "草原"; break;
            case TerrainType::FOREST: terrainName = "森"; break;
            case TerrainType::MOUNTAIN: terrainName = "山"; break;
            case TerrainType::WATER: terrainName = "川"; break;
            case TerrainType::ROAD: terrainName = "道路"; break;
            case TerrainType::BRIDGE: terrainName = "橋"; break;
            case TerrainType::FLOWER_FIELD: terrainName = "花畑"; break;
            case TerrainType::TOWN_ENTRANCE: terrainName = "街の入り口"; break;
            default: terrainName = "不明"; break;
        }
        std::string terrainInfo = "現在地: " + terrainName;
        
        if (terrainInfo != lastTerrainInfo) {
            static_cast<Label*>(ui.getElements()[3].get())->setText(terrainInfo);
            lastTerrainInfo = terrainInfo;
        }
    }
}

void FieldState::render(Graphics& graphics) {
    // 背景色を設定（草原風）
    graphics.setDrawColor(34, 139, 34, 255);
    graphics.clear();
    
    drawMap(graphics);
    drawPlayer(graphics);
    
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
        
        // 目標レベル情報を表示
        int currentLevel = player->getLevel();
        int remainingLevels = TownState::s_targetLevel - currentLevel;
        
        // 目標レベル背景
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(10, 60, 250, 80, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(10, 60, 250, 80, false);
        
        if (TownState::s_targetLevel > 0) { // 目標レベルが設定されている場合のみ表示
            if (TownState::s_levelGoalAchieved) {
                // 目標達成済み
                std::string goalText = "目標レベル" + std::to_string(TownState::s_targetLevel) + "達成！";
                graphics.drawText(goalText, 20, 70, "default", {0, 255, 0, 255}); // 緑色
                graphics.drawText("夜の街に進出可能", 20, 90, "default", {0, 255, 0, 255});
            } else {
                // 目標未達成
                std::string currentLevel = "現在レベル: " + std::to_string(player->getLevel());
                graphics.drawText(currentLevel, 20, 70, "default", {255, 255, 255, 255});
                std::string goalText = "目標レベル: " + std::to_string(TownState::s_targetLevel);
                graphics.drawText(goalText, 20, 90, "default", {255, 255, 255, 255});
                std::string remainingText = "残りレベル: " + std::to_string(remainingLevels);
                graphics.drawText(remainingText, 20, 110, "default", {255, 255, 0, 255}); // 黄色
            }
        }
    }
    
    // 新しいパラメータを表示（タイマーがアクティブな場合のみ）
    if (nightTimerActive) {
        // パラメータ背景
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(10, 150, 300, 80, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(10, 150, 300, 80, false);
        
        // パラメータテキスト
        std::string mentalText = "メンタル: " + std::to_string(player->getMental());
        std::string demonTrustText = "魔王からの信頼: " + std::to_string(player->getDemonTrust());
        std::string kingTrustText = "王様からの信頼: " + std::to_string(player->getKingTrust());
        
        graphics.drawText(mentalText, 20, 160, "default", {255, 255, 255, 255});
        graphics.drawText(demonTrustText, 20, 180, "default", {255, 100, 100, 255}); // 赤色
        graphics.drawText(kingTrustText, 20, 200, "default", {100, 100, 255, 255}); // 青色
    }
    
    graphics.present();
}

void FieldState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // ESCキーでメインメニューに戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<MainMenuState>(player));
        }
        return;
    }
    
    // スペースキーで街に入る
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        checkTownEntrance();
        return;
    }
    
    // プレイヤー移動
    handleMovement(input);
}

void FieldState::setupUI() {
    ui.clear();
    
    // プレイヤー情報表示
    auto playerInfoLabel = std::make_unique<Label>(10, 10, "", "default");
    playerInfoLabel->setColor({255, 255, 255, 255});
    ui.addElement(std::move(playerInfoLabel));
    
    // 現在レベル表示（上に追加）
    auto currentLevelLabel = std::make_unique<Label>(10, 30, "", "default");
    currentLevelLabel->setColor({255, 255, 0, 255}); // 黄色
    ui.addElement(std::move(currentLevelLabel));
    
    // 目標レベル表示（上に追加）
    auto targetLevelLabel = std::make_unique<Label>(10, 50, "", "default");
    targetLevelLabel->setColor({255, 200, 0, 255}); // オレンジ
    ui.addElement(std::move(targetLevelLabel));
    
    // 現在の地形情報表示（元の位置）
    auto terrainLabel = std::make_unique<Label>(10, 70, "", "default");
    terrainLabel->setColor({200, 255, 200, 255});
    ui.addElement(std::move(terrainLabel));
    
    // ストーリーメッセージ用のLabel（StoryMessageBoxの代わり）
    auto storyLabel = std::make_unique<Label>(50, 380, "", "default");
    storyLabel->setColor({255, 255, 255, 255});
    storyLabel->setText("フィールドを探索中... 敵に遭遇する可能性があります。");
    ui.addElement(std::move(storyLabel));
    
    // StoryMessageBoxは無効化
    storyBox = nullptr;
}

void FieldState::handleMovement(const InputManager& input) {
    if (moveTimer > 0) return;
    
    int newX = playerX;
    int newY = playerY;
    
    // キーボード入力
    if (input.isKeyPressed(InputKey::UP) || input.isKeyPressed(InputKey::W)) {
        newY--;
    } else if (input.isKeyPressed(InputKey::DOWN) || input.isKeyPressed(InputKey::S)) {
        newY++;
    } else if (input.isKeyPressed(InputKey::LEFT) || input.isKeyPressed(InputKey::A)) {
        newX--;
    } else if (input.isKeyPressed(InputKey::RIGHT) || input.isKeyPressed(InputKey::D)) {
        newX++;
    }
    
    // ゲームパッドのアナログスティック入力
    const float DEADZONE = 0.3f;
    float stickX = input.getLeftStickX();
    float stickY = input.getLeftStickY();
    
    if (abs(stickX) > DEADZONE || abs(stickY) > DEADZONE) {
        if (abs(stickX) > abs(stickY)) {
            if (stickX < -DEADZONE) {
                newX--;
            } else if (stickX > DEADZONE) {
                newX++;
            }
        } else {
            if (stickY < -DEADZONE) {
                newY--;
            } else if (stickY > DEADZONE) {
                newY++;
            }
        }
    }
    
    if (isValidPosition(newX, newY)) {
        // 実際に移動したかチェック
        bool actuallyMoved = (newX != playerX || newY != playerY);
        
        if (actuallyMoved) {
            playerX = newX;
            playerY = newY;
            
            // 静的位置変数も更新（位置を保持するため）
            s_staticPlayerX = newX;
            s_staticPlayerY = newY;
            
            moveTimer = MOVE_DELAY;
            hasMoved = true; // 移動フラグを設定
            
            std::cout << "実際に移動: (" << playerX << ", " << playerY << ")" << std::endl;
            
            // 移動した時のみエンカウントチェック
            checkEncounter();
        } else {
            
        }
    }
}

void FieldState::checkEncounter() {
    // 移動していない場合は敵出現しない
    if (!hasMoved) {
        return;
    }
    
    // 現在の地形に基づいてエンカウント率を決定
    TerrainType currentTerrain = getCurrentTerrain();
    TerrainData terrainData = TerrainRenderer::getTerrainData(currentTerrain);
    
    // エンカウント率が0の地形では敵が出現しない
    if (terrainData.encounterRate <= 0) {
        return;
    }
    
    // 建物タイルとモンスター専用タイルのチェック
    if (playerY >= 0 && playerY < 16 && playerX >= 0 && playerX < 28) {
        if (!terrainMap.empty() && playerY < static_cast<int>(terrainMap.size()) && 
            !terrainMap[0].empty() && playerX < static_cast<int>(terrainMap[0].size())) {
            const MapTile& currentTile = terrainMap[playerY][playerX];
            if (currentTile.objectType == 2) { // モンスター専用タイル
                std::cout << "モンスター専用タイルで敵遭遇！" << std::endl;
                
                // この位置の敵の種類を取得
                EnemyType enemyType = EnemyType::SLIME; // デフォルト
                for (size_t i = 0; i < activeMonsterPoints.size(); i++) {
                    if (activeMonsterPoints[i].first == playerX && activeMonsterPoints[i].second == playerY) {
                        enemyType = activeMonsterTypes[i];
                        break;
                    }
                }
                
                Enemy enemy(enemyType);
                if (stateManager) {
                    // 戦闘開始時の座標を保存
                    lastBattleX = playerX;
                    lastBattleY = playerY;
                    shouldRelocateMonster = true;
                    
                    auto battleState = std::make_unique<BattleState>(player, std::make_unique<Enemy>(enemy));
                    stateManager->changeState(std::move(battleState));
                }
                return;
            }
        }
    }
    
    // 通常のエンカウントは無効化（モンスター出現場所でのみエンカウント）
    
    // 移動フラグをリセット
    hasMoved = false;
}

void FieldState::drawMap(Graphics& graphics) {
    // 地形ベースのタイル描画（固定マップサイズ28x16に対応）
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 28; x++) {
            drawTerrain(graphics, terrainMap[y][x], x, y);
        }
    }
}

void FieldState::drawTerrain(Graphics& graphics, const MapTile& tile, int x, int y) {
    int drawX = x * TILE_SIZE;
    int drawY = y * TILE_SIZE;
    
    // 地形に応じた画像を描画
    SDL_Texture* terrainTexture = nullptr;
    std::string textureName;
    
    switch (tile.terrain) {
        case TerrainType::GRASS:
            textureName = "grass";
            break;
        case TerrainType::FOREST:
            textureName = "forest";
            break;
        case TerrainType::WATER:
            textureName = "river";
            break;
        case TerrainType::BRIDGE:
            textureName = "bridge";
            break;
        case TerrainType::ROCK:
            textureName = "rock";
            break;
        case TerrainType::TOWN_ENTRANCE:
            textureName = "town_entrance";
            break;
        default:
            textureName = "grass"; // デフォルト
            break;
    }
    
    terrainTexture = graphics.getTexture(textureName);
    if (terrainTexture) {
        // 画像がある場合は画像を描画
        graphics.drawTexture(terrainTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
    } else {
        // 画像がない場合は色で描画（フォールバック）
        auto terrainColor = TerrainRenderer::getTerrainColor(tile.terrain);
        graphics.setDrawColor(terrainColor.r, terrainColor.g, terrainColor.b, terrainColor.a);
        graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
        
        // 地形の境界線
        if (tile.terrain == TerrainType::WATER) {
            graphics.setDrawColor(0, 50, 150, 255);  // 濃い青
        } else if (tile.terrain == TerrainType::FOREST) {
            graphics.setDrawColor(0, 50, 0, 255);    // 濃い緑
        } else if (tile.terrain == TerrainType::BRIDGE) {
            graphics.setDrawColor(139, 69, 19, 255); // 茶色
        } else if (tile.terrain == TerrainType::ROCK) {
            graphics.setDrawColor(100, 100, 100, 255); // グレー
        } else if (tile.terrain == TerrainType::TOWN_ENTRANCE) {
            graphics.setDrawColor(255, 215, 0, 255);  // 金色
        } else {
            graphics.setDrawColor(20, 100, 20, 255);  // デフォルト
        }
        graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, false);
    }
    
    // オブジェクトがある場合は描画
    if (tile.hasObject) {
        auto objectColor = TerrainRenderer::getObjectColor(tile.objectType);
        graphics.setDrawColor(objectColor.r, objectColor.g, objectColor.b, objectColor.a);
        
        int objSize = TILE_SIZE / 3;
        int objX = drawX + (TILE_SIZE - objSize) / 2;
        int objY = drawY + (TILE_SIZE - objSize) / 2;
        
        if (tile.objectType == 2) { // モンスター専用タイル
            // この位置の敵の種類を取得
            EnemyType enemyType = EnemyType::SLIME; // デフォルト
            for (size_t i = 0; i < activeMonsterPoints.size(); i++) {
                if (activeMonsterPoints[i].first == x && activeMonsterPoints[i].second == y) {
                    enemyType = activeMonsterTypes[i];
                    break;
                }
            }
            
            // 敵の種類に応じた画像を表示
            Enemy tempEnemy(enemyType);
            std::string enemyTextureName = "enemy_" + tempEnemy.getTypeName();
            SDL_Texture* enemyTexture = graphics.getTexture(enemyTextureName);
            
            if (enemyTexture) {
                // 敵画像を表示（少し小さくして表示）
                int enemySize = TILE_SIZE * 0.8;
                int enemyX = drawX + (TILE_SIZE - enemySize) / 2;
                int enemyY = drawY + (TILE_SIZE - enemySize) / 2;
                graphics.drawTexture(enemyTexture, enemyX, enemyY, enemySize, enemySize);
                
                // 敵のレベルを表示
                Enemy tempEnemy(enemyType);
                std::string levelText = "Lv" + std::to_string(tempEnemy.getLevel());
                graphics.drawText(levelText, drawX + 2, drawY + 2, "default", {255, 255, 255, 255});
            } else {
                // 画像がない場合は赤色で表示
                graphics.setDrawColor(255, 0, 0, 255);
                graphics.drawRect(objX, objY, objSize, objSize, true);
                
                // 敵のレベルを表示
                Enemy tempEnemy(enemyType);
                std::string levelText = "Lv" + std::to_string(tempEnemy.getLevel());
                graphics.drawText(levelText, drawX + 2, drawY + 2, "default", {255, 255, 255, 255});
            }

        } else { // 岩や木の場合は四角形
            graphics.drawRect(objX, objY, objSize, objSize, true);
        }
    }
}

void FieldState::drawPlayer(Graphics& graphics) {
    int drawX = playerX * TILE_SIZE + 4;
    int drawY = playerY * TILE_SIZE + 4;
    int size = TILE_SIZE - 8;
    
    // プレイヤー画像を描画（存在しない場合は四角形）
    SDL_Texture* playerTexture = graphics.getTexture("player_field");
    if (playerTexture) {
        graphics.drawTexture(playerTexture, drawX, drawY, size, size);
    } else {
        // フォールバック：青い四角で描画（境界線付き）
        graphics.setDrawColor(0, 0, 255, 255);
        graphics.drawRect(drawX, drawY, size, size, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(drawX, drawY, size, size, false);
    }
}

void FieldState::checkTownEntrance() {
    // 現在の地形が街の入り口かチェック
    TerrainType currentTerrain = getCurrentTerrain();
    if (currentTerrain == TerrainType::TOWN_ENTRANCE) {
        std::cout << "街に入ります..." << std::endl;
        if (stateManager) {
            stateManager->changeState(std::make_unique<TownState>(player));
        }
    } else {
        std::cout << "ここには街の入り口がありません。" << std::endl;
    }
}

bool FieldState::isValidPosition(int x, int y) const {
    // 新しいマップサイズ（28x16）に対応
    if (x < 0 || x >= 28 || y < 0 || y >= 16) {
        std::cout << "境界外アクセス: (" << x << ", " << y << ")" << std::endl;
        return false;
    }
    
    // terrainMapの境界チェックを追加（空配列チェックも含む）
    if (terrainMap.empty() || y >= static_cast<int>(terrainMap.size()) || 
        terrainMap[0].empty() || x >= static_cast<int>(terrainMap[0].size())) {
        std::cout << "terrainMap境界外アクセス: (" << x << ", " << y << ") terrainMapサイズ: " << terrainMap.size() << "x" << (terrainMap.empty() ? 0 : terrainMap[0].size()) << std::endl;
        return false;
    }
    
    // 地形データに基づいて移動可能性をチェック（正しい順序でアクセス）
    TerrainData terrainData = TerrainRenderer::getTerrainData(terrainMap[y][x].terrain);
    if (!terrainData.walkable) {
        return false;
    }
    
    // オブジェクトがある場合の移動制限（正しい順序でアクセス）
    if (terrainMap[y][x].hasObject && terrainMap[y][x].objectType == 1) { // 岩は通れない
        return false;
    }
    
    return true;
}

TerrainType FieldState::getCurrentTerrain() const {
    // 新しいマップサイズ（28x16）での境界チェック
    if (playerY >= 0 && playerY < 16 && playerX >= 0 && playerX < 28) {
        // terrainMapの境界チェックを追加（空配列チェックも含む）
        if (!terrainMap.empty() && playerY < static_cast<int>(terrainMap.size()) && 
            !terrainMap[0].empty() && playerX < static_cast<int>(terrainMap[0].size())) {
            return terrainMap[playerY][playerX].terrain; // 正しい順序でアクセス
        } else {
            std::cout << "getCurrentTerrain: terrainMap境界外アクセス: (" << playerX << ", " << playerY << ") terrainMapサイズ: " << terrainMap.size() << "x" << (terrainMap.empty() ? 0 : terrainMap[0].size()) << std::endl;
        }
    }
    return TerrainType::GRASS; // デフォルト
}

void FieldState::loadFieldImages() {
    // フィールド用の画像を読み込み
    // 注意: 実際の画像読み込みはSDL2Game::loadResourcesで行われる
    // ここでは画像名の定義のみ
}

void FieldState::generateMonsterSpawnPoints() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(1, 26); // 境界を避ける
    std::uniform_int_distribution<> disY(1, 14); // 境界を避ける
    
    monsterSpawnPoints.clear();
    activeMonsterPoints.clear();
    activeMonsterTypes.clear();
    
    // 5箇所のモンスター出現場所を生成
    for (int i = 0; i < 5; i++) {
        int x, y;
        bool validPosition;
        
        do {
            x = disX(gen);
            y = disY(gen);
            validPosition = isValidPosition(x, y) && 
                          terrainMap[y][x].terrain == TerrainType::GRASS &&
                          !terrainMap[y][x].hasObject;
        } while (!validPosition);
        
        // 敵の種類をランダムに決定
        EnemyType enemyType = Enemy::createRandomEnemy(player->getLevel()).getType();
        
        monsterSpawnPoints.push_back({x, y});
        activeMonsterPoints.push_back({x, y});
        activeMonsterTypes.push_back(enemyType);
        
        // 地形マップにモンスター出現場所をマーク
        terrainMap[y][x].hasObject = true;
        terrainMap[y][x].objectType = 2; // モンスター専用タイル
    }
    
    std::cout << "モンスター出現場所を5箇所生成しました。" << std::endl;
}

void FieldState::relocateMonsterSpawnPoint(int oldX, int oldY) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(1, 26);
    std::uniform_int_distribution<> disY(1, 14);
    
    // 古い場所をクリア
    terrainMap[oldY][oldX].hasObject = false;
    terrainMap[oldY][oldX].objectType = 0;
    
    // 新しい場所を探す
    int newX, newY;
    bool validPosition;
    
    do {
        newX = disX(gen);
        newY = disY(gen);
        validPosition = isValidPosition(newX, newY) && 
                      terrainMap[newY][newX].terrain == TerrainType::GRASS &&
                      !terrainMap[newY][newX].hasObject;
    } while (!validPosition);
    
    // 新しい場所をマーク
    terrainMap[newY][newX].hasObject = true;
    terrainMap[newY][newX].objectType = 2;
    
    // アクティブリストを更新
    for (size_t i = 0; i < activeMonsterPoints.size(); i++) {
        if (activeMonsterPoints[i].first == oldX && activeMonsterPoints[i].second == oldY) {
            activeMonsterPoints[i].first = newX;
            activeMonsterPoints[i].second = newY;
            // 敵の種類も新しいものに更新
            activeMonsterTypes[i] = Enemy::createRandomEnemy(player->getLevel()).getType();
            break;
        }
    }
    
    std::cout << "モンスター出現場所を (" << oldX << "," << oldY << ") から (" << newX << "," << newY << ") に移動しました。" << std::endl;
}

// ストーリーシステム
void FieldState::showOpeningStory() {
    if (storyBox && player) {
        std::vector<std::string> story = player->getOpeningStory();
        storyBox->setMessage(story);
        storyBox->show();
    }
}

void FieldState::showLevelUpStory(int newLevel) {
    // レベルアップメッセージ（Labelで表示）
    if (ui.getElements().size() >= 3) {
        static_cast<Label*>(ui.getElements()[2].get())->setText("レベルアップ！新しい力が目覚めた！");
    }
} 