/**
 * @file SDL2Game.h
 * @brief SDL2を使用したゲームエンジンのメインクラス
 * @details ゲームループ、イベント処理、状態管理、リソース管理などを統合的に管理する。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "GameState.h"
#include "../entities/Player.h"
#include <memory>
#include <chrono>
#include <string>

/**
 * @brief SDL2を使用したゲームエンジンのメインクラス
 * @details ゲームループ、イベント処理、状態管理、リソース管理などを統合的に管理する。
 */
class SDL2Game {
private:
    Graphics graphics;
    InputManager inputManager;
    GameStateManager stateManager;
    std::shared_ptr<Player> player;
    
    bool isRunning;
    std::chrono::high_resolution_clock::time_point lastTime;
    
    // UI設定ファイルのホットリロード用タイマー
    float uiConfigCheckTimer;
    const float UI_CONFIG_CHECK_INTERVAL = 0.1f;  // 0.1秒ごとにチェック（より頻繁にチェック）
    
    const int SCREEN_WIDTH = 1100;
    const int SCREEN_HEIGHT = 650;
    
    // デバッグモード用
    std::string debugStartState;

public:
    /**
     * @brief コンストラクタ
     */
    SDL2Game();
    
    /**
     * @brief デストラクタ
     */
    ~SDL2Game();
    
    /**
     * @brief 初期化
     * @return 初期化が成功したか
     */
    bool initialize();
    
    /**
     * @brief ゲームループの実行
     */
    void run();
    
    /**
     * @brief クリーンアップ
     */
    void cleanup();
    
    /**
     * @brief デバッグモードの開始状態を設定
     * @param state 開始状態の名前
     */
    void setDebugStartState(const std::string& state);
    
private:
    /**
     * @brief イベント処理
     */
    void handleEvents();
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime);
    
    /**
     * @brief 描画処理
     */
    void render();
    
    /**
     * @brief ゲームの初期化
     */
    void initializeGame();
    
    /**
     * @brief リソースの読み込み
     */
    void loadResources();
    
    /**
     * @brief ゲーム画像の読み込み
     */
    void loadGameImages();
    
    /**
     * @brief デルタタイムの計算
     * @return 前フレームからの経過時間（秒）
     */
    float calculateDeltaTime();
   