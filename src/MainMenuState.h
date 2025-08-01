#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include <memory>

class MainMenuState : public GameState {
private:
    UIManager ui;
    std::shared_ptr<Player> player;
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> playerInfoLabel;

public:
    MainMenuState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::MAIN_MENU; }
    
private:
    void setupUI();
    void updatePlayerInfo();
}; 