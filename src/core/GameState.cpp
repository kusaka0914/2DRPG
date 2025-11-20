#include "GameState.h"
#include "../entities/Player.h"
#include "../game/TownState.h"
#include "../core/AudioManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

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

void GameState::showMessage(const std::string& message, Label* messageBoard, bool& isShowingMessage) {
    if (messageBoard) {
        messageBoard->setText(message);
        isShowingMessage = true;
        
        // メッセージ表示時に効果音を再生
        AudioManager::getInstance().playSound("decide", 0);
    }
}

void GameState::clearMessage(Label* messageBoard, bool& isShowingMessage) {
    if (messageBoard) {
        messageBoard->setText("");
        isShowingMessage = false;
    }
}

void GameState::saveCurrentState(std::shared_ptr<Player> player) {
    if (player && stateManager) {
        GameState* currentState = stateManager->getCurrentState();
        if (currentState) {
            nlohmann::json stateJson = currentState->toJson();
            player->setSavedGameState(stateJson);
            float nightTimer = TownState::s_nightTimer;
            bool nightTimerActive = TownState::s_nightTimerActive;
            player->saveGame("autosave.json", nightTimer, nightTimerActive);
        }
    }
}

void GameState::handleMovement(const InputManager& input, int& playerX, int& playerY, 
                              float& moveTimer, const float MOVE_DELAY,
                              std::function<bool(int, int)> isValidPosition,
                              std::function<void(int, int)> onMovement) {
    if (moveTimer > 0) return;
    
    int newX = playerX;
    int newY = playerY;
    bool moved = false;
    
    if (input.isKeyPressed(InputKey::UP) || input.isKeyPressed(InputKey::W)) {
        newY--;
        moved = true;
    }
    if (input.isKeyPressed(InputKey::DOWN) || input.isKeyPressed(InputKey::S)) {
        newY++;
        moved = true;
    }
    if (input.isKeyPressed(InputKey::LEFT) || input.isKeyPressed(InputKey::A)) {
        newX--;
        moved = true;
    }
    if (input.isKeyPressed(InputKey::RIGHT) || input.isKeyPressed(InputKey::D)) {
        newX++;
        moved = true;
    }
    
    // ゲームパッドスティック入力
    float stickX = input.getLeftStickX();
    float stickY = input.getLeftStickY();
    const float DEAD_ZONE = 0.3f;
    
    if (std::abs(stickX) > DEAD_ZONE || std::abs(stickY) > DEAD_ZONE) {
        if (std::abs(stickY) > std::abs(stickX)) {
            if (stickY < -DEAD_ZONE) {
                newY--;
                moved = true;
            } else if (stickY > DEAD_ZONE) {
                newY++;
                moved = true;
            }
        } else {
            if (stickX < -DEAD_ZONE) {
                newX--;
                moved = true;
            } else if (stickX > DEAD_ZONE) {
                newX++;
                moved = true;
            }
        }
    }
    
    if (moved && isValidPosition(newX, newY)) {
        playerX = newX;
        playerY = newY;
        moveTimer = MOVE_DELAY;
        onMovement(newX, newY);
    }
}

bool GameState::isNearObject(int playerX, int playerY, int objX, int objY, int maxDistance) const {
    int dx = abs(playerX - objX);
    int dy = abs(playerY - objY);
    return (dx <= maxDistance && dy <= maxDistance);
}

bool GameState::isValidPosition(int x, int y, int minX, int minY, int maxX, int maxY) const {
    return x >= minX && x < maxX && y >= minY && y < maxY;
}

void GameState::drawPlayer(Graphics& graphics, int playerX, int playerY, int tileSize, 
                          Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    int drawX = playerX * tileSize + 4;
    int drawY = playerY * tileSize + 4;
    int size = tileSize - 8;
    
    graphics.setDrawColor(r, g, b, a);
    graphics.drawRect(drawX, drawY, size, size, true);
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.drawRect(drawX, drawY, size, size, false);
}

void GameState::handleInputTemplate(const InputManager& input, UIManager& ui, bool& isShowingMessage,
                                   float& moveTimer, std::function<void()> handleMovement,
                                   std::function<void()> onInteraction, std::function<void()> onExit,
                                   std::function<bool()> isNearExit, std::function<void()> clearMessage) {
    ui.handleInput(input);
    
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (moveTimer <= 0) {
        handleMovement();
    }
    
    if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (isNearExit()) {
            onExit();
        } else {
            onInteraction();
        }
    }
}

SDL_Texture* GameState::loadPlayerTexture(Graphics& graphics) {
    return graphics.loadTexture("assets/textures/characters/player.png", "player");
}

SDL_Texture* GameState::loadKingTexture(Graphics& graphics) {
    return graphics.loadTexture("assets/textures/characters/king.png", "king");
}

SDL_Texture* GameState::loadGuardTexture(Graphics& graphics) {
    return graphics.loadTexture("assets/textures/characters/guard.png", "guard");
}

SDL_Texture* GameState::loadDemonTexture(Graphics& graphics) {
    return graphics.loadTexture("assets/textures/characters/demon.png", "demon");
}

void GameState::drawPlayerWithTexture(Graphics& graphics, SDL_Texture* playerTexture, 
                                    int playerX, int playerY, int tileSize) {
    if (playerTexture) {
        // アスペクト比を保持して縦幅に合わせて描画
        int centerX = playerX * tileSize + tileSize / 2;
        int centerY = playerY * tileSize + tileSize / 2;
        graphics.drawTextureAspectRatio(playerTexture, centerX, centerY, tileSize, true, true);
    } else {
        int drawX = playerX * tileSize + 4;
        int drawY = playerY * tileSize + 4;
        int size = tileSize - 8;
        
        graphics.setDrawColor(0, 255, 0, 255);
        graphics.drawRect(drawX, drawY, size, size, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(drawX, drawY, size, size, false);
    }
}

void GameState::drawCharacterWithTexture(Graphics& graphics, SDL_Texture* texture, 
                                       int x, int y, int tileSize) {
    if (texture) {
        // アスペクト比を保持して縦幅に合わせて描画
        int centerX = x * tileSize + tileSize / 2;
        int centerY = y * tileSize + tileSize / 2;
        graphics.drawTextureAspectRatio(texture, centerX, centerY, tileSize, true, true);
    } else {
        graphics.setDrawColor(255, 0, 0, 255);
        graphics.drawRect(x * tileSize + 4, y * tileSize + 4, tileSize - 8, tileSize - 8, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(x * tileSize + 4, y * tileSize + 4, tileSize - 8, tileSize - 8, false);
    }
}

void GameState::drawGate(Graphics& graphics, int gateX, int gateY, int tileSize,
                        SDL_Texture* stoneTileTexture, SDL_Texture* toriiTexture) {
    if (stoneTileTexture) {
        int drawX = gateX * tileSize;
        int drawY = gateY * tileSize;
        graphics.drawTexture(stoneTileTexture, drawX, drawY, tileSize, tileSize);
    }
    
    if (toriiTexture) {
        int drawX = gateX * tileSize;
        int drawY = gateY * tileSize;
        // 鳥居は少し大きく描画（2タイルサイズ）
        graphics.drawTexture(toriiTexture, drawX - tileSize/2, drawY - tileSize, 
                           tileSize * 2, tileSize * 2);
    } else {
        int drawX = gateX * tileSize + tileSize/4;
        int drawY = gateY * tileSize - tileSize/2;
        graphics.setDrawColor(255, 0, 0, 255); // 赤色
        graphics.drawRect(drawX, drawY, tileSize * 1.5, tileSize * 1.5, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(drawX, drawY, tileSize * 1.5, tileSize * 1.5, false);
    }
}

void GameState::drawBuilding(Graphics& graphics, int x, int y, int tileSize, int buildingSize,
                             SDL_Texture* texture, Uint8 fallbackR, Uint8 fallbackG, Uint8 fallbackB) {
    if (texture) {
        graphics.drawTexture(texture, x * tileSize, y * tileSize, 
                           buildingSize * tileSize, buildingSize * tileSize);
    } else {
        graphics.setDrawColor(fallbackR, fallbackG, fallbackB, 255);
        graphics.drawRect(x * tileSize, y * tileSize, 
                        buildingSize * tileSize, buildingSize * tileSize, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(x * tileSize, y * tileSize, 
                        buildingSize * tileSize, buildingSize * tileSize, false);
    }
}

void GameState::startFadeOut(float duration, std::function<void()> onComplete) {
    isFadingOut = true;
    isFadingIn = false;
    fadeTimer = 0.0f;
    fadeDuration = duration;
    fadeOutCompleteCallback = onComplete;
}

void GameState::startFadeIn(float duration) {
    isFadingIn = true;
    isFadingOut = false;
    fadeTimer = 0.0f;
    fadeDuration = duration;
    fadeOutCompleteCallback = nullptr;
}

void GameState::updateFade(float deltaTime) {
    if (isFadingOut) {
        fadeTimer += deltaTime;
        if (fadeTimer >= fadeDuration) {
            // フェードアウト完了
            if (fadeOutCompleteCallback) {
                fadeOutCompleteCallback();
            }
            // isFadingOutはexit()が呼ばれるまでtrueのままにしておく（遷移前の描画で完全に黒いオーバーレイを描画するため）
        }
    } else if (isFadingIn) {
        fadeTimer += deltaTime;
        if (fadeTimer >= fadeDuration) {
            isFadingIn = false;
        }
    }
}

void GameState::renderFade(Graphics& graphics) {
    if (isFadingOut) {
        float fadeProgress = std::min(1.0f, fadeTimer / fadeDuration);
        Uint8 alpha = static_cast<Uint8>(fadeProgress * 255.0f);
        
        // アルファブレンディングを有効化して描画
        SDL_SetRenderDrawBlendMode(graphics.getRenderer(), SDL_BLENDMODE_BLEND);
        graphics.setDrawColor(0, 0, 0, alpha);
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        graphics.drawRect(0, 0, screenWidth, screenHeight, true);
        
        // フェードアウト完了後は完全に黒いオーバーレイを描画（状態遷移まで）
        if (fadeTimer >= fadeDuration) {
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(0, 0, screenWidth, screenHeight, true);
        }
    } else if (isFadingIn) {
        float fadeProgress = std::min(1.0f, fadeTimer / fadeDuration);
        Uint8 alpha = static_cast<Uint8>((1.0f - fadeProgress) * 255.0f); // 255から0へ
        
        // アルファブレンディングを有効化して描画
        SDL_SetRenderDrawBlendMode(graphics.getRenderer(), SDL_BLENDMODE_BLEND);
        graphics.setDrawColor(0, 0, 0, alpha);
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        graphics.drawRect(0, 0, screenWidth, screenHeight, true);
    }
} 