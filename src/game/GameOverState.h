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
    
    // 住民戦用の情報（住民戦でゲームオーバーになった場合のみ有効）
    bool isResidentBattle;  /**< @brief 住民戦でゲームオーバーになったか */
    std::string residentName;  /**< @brief 住民の名前 */
    int residentX;  /**< @brief 住民のX座標 */
    int residentY;  /**< @brief 住民のY座標 */
    int residentTextureIndex;  /**< @brief 住民のテクスチャインデックス */
    
    // 目標レベル達成用の敵に負けた場合の情報
    bool isTargetLevelEnemy;  /**< @brief 目標レベル達成用の敵に負けたか */
    
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
     * @param isResident 住民戦でゲームオーバーになったか（オプション）
     * @param residentName 住民の名前（住民戦の場合のみ有効）
     * @param residentX 住民のX座標（住民戦の場合のみ有効）
     * @param residentY 住民のY座標（住民戦の場合のみ有効）
     * @param residentTextureIndex 住民のテクスチャインデックス（住民戦の場合のみ有効）
     * @param isTargetLevelEnemy 目標レベル達成用の敵に負けたか（オプション）
     */
    GameOverState(std::shared_ptr<Player> player, const std::string& reason, 
                  EnemyType enemyType = EnemyType::SLIME, int enemyLevel = 1,
                  bool isResident = false, const std::string& residentName = "",
                  int residentX = -1, int residentY = -1, int residentTextureIndex = -1,
                  bool isTargetLevelEnemy = false);
    
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