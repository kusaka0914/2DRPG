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
    const int TILE_SIZE = 32;
    const int MAP_WIDTH = 25;
    const int MAP_HEIGHT = 18;
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // エンカウント関連（移動時のみチェック）
    
    // 地形マップ
    std::vector<std::vector<MapTile>> terrainMap;
    bool hasMoved;

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
}; 