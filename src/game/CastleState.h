#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../core/GameUtils.h"
#include <memory>

class CastleState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int ROOM_WIDTH = 9;  // 28 → 13に変更
    const int ROOM_HEIGHT = 11; // 16 → 11に変更
    
    // 城のオブジェクト位置（1タイルサイズ）
    int throneX, throneY;    // 王座（中央）
    int guardLeftX, guardLeftY;  // 左衛兵
    int guardRightX, guardRightY; // 右衛兵
    int doorX, doorY;        // 出口ドア
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // メッセージ表示
    Label* messageBoard;
    bool isShowingMessage;
    
    // 王様との会話
    bool isTalkingToKing;
    int dialogueStep;
    std::vector<std::string> kingDialogues;
    bool hasReceivedQuest;
    
    // 魔王の城への移行フラグ
    bool shouldGoToDemonCastle;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* kingTexture;
    SDL_Texture* guardTexture;
    SDL_Texture* castleTileTexture;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒
    
    // NightStateから来たかどうか
    bool fromNightState;
    
    // NightStateからの場合の戦闘用変数
    bool kingDefeated;
    bool guardLeftDefeated;
    bool guardRightDefeated;
    bool allDefeated;
    
public:
    CastleState(std::shared_ptr<Player> player, bool fromNightState = false);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::CASTLE; }
    
private:
    void setupUI();
    void setupCastle();
    void loadTextures(Graphics& graphics);
    void handleMovement(const InputManager& input);
    void checkInteraction();
    void startDialogue();
    void nextDialogue();
    void showCurrentDialogue();
    void showMessage(const std::string& message);
    void clearMessage();
    void drawCastle(Graphics& graphics);
    void drawCastleObjects(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    bool isValidPosition(int x, int y) const;
    bool isNearObject(int x, int y) const;
    void interactWithThrone();
    void interactWithGuard();
    void attackKing();
    void attackGuardLeft();
    void attackGuardRight();
    void checkAllDefeated();
    void exitToTown();
}; 