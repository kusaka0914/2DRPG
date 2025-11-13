#include "InputManager.h"
#include <iostream>

InputManager::InputManager() {
    mouse = {0, 0, false, false, false, false};
    previousMouse = mouse;
    
    // ゲームパッド初期化
    gameController = nullptr;
    gameControllerConnected = false;
    
    // ゲームパッドの自動検出を有効化
    SDL_GameControllerEventState(SDL_ENABLE);
    
    // 接続済みのゲームパッドを検索
    std::cout << "ゲームパッド検索中..." << std::endl;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            std::cout << "ゲームパッド " << i << " を発見: " << SDL_GameControllerNameForIndex(i) << std::endl;
            gameController = SDL_GameControllerOpen(i);
            if (gameController) {
                gameControllerConnected = true;
                std::cout << "ゲームパッド接続成功: " << SDL_GameControllerName(gameController) << std::endl;
                break;
            } else {
                std::cout << "ゲームパッド接続失敗: " << SDL_GetError() << std::endl;
            }
        }
    }
    
    if (!gameControllerConnected) {
        std::cout << "ゲームパッドが見つかりませんでした。" << std::endl;
    }
}

InputManager::~InputManager() {
    if (gameController) {
        SDL_GameControllerClose(gameController);
    }
}

void InputManager::update() {
    // 前フレームの状態を保存
    previousKeys = currentKeys;
    previousMouse = mouse;
    
    // マウスクリック状態をリセット
    mouse.leftClicked = false;
    mouse.rightClicked = false;
}

void InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            {
                InputKey key = sdlKeyToInputKey(event.key.keysym.sym);
                if (key != static_cast<InputKey>(-1)) {
                    currentKeys[key] = true;
                }
            }
            break;
            
        case SDL_KEYUP:
            {
                InputKey key = sdlKeyToInputKey(event.key.keysym.sym);
                if (key != static_cast<InputKey>(-1)) {
                    currentKeys[key] = false;
                }
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                mouse.leftPressed = true;
                mouse.leftClicked = true;
            }
            if (event.button.button == SDL_BUTTON_RIGHT) {
                mouse.rightPressed = true;
                mouse.rightClicked = true;
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                mouse.leftPressed = false;
            }
            if (event.button.button == SDL_BUTTON_RIGHT) {
                mouse.rightPressed = false;
            }
            break;
            
        case SDL_MOUSEMOTION:
            mouse.x = event.motion.x;
            mouse.y = event.motion.y;
            break;
            
        // ゲームパッド接続/切断イベント
        case SDL_CONTROLLERDEVICEADDED:
            std::cout << "ゲームパッド接続: " << SDL_GameControllerNameForIndex(event.cdevice.which) << std::endl;
            if (!gameControllerConnected) {
                gameController = SDL_GameControllerOpen(event.cdevice.which);
                if (gameController) {
                    gameControllerConnected = true;
                    std::cout << "ゲームパッド接続成功" << std::endl;
                }
            }
            break;
            
        case SDL_CONTROLLERDEVICEREMOVED:
            std::cout << "ゲームパッド切断" << std::endl;
            if (gameControllerConnected) {
                SDL_GameControllerClose(gameController);
                gameController = nullptr;
                gameControllerConnected = false;
            }
            break;
            
        // ゲームパッドボタンイベント
        case SDL_CONTROLLERBUTTONDOWN:
            {
                InputKey key = sdlGameControllerButtonToInputKey(static_cast<SDL_GameControllerButton>(event.cbutton.button));
                if (key != static_cast<InputKey>(-1)) {
                    currentKeys[key] = true;
                }
            }
            break;
            
        case SDL_CONTROLLERBUTTONUP:
            {
                InputKey key = sdlGameControllerButtonToInputKey(static_cast<SDL_GameControllerButton>(event.cbutton.button));
                if (key != static_cast<InputKey>(-1)) {
                    currentKeys[key] = false;
                }
            }
            break;
            
        // アナログスティックイベント（必要に応じて）
        case SDL_CONTROLLERAXISMOTION:
            // アナログスティックの値はgetLeftStickX/Y()で取得
            if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                float value = event.caxis.value / 32767.0f;
                if (abs(value) > 0.1f) { // デッドゾーンより大きい時のみ表示
                }
            }
            break;
    }
}

InputKey InputManager::sdlKeyToInputKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_UP:
        case SDLK_w:
            return InputKey::UP;
        case SDLK_DOWN:
        case SDLK_s:
            return InputKey::DOWN;
        case SDLK_LEFT:
        case SDLK_a:
            return InputKey::LEFT;
        case SDLK_RIGHT:
        case SDLK_d:
            return InputKey::RIGHT;
        case SDLK_ESCAPE:
            return InputKey::ESCAPE;
        case SDLK_SPACE:
            return InputKey::SPACE;
        case SDLK_n: 
            return InputKey::N;
        default:
            return static_cast<InputKey>(-1);
    }
}

InputKey InputManager::sdlGameControllerButtonToInputKey(SDL_GameControllerButton button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:
            return InputKey::GAMEPAD_A;
        case SDL_CONTROLLER_BUTTON_B:
            return InputKey::GAMEPAD_B;
        case SDL_CONTROLLER_BUTTON_X:
            return InputKey::GAMEPAD_X;
        case SDL_CONTROLLER_BUTTON_Y:
            return InputKey::GAMEPAD_Y;
        default:
            return static_cast<InputKey>(-1);
    }
}

bool InputManager::isKeyPressed(InputKey key) const {
    auto it = currentKeys.find(key);
    return it != currentKeys.end() && it->second;
}

bool InputManager::isKeyJustPressed(InputKey key) const {
    auto currentIt = currentKeys.find(key);
    auto previousIt = previousKeys.find(key);
    
    bool currentPressed = (currentIt != currentKeys.end() && currentIt->second);
    bool previousPressed = (previousIt != previousKeys.end() && previousIt->second);
    
    return currentPressed && !previousPressed;
}

bool InputManager::isKeyJustReleased(InputKey key) const {
    auto currentIt = currentKeys.find(key);
    auto previousIt = previousKeys.find(key);
    
    bool currentPressed = (currentIt != currentKeys.end() && currentIt->second);
    bool previousPressed = (previousIt != previousKeys.end() && previousIt->second);
    
    return !currentPressed && previousPressed;
} 

float InputManager::getLeftStickX() const {
    if (gameControllerConnected && gameController) {
        return SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
    }
    return 0.0f;
}

float InputManager::getLeftStickY() const {
    if (gameControllerConnected && gameController) {
        return SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;
    }
    return 0.0f;
}

float InputManager::getRightStickX() const {
    if (gameControllerConnected && gameController) {
        return SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f;
    }
    return 0.0f;
}

float InputManager::getRightStickY() const {
    if (gameControllerConnected && gameController) {
        return SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;
    }
    return 0.0f;
} 