/**
 * @file Battle.h
 * @brief 古い戦闘システムを担当するクラス
 * @details 従来のターン制戦闘システムを実装するクラス。
 * 新しい読み合いシステム（BattleState）では使用されていないが、後方互換性のため残存している。
 */

#pragma once
#include "../entities/Player.h"
#include "../entities/Enemy.h"

/**
 * @brief 戦闘結果の種類
 */
enum class BattleResult {
    PLAYER_VICTORY,  /**< @brief プレイヤー勝利 */
    PLAYER_DEFEAT,   /**< @brief プレイヤー敗北 */
    PLAYER_ESCAPED   /**< @brief プレイヤー逃走 */
};

/**
 * @brief プレイヤーの行動の種類
 */
enum class PlayerAction {
    ATTACK,  /**< @brief 攻撃 */
    MAGIC,   /**< @brief 呪文 */
    ITEM,    /**< @brief アイテム */
    DEFEND,  /**< @brief 防御 */
    ESCAPE   /**< @brief 逃走 */
};

/**
 * @brief 古い戦闘システムを担当するクラス
 * @details 従来のターン制戦闘システムを実装するクラス。
 * 新しい読み合いシステム（BattleState）では使用されていないが、後方互換性のため残存している。
 */
class Battle {
private:
    Player* player;
    Enemy* enemy;

public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへのポインタ
     * @param enemy 敵へのポインタ
     */
    Battle(Player* player, Enemy* enemy);
    
    /**
     * @brief 戦闘開始
     * @return 戦闘結果
     */
    BattleResult startBattle();
    
    /**
     * @brief 有効な入力の取得
     * @param min 最小値
     * @param max 最大値
     * @return 入力された値
     */
    int getValidInput(int min, int max) const;
    
    /**
     * @brief プレイヤーの選択を取得
     * @return プレイヤーの行動
     */
    PlayerAction getPlayerChoice() const;
    
    /**
     * @brief プレイヤーの行動を実行
     * @param action プレイヤーの行動
     */
    void executePlayerAction(PlayerAction action);
    
    /**
     * @brief 敵の行動を実行
     */
    void executeEnemyAction();
    
    /**
     * @brief 戦闘終了判定
     * @return 戦闘が終了しているか
     */
    bool isBattleOver() const;
    
    /**
     * @brief 呪文の選択
     * @return 選択された呪文の種類
     */
    SpellType chooseMagic() const;
    
    /**
     * @brief アイテムメニューの表示
     */
    void showItemMenu() const;
    
    /**
     * @brief アイテムの選択
     * @return 選択されたアイテムのインデックス
     */
    int chooseItem() const;
    
    /**
     * @brief 戦闘終了処理
     * @param result 戦闘結果
     */
    void handleBattleEnd(BattleResult result);
}; 