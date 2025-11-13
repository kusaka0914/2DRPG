#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include <memory>

enum class EndingPhase {
    ENDING_MESSAGE,
    STAFF_ROLL,
    COMPLETE
};

class EndingState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    EndingPhase currentPhase;
    float phaseTimer;
    float scrollOffset;
    
    // UI要素
    Label* messageLabel;
    Label* staffRollLabel;
    
    // エンディングメッセージ
    std::vector<std::string> endingMessages;
    int currentMessageIndex;
    
    // スタッフロール
    std::vector<std::string> staffRoll;
    int currentStaffIndex;
    
    // 表示設定
    const float MESSAGE_DISPLAY_TIME = 3.0f;
    const float STAFF_ROLL_SPEED = 50.0f;
    const float STAFF_ROLL_DELAY = 0.5f;

public:
    EndingState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    StateType getType() const override { return StateType::ENDING; }
    
private:
    void setupUI();
    void setupEndingMessages();
    void setupStaffRoll();
    void showCurrentMessage();
    void showStaffRoll();
    void nextPhase();
};
