/**
 * @file RoomState.h
 * @brief 自室の状態を担当するクラス
 * @details 自室での移動、ベッドでの休息、机でのセーブ、宝箱の開封などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../core/GameUtils.h"
#include <memory>

/**
 * @brief 自室の状態を担当するクラス
 * @details 自室での移動、ベッドでの休息、机でのセーブ、宝箱の開封などの機能を管理する。
 */
class RoomState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // メッセージボード用UI
    Label* messageBoard;
    Label* howtooperateBoard;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int ROOM_WIDTH = 7;   // 5 → 7に変更
    const int ROOM_HEIGHT = 5;  // 3 → 5に変更
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // 部屋のオブジェクト位置
    int bedX, bedY;      // ベッド
    int deskX, deskY;    // 机
    int chestX, chestY;  // 宝箱
    int doorX, doorY;    // 出口ドア
    
    // ゲーム状態
    bool hasOpenedChest; // 宝箱を開けたか
    bool isFirstTime;    // 初回プレイか
    bool isShowingMessage; // メッセージ表示中か
    
    // メッセージ表示用フラグ
    bool pendingWelcomeMessage; // ウェルカムメッセージを表示するか
    std::string pendingMessage; // 保留中のメッセージ
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* deskTexture;
    SDL_Texture* chestClosedTexture;
    SDL_Texture* chestOpenTexture;
    SDL_Texture* bedTexture;
    SDL_Texture* houseTileTexture;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒

public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     */
    RoomState(std::shared_ptr<Player> player);
    
    /**
     * @brief 状態に入る
     */
    void enter() override;
    
    /**
     * @brief 状態から出る
     */
    void exit() override;
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime) override;
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics) override;
    
    /**
     * @brief 入力処理
     * @param input 入力マネージャーへの参照
     */
    void handleInput(const InputManager& input) override;
    
    /**
     * @brief 状態タイプの取得
     * @return 状態タイプ（ROOM）
     */
    StateType getType() const override;
    
private:
    /**
     * @brief UIのセットアップ
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void setupUI(Graphics& graphics);
    
    /**
     * @brief 部屋のセットアップ
     */
    void setupRoom();
    
    /**
     * @brief テクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void loadTextures(Graphics& graphics);
    
    /**
     * @brief 移動処理
     * @param input 入力マネージャーへの参照
     */
    void handleMovement(const InputManager& input);
    
    /**
     * @brief 相互作用チェック
     */
    void checkInteraction();
    
    /**
     * @brief 部屋の描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawRoom(Graphics& graphics);
    
    /**
     * @brief プレイヤーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawPlayer(Graphics& graphics);
    
    /**
     * @brief 部屋のオブジェクトの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawRoomObjects(Graphics& graphics);
    
    /**
     * @brief 有効な位置かどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return 有効な位置かどうか
     */
    bool isValidPosition(int x, int y) const;
    
    /**
     * @brief オブジェクトの近くにいるかどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return オブジェクトの近くにいるか
     */
    bool isNearObject(int x, int y) const;
    
    /**
     * @brief ウェルカムメッセージの表示
     */
    void showWelcomeMessage();
    
    /**
     * @brief ベッドとの相互作用
     */
    void interactWithBed();
    
    /**
     * @brief 机との相互作用
     */
    void interactWithDesk();
    
    /**
     * @brief 宝箱との相互作用
     */
    void interactWithChest();
    
    /**
     * @brief 街への退出
     */
    void exitToTown();
    
    /**
     * @brief メッセージの表示
     * @param message 表示するメッセージ
     */
    void showMessage(const std::string& message);
    
    /**
     * @brief メッセージのクリア
     */
    void clearMessage();
}; 