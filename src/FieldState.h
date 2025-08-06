#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include "Enemy.h"
#include "MapTerrain.h"
#include <memory>

class FieldState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    StoryMessageBox* storyBox;  // ストーリーメッセージ用
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int MAP_WIDTH = 28;  // 25 → 28に拡大（画面幅1100px ÷ 38px = 約29タイル、UI部分を考慮して28）
    const int MAP_HEIGHT = 16; // 18 → 16に調整（画面高さ650px ÷ 38px = 約17タイル、UI部分を考慮して16）
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // エンカウント関連（移動時のみチェック）
    
    // モンスター出現場所管理
    std::vector<std::pair<int, int>> monsterSpawnPoints; // モンスター出現場所のリスト
    std::vector<std::pair<int, int>> activeMonsterPoints; // 現在アクティブなモンスター出現場所
    std::vector<EnemyType> activeMonsterTypes; // 各出現場所の敵の種類
    
    // 戦闘終了時の処理
    bool shouldRelocateMonster;
    int lastBattleX, lastBattleY;
    
    // 地形マップ
    std::vector<std::vector<MapTile>> terrainMap;
    bool hasMoved;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒

public:
    FieldState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::FIELD; }
    
    // ストーリーシステム
    void showOpeningStory();
    void showLevelUpStory(int newLevel);
    
private:
    void setupUI();
    void handleMovement(const InputManager& input);
    void checkEncounter();
    void checkTownEntrance();
    void drawMap(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    void drawTerrain(Graphics& graphics, const MapTile& tile, int x, int y);
    bool isValidPosition(int x, int y) const;
    TerrainType getCurrentTerrain() const;
    void loadFieldImages();
    void generateMonsterSpawnPoints();
    void relocateMonsterSpawnPoint(int oldX, int oldY);
    void drawFieldGate(Graphics& graphics);
}; 