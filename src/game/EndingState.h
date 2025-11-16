/**
 * @file EndingState.h
 * @brief エンディングの状態を担当するクラス
 * @details エンディングメッセージの表示、スタッフロールの表示などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include <memory>

/**
 * @brief エンディングフェーズの種類
 */
enum class EndingPhase {
    ENDING_MESSAGE,  /**< @brief エンディングメッセージ表示 */
    STAFF_ROLL,      /**< @brief スタッフロール表示 */
    COMPLETE         /**< @brief 完了 */
};

/**
 * @brief エンディングの状態を担当するクラス
 * @details エンディングメッセージの表示、スタッフロールの表示などの機能を管理する。
 */
class EndingState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    EndingPhase currentPhase;
    float phaseTimer;
    float scrollOffset;
    
    // UI要素
    Label* messageLabel;
    Label* staffRollLabel;
    
    // エンディングメッセージ
    std::vector<std::string> endingMessages;
    int currentMessageIndex;
    
    // スタッフロール
    std::vector<std::string> staffRoll;
    int currentStaffIndex;
    
    // 表示設定
    const float MESSAGE_DISPLAY_TIME = 3.0f;
    const float STAFF_ROLL_SPEED = 50.0f;
    const float STAFF_ROLL_DELAY = 0.5f;

public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     */
    EndingState(std::shared_ptr<Player> player);
    
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
     * @return 状態タイプ（ENDING）
     */
    StateType getType() const override { return StateType::ENDING; }
    
private:
    /**
     * @brief UIのセットアップ
     */
    void setupUI();
    
    /**
     * @brief エンディングメッセージのセットアップ
     */
    void setupEndingMessages();
    
    /**
     * @brief スタッフロールのセットアップ
     */
    void setupStaffRoll();
    
    /**
     * @brief 現在のメッセージの表示
     */
    void showCurrentMessage();
    
    /**
     * @brief スタッフロールの表示
     */
    void showStaffRoll();
    
    /**
     * @brief 次のフェーズへ進む
     */
    void nextPhase();
};
