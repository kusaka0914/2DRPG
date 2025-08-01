#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include <memory>

class CastleState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    StoryMessageBox* messageBox;  // 王様との会話用（現在無効化）
    
    // プレイヤーと王様の位置（画面座標）
    int playerX, playerY;
    int kingX, kingY;
    
    // 会話システム
    bool isTalkingToKing;
    std::vector<std::string> currentDialogue;
    int dialogueIndex;
    bool hasReceivedQuest;
    
    // 移動タイマー
    float moveTimer;
    
    // 描画最適化用
    bool castleRendered;

public:
    CastleState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::FIELD; } // 便宜上FIELDを使用
    
private:
    void setupUI();
    void setupCastle();
    void handleMovement(const InputManager& input);
    void handleKingInteraction();
    void startDialogue();
    void nextDialogue();
    void endDialogue();
    void drawSimpleField(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    void drawKing(Graphics& graphics);
    bool isNearKing() const;
}; 