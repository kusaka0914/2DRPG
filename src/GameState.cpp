#include "GameState.h"

GameStateManager::GameStateManager() : shouldChangeState(false) {
}

GameStateManager::~GameStateManager() {
}

void GameStateManager::changeState(std::unique_ptr<GameState> newState) {
    newState->setStateManager(this);
    nextState = std::move(newState);
    shouldChangeState = true;
}

void GameStateManager::update(float deltaTime) {
    if (shouldChangeState) {
        performStateChange();
    }
    
    if (currentState) {
        currentState->update(deltaTime);
    }
}

void GameStateManager::render(Graphics& graphics) {
    if (currentState) {
        currentState->render(graphics);
    }
}

void GameStateManager::handleInput(const InputManager& input) {
    if (currentState) {
        currentState->handleInput(input);
    }
}

StateType GameStateManager::getCurrentStateType() const {
    if (currentState) {
        return currentState->getType();
    }
    return StateType::MAIN_MENU;
}

void GameStateManager::performStateChange() {
    if (currentState) {
        currentState->exit();
    }
    
    currentState = std::move(nextState);
    nextState.reset();
    
    if (currentState) {
        currentState->enter();
    }
    
    shouldChangeState = false;
} 