/**
 * @file GameOverState.h
 * @brief ゲームオーバーの状態を担当するクラス
 * @details ゲームオーバー画面の表示、ゲームオーバーの理由表示、メインメニューへの戻りなどの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include <memory>

/**
 * @brief ゲームオーバーの状態を担当するクラス
 * @details ゲームオーバー画面の表示、ゲームオーバーの理由表示、メインメニューへの戻りなどの機能を管理する。
 */
class GameOverState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    std::string gameOverReason;
    
    // 再戦用の敵情報（戦闘に敗北した場合のみ有効）
    bool hasBattleEnemyInfo;  /**< @brief 戦闘に敗北した場合の敵情報があるか */
    EnemyType battleEnemyType; /**< @brief 戦闘に敗北した場合の敵の種類 */
    int battleEnemyLevel;      /**< @brief 戦闘に敗北した場合の敵のレベル */
    
    // UI要素
    Label* titleLabel;
    Label* reasonLabel;
    Label* instruction;
    
public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     * @param reason ゲームオーバーの理由
     * @param enemyType 戦闘に敗北した場合の敵の種類（オプション）
     * @param enemyLevel 戦闘に敗北した場合の敵のレベル（オプション）
     */
    GameOverState(std::shared_ptr<Player> player, const std::string& reason, 
                  EnemyType enemyType = EnemyType::SLIME, int enemyLevel = 1);
    
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
     * @return 状態タイプ（GAME_OVER）
     */
    StateType getType() const override { return StateType::GAME_OVER; }
    
private:
    /**
     * @brief UIのセットアップ
     */
    void setupUI();
}; 