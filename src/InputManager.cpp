#include "InputManager.h"

InputManager::InputManager() {
    mouse = {0, 0, false, false, false, false};
    previousMouse = mouse;
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
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return InputKey::ENTER;
        case SDLK_ESCAPE:
            return InputKey::ESCAPE;
        case SDLK_SPACE:
            return InputKey::SPACE;
        case SDLK_1:
            return InputKey::KEY_1;
        case SDLK_2:
            return InputKey::KEY_2;
        case SDLK_3:
            return InputKey::KEY_3;
        case SDLK_4:
            return InputKey::KEY_4;
        case SDLK_5:
            return InputKey::KEY_5;
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