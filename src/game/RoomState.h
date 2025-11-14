#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../core/GameUtils.h"
#include <memory>

class RoomState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // メッセージボード用UI
    Label* messageBoard;
    Label* howtooperateBoard;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int ROOM_WIDTH = 7;   // 5 → 7に変更
    const int ROOM_HEIGHT = 5;  // 3 → 5に変更
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // 部屋のオブジェクト位置
    int bedX, bedY;      // ベッド
    int deskX, deskY;    // 机
    int chestX, chestY;  // 宝箱
    int doorX, doorY;    // 出口ドア
    
    // ゲーム状態
    bool hasOpenedChest; // 宝箱を開けたか
    bool isFirstTime;    // 初回プレイか
    bool isShowingMessage; // メッセージ表示中か
    
    // メッセージ表示用フラグ
    bool pendingWelcomeMessage; // ウェルカムメッセージを表示するか
    std::string pendingMessage; // 保留中のメッセージ
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* deskTexture;
    SDL_Texture* chestClosedTexture;
    SDL_Texture* chestOpenTexture;
    SDL_Texture* bedTexture;
    SDL_Texture* houseTileTexture;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒

public:
    RoomState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    StateType getType() const override;
    
private:
    void setupUI(Graphics& graphics);
    void setupRoom();
    void loadTextures(Graphics& graphics);
    void handleMovement(const InputManager& input);
    void checkInteraction();
    void drawRoom(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    void drawRoomObjects(Graphics& graphics);
    bool isValidPosition(int x, int y) const;
    bool isNearObject(int x, int y) const;
    void showWelcomeMessage();
    void interactWithBed();
    void interactWithDesk();
    void interactWithChest();
    void exitToTown();
    void showMessage(const std::string& message);
    void clearMessage();
}; 