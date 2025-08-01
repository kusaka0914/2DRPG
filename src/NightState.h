#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include <memory>

class NightState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 32;
    
    // 夜間の街の状態
    bool isStealthMode;
    int stealthLevel;
    std::vector<std::pair<int, int>> residents; // 住民の位置
    std::vector<std::pair<int, int>> guards; // 見張りの位置
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
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
    bool isValidPosition(int x, int y) const;
}; 