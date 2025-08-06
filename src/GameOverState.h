#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include <memory>

class GameOverState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    std::string gameOverReason;
    
    // UI要素
    Label* titleLabel;
    Label* reasonLabel;
    Label* instructionLabel;
    
public:
    GameOverState(std::shared_ptr<Player> player, const std::string& reason);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::GAME_OVER; }
    
private:
    void setupUI();
}; 