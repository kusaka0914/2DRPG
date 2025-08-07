#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include "GameUtils.h"
#include <memory>

class DemonCastleState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int ROOM_WIDTH = 9;   // 28 → 9に変更
    const int ROOM_HEIGHT = 11; // 16 → 11に変更
    
    // 魔王の位置
    int demonX, demonY;
    int doorX, doorY;        // 出口ドア
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // メッセージ表示
    Label* messageBoard;
    bool isShowingMessage;
    
    // 魔王との会話
    bool isTalkingToDemon;
    int dialogueStep;
    std::vector<std::string> demonDialogues;
    bool hasReceivedEvilQuest;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* demonTexture;
    SDL_Texture* demonCastleTileTexture;
    
    // CastleStateから来たかどうか
    bool fromCastleState;
    
public:
    DemonCastleState(std::shared_ptr<Player> player, bool fromCastleState = false);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::DEMON_CASTLE; }
    
private:
    void setupUI();
    void setupDemonCastle();
    void loadTextures(Graphics& graphics);
    void handleMovement(const InputManager& input);
    void checkInteraction();
    void startDialogue();
    void nextDialogue();
    void showCurrentDialogue();
    void showMessage(const std::string& message);
    void clearMessage();
    void drawDemonCastle(Graphics& graphics);
    void drawDemonCastleObjects(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    bool isValidPosition(int x, int y) const;
    bool isNearObject(int x, int y) const;
    void interactWithDemon();
    void exitToTown();
}; 