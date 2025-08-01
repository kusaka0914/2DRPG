#include "FieldState.h"
#include "MainMenuState.h"
#include "BattleState.h"
#include "TownState.h"
#include "MapTerrain.h"
#include <random>

// 静的変数（ファイル内で共有）
static int s_staticPlayerX = 12;  // 25の中央：12
static int s_staticPlayerY = 9;   // 18の中央：9
static bool s_positionInitialized = false;

FieldState::FieldState(std::shared_ptr<Player> player) 
    : player(player), moveTimer(0), storyBox(nullptr) {
    // 多様な地形マップ生成（画面いっぱいサイズ25x18）
    static std::vector<std::vector<MapTile>> staticTerrainMap;
    static bool mapGenerated = false;
    
    if (!mapGenerated) {
        // リアルなマップを生成（川、森、山など）- 画面いっぱいサイズ
        staticTerrainMap = MapGenerator::generateRealisticMap(25, 18);
        
        // 街の入り口を追加（右端中央付近）
        staticTerrainMap[9][24] = MapTile(TerrainType::TOWN_ENTRANCE);
        
        std::cout << "リアルなフィールドマップを生成しました！" << std::endl;
        mapGenerated = true;
    }
    
    if (!s_positionInitialized) {
        std::cout << "フィールドの初期位置を設定しました！" << std::endl;
        s_positionInitialized = true;
    }
    
    terrainMap = staticTerrainMap;
    playerX = s_staticPlayerX;
    playerY = s_staticPlayerY;
}

void FieldState::enter() {
    setupUI();
    loadFieldImages();
    
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
    
    // レベルアップストーリーのチェック
    if (player->hasLevelUpStory()) {
        showLevelUpStory(player->getLevelUpStoryLevel());
        player->clearLevelUpStoryFlag();
    }
    
    ui.update(deltaTime);
    
    // UI情報を更新（値が変更された場合のみ）
    if (ui.getElements().size() >= 2) {
        // プレイヤー情報（変更検出）
        static std::string lastPlayerInfo = "";
        std::string playerInfo = player->getName() + " Lv:" + std::to_string(player->getLevel()) + 
                                " HP:" + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp()) +
                                " Gold:" + std::to_string(player->getGold());
        
        if (playerInfo != lastPlayerInfo) {
            static_cast<Label*>(ui.getElements()[0].get())->setText(playerInfo);
            lastPlayerInfo = playerInfo;
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
            static_cast<Label*>(ui.getElements()[1].get())->setText(terrainInfo);
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
    
    graphics.present();
}

void FieldState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // ESCキーでメインメニューに戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE)) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<MainMenuState>(player));
        }
        return;
    }
    
    // スペースキーで街に入る
    if (input.isKeyJustPressed(InputKey::SPACE)) {
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
    
    // 現在の地形情報表示
    auto terrainLabel = std::make_unique<Label>(10, 35, "", "default");
    terrainLabel->setColor({200, 255, 200, 255});
    ui.addElement(std::move(terrainLabel));
    
    // 操作説明
    auto controlsLabel = std::make_unique<Label>(10, 550, "方向キー: 移動（敵遭遇あり）, スペース: 街へ入る, ESC: メニューに戻る", "default");
    controlsLabel->setColor({200, 200, 200, 255});
    ui.addElement(std::move(controlsLabel));
    
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
        
        // 静的位置変数も更新（位置を保持するため）
        s_staticPlayerX = newX;
        s_staticPlayerY = newY;
        
        moveTimer = MOVE_DELAY;
        
        // 移動した時のみエンカウントチェック
        checkEncounter();
    }
}

void FieldState::checkEncounter() {
    // 現在の地形に基づいてエンカウント率を決定
    TerrainType currentTerrain = getCurrentTerrain();
    TerrainData terrainData = TerrainRenderer::getTerrainData(currentTerrain);
    
    // エンカウント率が0の地形では敵が出現しない
    if (terrainData.encounterRate <= 0) {
        return;
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    if (dis(gen) <= terrainData.encounterRate) {
        Enemy enemy = Enemy::createRandomEnemy(player->getLevel());
        if (stateManager) {
            stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(enemy)));
        }
    }
}

void FieldState::drawMap(Graphics& graphics) {
    // 地形ベースのタイル描画（画面いっぱいマップサイズ25x18に対応）
    for (int y = 0; y < 18; y++) {
        for (int x = 0; x < 25; x++) {
            drawTerrain(graphics, terrainMap[y][x], x, y);
        }
    }
}

void FieldState::drawTerrain(Graphics& graphics, const MapTile& tile, int x, int y) {
    int drawX = x * TILE_SIZE;
    int drawY = y * TILE_SIZE;
    
    // 地形の色を取得
    auto terrainColor = TerrainRenderer::getTerrainColor(tile.terrain);
    graphics.setDrawColor(terrainColor.r, terrainColor.g, terrainColor.b, terrainColor.a);
    graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
    
    // 地形の境界線（地形タイプによって色を変える）
    if (tile.terrain == TerrainType::WATER) {
        graphics.setDrawColor(0, 50, 150, 255);  // 濃い青
    } else if (tile.terrain == TerrainType::FOREST) {
        graphics.setDrawColor(0, 50, 0, 255);    // 濃い緑
    } else if (tile.terrain == TerrainType::MOUNTAIN) {
        graphics.setDrawColor(100, 100, 100, 255); // 濃いグレー
    } else if (tile.terrain == TerrainType::TOWN_ENTRANCE) {
        graphics.setDrawColor(255, 215, 0, 255);  // 金色の境界線
    } else {
        graphics.setDrawColor(20, 100, 20, 255);  // デフォルト
    }
    graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, false);
    
    // オブジェクトがある場合は描画
    if (tile.hasObject) {
        auto objectColor = TerrainRenderer::getObjectColor(tile.objectType);
        graphics.setDrawColor(objectColor.r, objectColor.g, objectColor.b, objectColor.a);
        
        int objSize = TILE_SIZE / 3;
        int objX = drawX + (TILE_SIZE - objSize) / 2;
        int objY = drawY + (TILE_SIZE - objSize) / 2;
        
        if (tile.objectType == 2) { // 花の場合は小さな円形
            graphics.drawRect(objX + objSize/4, objY + objSize/4, objSize/2, objSize/2, true);
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
    // 画面いっぱいマップサイズ（25x18）に対応
    if (x < 0 || x >= 25 || y < 0 || y >= 18) {
        return false;
    }
    
    // 地形データに基づいて移動可能性をチェック
    TerrainData terrainData = TerrainRenderer::getTerrainData(terrainMap[y][x].terrain);
    if (!terrainData.walkable) {
        return false;
    }
    
    // オブジェクトがある場合の移動制限
    if (terrainMap[y][x].hasObject && terrainMap[y][x].objectType == 1) { // 岩は通れない
        return false;
    }
    
    return true;
}

TerrainType FieldState::getCurrentTerrain() const {
    // 新しいマップサイズ（20x15）での境界チェック
    if (playerY >= 0 && playerY < 15 && playerX >= 0 && playerX < 20) {
        return terrainMap[playerY][playerX].terrain;
    }
    return TerrainType::GRASS; // デフォルト
}

void FieldState::loadFieldImages() {
    // 注意: 画像読み込みはSDL2Game::loadResourcesで行うため、ここでは何もしない
    // 実際の画像読み込みはゲーム初期化時に実行される
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