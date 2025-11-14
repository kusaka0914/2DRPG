/**
 * @file GameState.h
 * @brief ゲーム状態の基底クラス
 * @details ゲームの各状態（メインメニュー、フィールド、戦闘、街など）の基底クラス。
 * 状態パターンに従い、各状態のライフサイクル（enter、update、render、exit）を管理する。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include <memory>

/**
 * @brief ゲーム状態の種類
 */
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

/**
 * @brief ゲーム状態の基底クラス
 * @details ゲームの各状態（メインメニュー、フィールド、戦闘、街など）の基底クラス。
 * 状態パターンに従い、各状態のライフサイクル（enter、update、render、exit）を管理する。
 */
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
    /**
     * @brief 状態マネージャーの設定
     * @param manager 状態マネージャーへのポインタ
     */
    void setStateManager(GameStateManager* manager) { stateManager = manager; }
    
    /**
     * @brief メッセージの表示
     * @param message 表示するメッセージ
     * @param messageBoard メッセージボードへのポインタ
     * @param isShowingMessage メッセージ表示中フラグ（参照渡し）
     */
    void showMessage(const std::string& message, Label* messageBoard, bool& isShowingMessage);
    
    /**
     * @brief メッセージのクリア
     * @param messageBoard メッセージボードへのポインタ
     * @param isShowingMessage メッセージ表示中フラグ（参照渡し）
     */
    void clearMessage(Label* messageBoard, bool& isShowingMessage);
    
    /**
     * @brief 移動処理
     * @param input 入力マネージャーへの参照
     * @param playerX プレイヤーのX座標（参照渡し）
     * @param playerY プレイヤーのY座標（参照渡し）
     * @param moveTimer 移動タイマー（参照渡し）
     * @param MOVE_DELAY 移動遅延時間（秒）
     * @param isValidPosition 位置検証関数
     * @param onMovement 移動時のコールバック関数
     */
    void handleMovement(const InputManager& input, int& playerX, int& playerY, 
                       float& moveTimer, const float MOVE_DELAY,
                       std::function<bool(int, int)> isValidPosition,
                       std::function<void(int, int)> onMovement);
    
    /**
     * @brief オブジェクトの近くにいるかどうかの判定
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param objX オブジェクトのX座標
     * @param objY オブジェクトのY座標
     * @param maxDistance 最大距離（デフォルト: 1）
     * @return オブジェクトの近くにいるか
     */
    bool isNearObject(int playerX, int playerY, int objX, int objY, int maxDistance = 1) const;
    
    /**
     * @brief 有効な位置かどうかの判定
     * @param x X座標
     * @param y Y座標
     * @param minX 最小X座標
     * @param minY 最小Y座標
     * @param maxX 最大X座標
     * @param maxY 最大Y座標
     * @return 有効な位置かどうか
     */
    bool isValidPosition(int x, int y, int minX, int minY, int maxX, int maxY) const;
    
    /**
     * @brief プレイヤーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param tileSize タイルサイズ
     * @param r 赤成分（デフォルト: 0）
     * @param g 緑成分（デフォルト: 255）
     * @param b 青成分（デフォルト: 0）
     * @param a アルファ成分（デフォルト: 255）
     */
    void drawPlayer(Graphics& graphics, int playerX, int playerY, int tileSize, 
                   Uint8 r = 0, Uint8 g = 255, Uint8 b = 0, Uint8 a = 255);
    
    /**
     * @brief 入力処理テンプレート
     * @param input 入力マネージャーへの参照
     * @param ui UIマネージャーへの参照
     * @param isShowingMessage メッセージ表示中フラグ（参照渡し）
     * @param moveTimer 移動タイマー（参照渡し）
     * @param handleMovement 移動処理関数
     * @param onInteraction 相互作用処理関数
     * @param onExit 退出処理関数
     * @param isNearExit 退出判定関数
     * @param clearMessage メッセージクリア関数
     */
    void handleInputTemplate(const InputManager& input, UIManager& ui, bool& isShowingMessage,
                           float& moveTimer, std::function<void()> handleMovement,
                           std::function<void()> onInteraction, std::function<void()> onExit,
                           std::function<bool()> isNearExit, std::function<void()> clearMessage);
    
    /**
     * @brief プレイヤーテクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     * @return プレイヤーテクスチャへのポインタ
     */
    static SDL_Texture* loadPlayerTexture(Graphics& graphics);
    
    /**
     * @brief 王様テクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     * @return 王様テクスチャへのポインタ
     */
    static SDL_Texture* loadKingTexture(Graphics& graphics);
    
    /**
     * @brief 衛兵テクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     * @return 衛兵テクスチャへのポインタ
     */
    static SDL_Texture* loadGuardTexture(Graphics& graphics);
    
    /**
     * @brief 魔王テクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     * @return 魔王テクスチャへのポインタ
     */
    static SDL_Texture* loadDemonTexture(Graphics& graphics);
    
    /**
     * @brief テクスチャを使用したプレイヤーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param playerTexture プレイヤーテクスチャへのポインタ
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param tileSize タイルサイズ
     */
    static void drawPlayerWithTexture(Graphics& graphics, SDL_Texture* playerTexture, 
                                    int playerX, int playerY, int tileSize);
    
    /**
     * @brief テクスチャを使用したキャラクターの描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param texture テクスチャへのポインタ
     * @param x X座標
     * @param y Y座標
     * @param tileSize タイルサイズ
     */
    static void drawCharacterWithTexture(Graphics& graphics, SDL_Texture* texture, 
                                       int x, int y, int tileSize);
};

/**
 * @brief ゲーム状態管理を担当するクラス
 * @details ゲーム状態の遷移、更新、描画、入力処理を管理する。
 */
class GameStateManager {
private:
    std::unique_ptr<GameState> currentState;
    std::unique_ptr<GameState> nextState;
    bool shouldChangeState;

public:
    /**
     * @brief コンストラクタ
     */
    GameStateManager();
    
    /**
     * @brief デストラクタ
     */
    ~GameStateManager();
    
    /**
     * @brief 状態の変更
     * @param newState 新しい状態へのユニークポインタ
     */
    void changeState(std::unique_ptr<GameState> newState);
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime);
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics);
    
    /**
     * @brief 入力処理
     * @param input 入力マネージャーへの参照
     */
    void handleInput(const InputManager& input);
    
    /**
     * @brief 現在の状態の取得
     * @return 現在の状態へのポインタ
     */
    GameState* getCurrentState() const { return currentState.get(); }
    
    /**
     * @brief 現在の状態タイプの取得
     * @return 現在の状態タイプ
     */
    StateType getCurrentStateType() const;
    
private:
    /**
     * @brief 状態変更の実行
     */
    void performStateChange();
}; 