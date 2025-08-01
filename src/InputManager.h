#pragma once
#include <SDL.h>
#include <unordered_map>

enum class InputKey {
    UP, DOWN, LEFT, RIGHT,
    W, A, S, D,
    SPACE, ESCAPE,
    GAMEPAD_A, GAMEPAD_B, GAMEPAD_X, GAMEPAD_Y,
    N
};

struct MouseState {
    int x, y;
    bool leftPressed;
    bool rightPressed;
    bool leftClicked;
    bool rightClicked;
};

class InputManager {
private:
    std::unordered_map<InputKey, bool> currentKeys;
    std::unordered_map<InputKey, bool> previousKeys;
    MouseState mouse;
    MouseState previousMouse;
    
    // ゲームパッド関連
    SDL_GameController* gameController;
    bool gameControllerConnected;
    
    InputKey sdlKeyToInputKey(SDL_Keycode key);
    InputKey sdlGameControllerButtonToInputKey(SDL_GameControllerButton button);

public:
    InputManager();
    ~InputManager();
    
    void update();
    void handleEvent(const SDL_Event& event);
    
    // キー状態チェック
    bool isKeyPressed(InputKey key) const;
    bool isKeyJustPressed(InputKey key) const;
    bool isKeyJustReleased(InputKey key) const;
    
    // マウス状態
    const MouseState& getMouseState() const { return mouse; }
    bool isMouseClicked() const { return mouse.leftClicked; }
    bool isMousePressed() const { return mouse.leftPressed; }
    
    // ゲームパッド状態
    bool isGameControllerConnected() const { return gameControllerConnected; }
    
    // アナログスティック状態
    float getLeftStickX() const;
    float getLeftStickY() const;
    float getRightStickX() const;
    float getRightStickY() const;
}; 