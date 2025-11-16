/**
 * @file InputManager.h
 * @brief 入力管理を担当するクラス
 * @details キーボード、マウス、ゲームパッドの入力を統一的に管理する。
 */

#pragma once
#include <SDL.h>
#include <unordered_map>

/**
 * @brief 入力キーの種類
 */
enum class InputKey {
    UP, DOWN, LEFT, RIGHT,
    W, A, S, D,
    SPACE, ESCAPE, ENTER, Q,
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
    /**
     * @brief コンストラクタ
     */
    InputManager();
    
    /**
     * @brief デストラクタ
     */
    ~InputManager();
    
    /**
     * @brief 更新処理
     */
    void update();
    
    /**
     * @brief イベント処理
     * @param event SDLイベント
     */
    void handleEvent(const SDL_Event& event);
    
    /**
     * @brief キーが押されているか
     * @param key 入力キー
     * @return キーが押されているか
     */
    bool isKeyPressed(InputKey key) const;
    
    /**
     * @brief キーが今フレームで押されたか
     * @param key 入力キー
     * @return キーが今フレームで押されたか
     */
    bool isKeyJustPressed(InputKey key) const;
    
    /**
     * @brief キーが今フレームで離されたか
     * @param key 入力キー
     * @return キーが今フレームで離されたか
     */
    bool isKeyJustReleased(InputKey key) const;
    
    /**
     * @brief マウス状態の取得
     * @return マウス状態への参照
     */
    const MouseState& getMouseState() const { return mouse; }
    
    /**
     * @brief マウスがクリックされたか
     * @return マウスがクリックされたか
     */
    bool isMouseClicked() const { return mouse.leftClicked; }
    
    /**
     * @brief マウスが押されているか
     * @return マウスが押されているか
     */
    bool isMousePressed() const { return mouse.leftPressed; }
    
    /**
     * @brief ゲームコントローラーが接続されているか
     * @return ゲームコントローラーが接続されているか
     */
    bool isGameControllerConnected() const { return gameControllerConnected; }
    
    /**
     * @brief 左スティックのX軸の取得
     * @return X軸の値（-1.0～1.0）
     */
    float getLeftStickX() const;
    
    /**
     * @brief 左スティックのY軸の取得
     * @return Y軸の値（-1.0～1.0）
     */
    float getLeftStickY() const;
    
    /**
     * @brief 右スティックのX軸の取得
     * @return X軸の値（-1.0～1.0）
     */
    float getRightStickX() const;
    
    /**
     * @brief 右スティックのY軸の取得
     * @return Y軸の値（-1.0～1.0）
     */
    float getRightStickY() const;
}; 