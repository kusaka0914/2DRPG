#pragma once
#include "Graphics.h"
#include "InputManager.h"

enum class StateType {
    MAIN_MENU,
    FIELD,
    BATTLE,
    TOWN,
    CASTLE,
    ROOM,
    NIGHT,
    DEMON_CASTLE
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