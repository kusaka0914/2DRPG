#include "FieldState.h"
#include "MainMenuState.h"
#include "BattleState.h"
#include "TownState.h"
#include "RoomState.h"
#include "CastleState.h"
#include "DemonCastleState.h"
#include "../utils/MapTerrain.h"
#include "NightState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>
#include <random>

static int s_staticPlayerX = 25;  // 街の入り口の近く：25
static int s_staticPlayerY = 8;   // 16の中央：8
static bool s_positionInitialized = false;
static bool firstEnter= true;
static bool saved = TownState::saved;

FieldState::FieldState(std::shared_ptr<Player> player)
    : player(player), storyBox(nullptr), hasMoved(false),
      moveTimer(0), nightTimerActive(false), nightTimer(0.0f),
      shouldRelocateMonster(false), lastBattleX(0), lastBattleY(0) {
    
    static std::vector<std::vector<MapTile>> staticTerrainMap;
    static bool mapGenerated = false;

    if (!mapGenerated) {
        staticTerrainMap.resize(16, std::vector<MapTile>(28));
        
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 28; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::GRASS);
            }
        }
        
        for (int y = 3; y < 13; y++) {
            staticTerrainMap[y][12] = MapTile(TerrainType::WATER);
        }
        for (int y = 4; y < 12; y++) {
            staticTerrainMap[y][18] = MapTile(TerrainType::WATER);
        }
        for (int y = 5; y < 11; y++) {
            staticTerrainMap[y][24] = MapTile(TerrainType::WATER);
        }
        
        staticTerrainMap[8][12] = MapTile(TerrainType::BRIDGE);
        staticTerrainMap[8][18] = MapTile(TerrainType::BRIDGE);
        staticTerrainMap[8][24] = MapTile(TerrainType::BRIDGE);
        
        for (int y = 1; y < 6; y++) {
            for (int x = 1; x < 6; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::FOREST);
            }
        }
        for (int y = 1; y < 5; y++) {
            for (int x = 22; x < 27; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::FOREST);
            }
        }
        for (int y = 11; y < 15; y++) {
            for (int x = 2; x < 7; x++) {
                staticTerrainMap[y][x] = MapTile(TerrainType::FOREST);
            }
        }
        
        staticTerrainMap[2][8] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[5][15] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[7][22] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[10][10] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[12][19] = MapTile(TerrainType::ROCK, true, 1);
        staticTerrainMap[14][25] = MapTile(TerrainType::ROCK, true, 1);
        
        // 建物は削除（街から直接アクセス）
        
        staticTerrainMap[8][26] = MapTile(TerrainType::TOWN_ENTRANCE);
        mapGenerated = true;
    }
    
    if (!s_positionInitialized) {
        s_positionInitialized = true;
    }
    
    terrainMap = staticTerrainMap;
    playerX = s_staticPlayerX;
    playerY = s_staticPlayerY;
    
    generateMonsterSpawnPoints();
}

void FieldState::enter() {
    setupUI();
    loadFieldImages();

    nightTimerActive = TownState::s_nightTimerActive;
    nightTimer = TownState::s_nightTimer;
    
    // デバッグ出力
    std::cout << "FieldState: 静的変数から読み込み - Timer: " << nightTimer 
              << ", Active: " << (nightTimerActive ? "true" : "false") << std::endl;

    // タイマー情報も含めてautosave
    player->saveGame("autosave.dat", nightTimer, nightTimerActive);
    
    if (shouldRelocateMonster) {
        relocateMonsterSpawnPoint(lastBattleX, lastBattleY);
        shouldRelocateMonster = false;
    }
    
    if (player->getLevel() >= TownState::s_targetLevel && !TownState::s_levelGoalAchieved) {
        TownState::s_levelGoalAchieved = true;
    }
    
    if (TownState::s_levelGoalAchieved && nightTimerActive) {
        nightTimer = 0.0f;
        TownState::s_nightTimer = 0.0f;
    }
    
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
    
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        TownState::s_nightTimerActive = true;
        TownState::s_nightTimer = nightTimer;
        
        // 目標レベルチェック
        if (player->getLevel() >= TownState::s_targetLevel && !TownState::s_levelGoalAchieved) {
            TownState::s_levelGoalAchieved = true;
        }
        
        if (nightTimer <= 0.0f) {
            nightTimerActive = false;
            TownState::s_nightTimerActive = false;
            TownState::s_nightTimer = 0.0f;
            
            // 目標達成していない場合はゲームオーバー
            if (!TownState::s_levelGoalAchieved) {
                if (stateManager) {
                    auto newPlayer = std::make_shared<Player>("勇者");
                    stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
                }
            } else {
                if (stateManager) {
                    stateManager->changeState(std::make_unique<NightState>(player));
                    player->setCurrentNight(player->getCurrentNight() + 1);
                    TownState::s_levelGoalAchieved = false;
                    TownState::s_nightCount++;
                }
            }
        }
    } else {
        // タイマーが非アクティブな場合、静的変数もクリア
        TownState::s_nightTimerActive = false;
        TownState::s_nightTimer = 0.0f;
    }
    
    
    if (ui.getElements().size() >= 5) {
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
    graphics.setDrawColor(34, 139, 34, 255);
    graphics.clear();
    
    drawMap(graphics);
    drawPlayer(graphics);
    
    ui.render(graphics);
    
    CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
    CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
    CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    
    graphics.present();
}

void FieldState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<MainMenuState>(player));
        }
        return;
    }
    
    if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        checkTownEntrance();
        return;
    }
    
    handleMovement(input);
}

void FieldState::setupUI() {
    ui.clear();
    // StoryMessageBoxは無効化
    storyBox = nullptr;
}

void FieldState::handleMovement(const InputManager& input) {
    if (moveTimer > 0) return;
    
    int newX = playerX;
    int newY = playerY;
    
    if (input.isKeyPressed(InputKey::UP) || input.isKeyPressed(InputKey::W)) {
        newY--;
    } else if (input.isKeyPressed(InputKey::DOWN) || input.isKeyPressed(InputKey::S)) {
        newY++;
    } else if (input.isKeyPressed(InputKey::LEFT) || input.isKeyPressed(InputKey::A)) {
        newX--;
    } else if (input.isKeyPressed(InputKey::RIGHT) || input.isKeyPressed(InputKey::D)) {
        newX++;
    }
    
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
        bool actuallyMoved = (newX != playerX || newY != playerY);
        
        if (actuallyMoved) {
            playerX = newX;
            playerY = newY;
            
            s_staticPlayerX = newX;
            s_staticPlayerY = newY;
            
            moveTimer = MOVE_DELAY;
            hasMoved = true; // 移動フラグを設定 
            checkEncounter();
            
            checkTownEntrance();
        } else {
            
        }
    }
}

void FieldState::checkEncounter() {
    if (!hasMoved) {
        return;
    }
    
    TerrainType currentTerrain = getCurrentTerrain();
    TerrainData terrainData = TerrainRenderer::getTerrainData(currentTerrain);
    
    if (terrainData.encounterRate <= 0) {
        return;
    }
    
    if (playerY >= 0 && playerY < 16 && playerX >= 0 && playerX < 28) {
        if (!terrainMap.empty() && playerY < static_cast<int>(terrainMap.size()) && 
            !terrainMap[0].empty() && playerX < static_cast<int>(terrainMap[0].size())) {
            const MapTile& currentTile = terrainMap[playerY][playerX];
            if (currentTile.objectType == 2) { // モンスター専用タイル
                EnemyType enemyType = EnemyType::SLIME; // デフォルト
                int enemyLevel = 1; // デフォルト
                for (size_t i = 0; i < activeMonsterPoints.size(); i++) {
                    if (activeMonsterPoints[i].first == playerX && activeMonsterPoints[i].second == playerY) {
                        enemyType = activeMonsterTypes[i];
                        if (i < activeMonsterLevels.size()) {
                            enemyLevel = activeMonsterLevels[i];
                        }
                        break;
                    }
                }
                
                Enemy enemy(enemyType);
                enemy.setLevel(enemyLevel);
                if (stateManager) {
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
    
    
    hasMoved = false;
}

void FieldState::drawMap(Graphics& graphics) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 28; x++) {
            drawTerrain(graphics, terrainMap[y][x], x, y);
        }
    }
}

void FieldState::drawTerrain(Graphics& graphics, const MapTile& tile, int x, int y) {
    int drawX = x * TILE_SIZE;
    int drawY = y * TILE_SIZE;
    
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
            textureName = "grass"; // town_entrance.pngが存在しないため、grassを使用
            break;
        default:
            textureName = "grass"; // デフォルト
            break;
    }
    
    if (tile.terrain == TerrainType::ROCK) {
        SDL_Texture* grassTexture = graphics.getTexture("grass");
        if (grassTexture) {
            graphics.drawTexture(grassTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
        }
    }
    
    terrainTexture = graphics.getTexture(textureName);
    if (terrainTexture) {
        graphics.drawTexture(terrainTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
    } else {
        auto terrainColor = TerrainRenderer::getTerrainColor(tile.terrain);
        graphics.setDrawColor(terrainColor.r, terrainColor.g, terrainColor.b, terrainColor.a);
        graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
        
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
            EnemyType enemyType = EnemyType::SLIME; // デフォルト
            int enemyLevel = 1; // デフォルト
            for (size_t i = 0; i < activeMonsterPoints.size(); i++) {
                if (activeMonsterPoints[i].first == x && activeMonsterPoints[i].second == y) {
                    enemyType = activeMonsterTypes[i];
                    if (i < activeMonsterLevels.size()) {
                        enemyLevel = activeMonsterLevels[i];
                    }
                    break;
                }
            }
            
            Enemy tempEnemy(enemyType);
            tempEnemy.setLevel(enemyLevel);
            std::string enemyTextureName = "enemy_" + tempEnemy.getTypeName();
            SDL_Texture* enemyTexture = graphics.getTexture(enemyTextureName);
            
            // プレイヤーレベルと比較して色を決定
            int playerLevel = player->getLevel();
            SDL_Color levelColor;
            if (enemyLevel < playerLevel) {
                levelColor = {0, 255, 0, 255}; // 緑（弱い）
            } else if (enemyLevel == playerLevel) {
                levelColor = {255, 255, 255, 255}; // 白（同じ）
            } else {
                levelColor = {255, 0, 0, 255}; // 赤（高い）
            }
            
            auto& config = UIConfig::UIConfigManager::getInstance();
            auto fieldConfig = config.getFieldConfig();
            
            if (enemyTexture) {
                // 元の画像サイズを取得してアスペクト比を保持
                int textureWidth, textureHeight;
                SDL_QueryTexture(enemyTexture, nullptr, nullptr, &textureWidth, &textureHeight);
                
                // タイルサイズの80%を基準に、アスペクト比を保持してサイズを計算
                int baseSize = static_cast<int>(TILE_SIZE * 0.8);
                float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
                
                int displayWidth, displayHeight;
                if (textureWidth > textureHeight) {
                    // 横長の画像
                    displayWidth = baseSize;
                    displayHeight = static_cast<int>(baseSize / aspectRatio);
                } else {
                    // 縦長または正方形の画像
                    displayHeight = baseSize;
                    displayWidth = static_cast<int>(baseSize * aspectRatio);
                }
                
                int enemyX = drawX + (TILE_SIZE - displayWidth) / 2;
                int enemyY = drawY + (TILE_SIZE - displayHeight) / 2;
                graphics.drawTexture(enemyTexture, enemyX, enemyY, displayWidth, displayHeight);
                
                std::string levelText = "Lv" + std::to_string(enemyLevel);
                int levelX = drawX + 6 + static_cast<int>(fieldConfig.monsterLevel.position.absoluteX);
                int levelY = drawY - 10 + static_cast<int>(fieldConfig.monsterLevel.position.absoluteY);
                graphics.drawText(levelText, levelX, levelY, "default", levelColor);
            } else {
                graphics.setDrawColor(255, 0, 0, 255);
                graphics.drawRect(objX, objY, objSize, objSize, true);
                
                std::string levelText = "Lv" + std::to_string(enemyLevel);
                int levelX = drawX + 6 + static_cast<int>(fieldConfig.monsterLevel.position.absoluteX);
                int levelY = drawY - 10 + static_cast<int>(fieldConfig.monsterLevel.position.absoluteY);
                graphics.drawText(levelText, levelX, levelY, "default", levelColor);
            }

        } else { // 岩や木の場合は四角形
            graphics.drawRect(objX, objY, objSize, objSize, true);
        }
    }
    
    if (tile.terrain == TerrainType::TOWN_ENTRANCE) {
        drawFieldGate(graphics);
    }
}

void FieldState::drawPlayer(Graphics& graphics) {
    int drawX = playerX * TILE_SIZE + 4;
    int drawY = playerY * TILE_SIZE + 4;
    int size = TILE_SIZE - 8;
    
    SDL_Texture* playerTexture = graphics.getTexture("player_field");
    if (playerTexture) {
        graphics.drawTexture(playerTexture, drawX, drawY, size, size);
    } else {
        graphics.setDrawColor(0, 0, 255, 255);
        graphics.drawRect(drawX, drawY, size, size, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(drawX, drawY, size, size, false);
    }
    
    // プレイヤーの上にレベルを表示
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto fieldConfig = config.getFieldConfig();
    int playerLevel = player->getLevel();
    std::string levelText = "Lv" + std::to_string(playerLevel);
    SDL_Color levelColor = {255, 255, 255, 255}; // 白（プレイヤー自身なので固定）
    
    // 敵のレベル表示と同じ位置計算（タイルの左上から相対位置）
    int levelX = drawX + 6 + static_cast<int>(fieldConfig.monsterLevel.position.absoluteX);
    int levelY = drawY - 10 + static_cast<int>(fieldConfig.monsterLevel.position.absoluteY);
    graphics.drawText(levelText, levelX, levelY, "default", levelColor);
}

void FieldState::checkTownEntrance() {
    TerrainType currentTerrain = getCurrentTerrain();
    if (currentTerrain == TerrainType::TOWN_ENTRANCE) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<TownState>(player));
        }
    }
}

bool FieldState::isValidPosition(int x, int y) const {
    if (x < 0 || x >= 28 || y < 0 || y >= 16) {
        return false;
    }
    
    if (terrainMap.empty() || y >= static_cast<int>(terrainMap.size()) || 
        terrainMap[0].empty() || x >= static_cast<int>(terrainMap[0].size())) {
        return false;
    }
    
    TerrainData terrainData = TerrainRenderer::getTerrainData(terrainMap[y][x].terrain);
    if (!terrainData.walkable) {
        return false;
    }
    
    if (terrainMap[y][x].hasObject && terrainMap[y][x].objectType == 1) { // 岩は通れない
        return false;
    }
    
    return true;
}

TerrainType FieldState::getCurrentTerrain() const {
    if (playerY >= 0 && playerY < 16 && playerX >= 0 && playerX < 28) {
        if (!terrainMap.empty() && playerY < static_cast<int>(terrainMap.size()) && 
            !terrainMap[0].empty() && playerX < static_cast<int>(terrainMap[0].size())) {
            return terrainMap[playerY][playerX].terrain; // 正しい順序でアクセス
        } else {
        }
    }
    return TerrainType::GRASS; // デフォルト
}

void FieldState::loadFieldImages() {
}

void FieldState::generateMonsterSpawnPoints() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(1, 26); // 境界を避ける
    std::uniform_int_distribution<> disY(1, 14); // 境界を避ける
    
    monsterSpawnPoints.clear();
    activeMonsterPoints.clear();
    activeMonsterTypes.clear();
    activeMonsterLevels.clear();
    
    int playerLevel = player->getLevel();
    std::uniform_int_distribution<> disLevel(std::max(1, playerLevel - 2), playerLevel + 2); // プレイヤーレベル±2の範囲
    
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
        
        EnemyType enemyType = Enemy::createRandomEnemy(playerLevel).getType();
        int enemyLevel = disLevel(gen); // プレイヤーレベル±2の範囲でランダム
        
        monsterSpawnPoints.push_back({x, y});
        activeMonsterPoints.push_back({x, y});
        activeMonsterTypes.push_back(enemyType);
        activeMonsterLevels.push_back(enemyLevel);
        
        terrainMap[y][x].hasObject = true;
        terrainMap[y][x].objectType = 2; // モンスター専用タイル
    }
}

void FieldState::relocateMonsterSpawnPoint(int oldX, int oldY) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(1, 26);
    std::uniform_int_distribution<> disY(1, 14);
    
    terrainMap[oldY][oldX].hasObject = false;
    terrainMap[oldY][oldX].objectType = 0;
    
    int newX, newY;
    bool validPosition;
    
    do {
        newX = disX(gen);
        newY = disY(gen);
        validPosition = isValidPosition(newX, newY) && 
                      terrainMap[newY][newX].terrain == TerrainType::GRASS &&
                      !terrainMap[newY][newX].hasObject;
    } while (!validPosition);
    
    terrainMap[newY][newX].hasObject = true;
    terrainMap[newY][newX].objectType = 2;
    
    int playerLevel = player->getLevel();
    std::uniform_int_distribution<> disLevel(std::max(1, playerLevel - 2), playerLevel + 2); // プレイヤーレベル±2の範囲
    
    for (size_t i = 0; i < activeMonsterPoints.size(); i++) {
        if (activeMonsterPoints[i].first == oldX && activeMonsterPoints[i].second == oldY) {
            activeMonsterPoints[i].first = newX;
            activeMonsterPoints[i].second = newY;
            activeMonsterTypes[i] = Enemy::createRandomEnemy(playerLevel).getType();
            activeMonsterLevels[i] = disLevel(gen); // プレイヤーレベル±2の範囲でランダム
            break;
        }
    }
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
    if (ui.getElements().size() >= 3) {
        static_cast<Label*>(ui.getElements()[2].get())->setText("レベルアップ！新しい力が目覚めた！");
    }
}

void FieldState::drawFieldGate(Graphics& graphics) {
    int gateX = 26;
    int gateY = 8;
    
    SDL_Texture* grassTexture = graphics.getTexture("grass");
    if (grassTexture) {
        int drawX = gateX * TILE_SIZE;
        int drawY = gateY * TILE_SIZE;
        graphics.drawTexture(grassTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
    }
    
    SDL_Texture* toriiTexture = graphics.getTexture("torii");
    if (toriiTexture) {
        int drawX = gateX * TILE_SIZE;
        int drawY = gateY * TILE_SIZE;
        // 鳥居は少し大きく描画（2タイルサイズ）
        graphics.drawTexture(toriiTexture, drawX - TILE_SIZE/2, drawY - TILE_SIZE, 
                           TILE_SIZE * 2, TILE_SIZE * 2);
    } else {
        int drawX = gateX * TILE_SIZE + TILE_SIZE/4;
        int drawY = gateY * TILE_SIZE - TILE_SIZE/2;
        graphics.setDrawColor(255, 0, 0, 255); // 赤色
        graphics.drawRect(drawX, drawY, TILE_SIZE * 1.5, TILE_SIZE * 1.5, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(drawX, drawY, TILE_SIZE * 1.5, TILE_SIZE * 1.5, false);
    }
} 