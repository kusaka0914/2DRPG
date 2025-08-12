#pragma once
#include "Graphics.h"
#include "InputManager.h"
#include "UI.h"
#include "Player.h"
#include <memory>

enum class StateType {
    MAIN_MENU,
    FIELD,
    BATTLE,
    TOWN,
    CASTLE,
    ROOM,
    NIGHT,
    DEMON_CASTLE,
    GAME_OVER,
    ENDING
};

// 前方宣言
class GameStateManager;

class GameState {
public:
    virtual ~GameState() = default;
    
    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(Graphics& graphics) = 0;
    virtual void handleInput(const InputManager& input) = 0;
    
    virtual StateType getType() const = 0;
    
protected:
    GameStateManager* stateManager = nullptr;
    
public:
    void setStateManager(GameStateManager* manager) { stateManager = manager; }
    
    // 共通のメッセージ処理
    void showMessage(const std::string& message, Label* messageBoard, bool& isShowingMessage);
    void clearMessage(Label* messageBoard, bool& isShowingMessage);
    
    // 共通の移動処理
    void handleMovement(const InputManager& input, int& playerX, int& playerY, 
                       float& moveTimer, const float MOVE_DELAY,
                       std::function<bool(int, int)> isValidPosition,
                       std::function<void(int, int)> onMovement);
    
    // 共通の距離チェック
    bool isNearObject(int playerX, int playerY, int objX, int objY, int maxDistance = 1) const;
    
    // 共通の位置検証
    bool isValidPosition(int x, int y, int minX, int minY, int maxX, int maxY) const;
    
    // 共通のプレイヤー描画
    void drawPlayer(Graphics& graphics, int playerX, int playerY, int tileSize, 
                   Uint8 r = 0, Uint8 g = 255, Uint8 b = 0, Uint8 a = 255);
    
    // 共通の入力処理テンプレート
    void handleInputTemplate(const InputManager& input, UIManager& ui, bool& isShowingMessage,
                           float& moveTimer, std::function<void()> handleMovement,
                           std::function<void()> onInteraction, std::function<void()> onExit,
                           std::function<bool()> isNearExit, std::function<void()> clearMessage);
    
    // 共通の画像管理
    static SDL_Texture* loadPlayerTexture(Graphics& graphics);
    static SDL_Texture* loadKingTexture(Graphics& graphics);
    static SDL_Texture* loadGuardTexture(Graphics& graphics);
    static SDL_Texture* loadDemonTexture(Graphics& graphics);
    static void drawPlayerWithTexture(Graphics& graphics, SDL_Texture* playerTexture, 
                                    int playerX, int playerY, int tileSize);
    static void drawCharacterWithTexture(Graphics& graphics, SDL_Texture* texture, 
                                       int x, int y, int tileSize);
};

class GameStateManager {
private:
    std::unique_ptr<GameState> currentState;
    std::unique_ptr<GameState> nextState;
    bool shouldChangeState;

public:
    GameStateManager();
    ~GameStateManager();
    
    void changeState(std::unique_ptr<GameState> newState);
    void update(float deltaTime);
    void render(Graphics& graphics);
    void handleInput(const InputManager& input);
    
    GameState* getCurrentState() const { return currentState.get(); }
    StateType getCurrentStateType() const;
    
private:
    void performStateChange();
}; 