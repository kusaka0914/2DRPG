#pragma once
#include <SDL.h>
#include <unordered_map>

enum class InputKey {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ENTER,
    ESCAPE,
    SPACE,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5
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
    
    InputKey sdlKeyToInputKey(SDL_Keycode key);

public:
    InputManager();
    
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
}; 