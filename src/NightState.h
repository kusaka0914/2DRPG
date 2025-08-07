#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include "GameUtils.h"
#include "TownLayout.h"
#include <memory>

class NightState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    
    // 夜間の街の状態
    bool isStealthMode;
    int stealthLevel;
    std::vector<std::pair<int, int>> residents; // 住民の位置
    std::vector<std::pair<int, int>> guards; // 見張りの位置
    std::vector<std::pair<int, int>> guardDirections; // 衛兵の移動方向
    
    // 住民を倒した回数
    int residentsKilled;
    const int MAX_RESIDENTS_PER_NIGHT = 3;
    
    // 合計倒した人数（メンタル計算用）
    static int totalResidentsKilled;
    
    // 倒した住民の位置を記録（次の夜に配置しないため）
    static std::vector<std::pair<int, int>> killedResidentPositions;
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* residentTextures[6]; // 住民画像（6人分）
    SDL_Texture* guardTexture;
    SDL_Texture* shopTexture;
    SDL_Texture* weaponShopTexture;
    SDL_Texture* houseTexture;
    SDL_Texture* castleTexture;
    SDL_Texture* stoneTileTexture;
    SDL_Texture* residentHomeTexture;
    SDL_Texture* toriiTexture;
    
    // メッセージ表示
    Label* messageLabel;
    bool isShowingMessage;
    
    // 建物の位置（街と同じ）
    std::vector<std::pair<int, int>> buildings;
    std::vector<std::string> buildingTypes;
    std::vector<std::pair<int, int>> residentHomes;
    
    // 城の位置
    int castleX, castleY;
    
    // 衛兵の移動管理
    float guardMoveTimer;
    std::vector<int> guardTargetHomeIndices;
    std::vector<float> guardStayTimers;
    bool guardsInitialized;
    
public:
    NightState(std::shared_ptr<Player> player);
    ~NightState();
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::NIGHT; }
    
    // 倒した住民の位置を取得するpublic関数
    static const std::vector<std::pair<int, int>>& getKilledResidentPositions() { return killedResidentPositions; }
    
private:
    void setupUI();
    void handleMovement(const InputManager& input);
    void checkResidentInteraction();
    void attackResident(int x, int y);
    void hideEvidence();
    void showMessage(const std::string& message);
    void clearMessage();
    void drawNightTown(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    void loadTextures(Graphics& graphics);
    void drawMap(Graphics& graphics);
    void drawBuildings(Graphics& graphics);
    void drawGate(Graphics& graphics);
    bool isValidPosition(int x, int y) const;
    bool isCollidingWithBuilding(int x, int y) const;
    bool isCollidingWithResident(int x, int y) const;
    bool isCollidingWithGuard(int x, int y) const;
    int getResidentTextureIndex(int x, int y) const;
    void updateUI();
    void checkTrustLevels();
    void updateGuards(float deltaTime);
    bool isNearGuard(int x, int y) const;
}; 