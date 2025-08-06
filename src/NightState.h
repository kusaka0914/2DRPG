#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include "GameUtils.h"
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
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* residentTexture;
    SDL_Texture* guardTexture;
    
    // メッセージ表示
    Label* messageLabel;
    bool isShowingMessage;
    
public:
    NightState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::NIGHT; }
    
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
    bool isValidPosition(int x, int y) const;
    void updateUI();
    void checkTrustLevels();
    void updateGuards(float deltaTime);
    bool isNearGuard(int x, int y) const;
}; 