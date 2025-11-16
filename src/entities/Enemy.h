/**
 * @file Enemy.h
 * @brief 敵キャラクターを担当するクラス
 * @details 敵キャラクターの種類、報酬、行動AIなどを管理する。
 * Characterクラスを継承し、敵固有の機能を提供する。
 */

#pragma once
#include "Character.h"
#include <vector>

/**
 * @brief 敵の種類
 * @details スライムから魔王まで、様々な敵の種類を定義する。
 */
enum class EnemyType {
    SLIME,
    GOBLIN,
    ORC,
    DRAGON,
    // === ボス敵追加 ===
    GOBLIN_KING,    // レベル3のボス
    ORC_LORD,       // レベル5のボス
    DRAGON_LORD,    // レベル8のボス（魔王）
    // === 新しいモンスター追加 ===
    SKELETON,       // レベル10以降
    GHOST,          // レベル15以降
    VAMPIRE,        // レベル20以降
    DEMON_SOLDIER,  // レベル25以降
    WEREWOLF,       // レベル30以降
    MINOTAUR,       // レベル35以降
    CYCLOPS,        // レベル40以降
    GARGOYLE,       // レベル45以降
    PHANTOM,        // レベル50以降
    DARK_KNIGHT,    // レベル60以降
    ICE_GIANT,      // レベル70以降
    FIRE_DEMON,     // レベル80以降
    SHADOW_LORD,    // レベル90以降
    ANCIENT_DRAGON, // レベル100以降
    CHAOS_BEAST,    // レベル150以降
    ELDER_GOD,      // レベル200以降
    DEMON_LORD      /**< @brief 魔王（最終ボス） */
};

/**
 * @brief 敵キャラクターを担当するクラス
 * @details 敵キャラクターの種類、報酬、行動AIなどを管理する。
 * Characterクラスを継承し、敵固有の機能を提供する。
 */
class Enemy : public Character {
private:
    EnemyType type;
    int goldReward;
    int expReward;
    bool canCastMagic;
    int magicDamage;
    int residentTextureIndex;  /**< @brief 住民のテクスチャインデックス（住民の場合のみ有効、-1=住民ではない） */
    int residentX;  /**< @brief 住民のX座標（住民の場合のみ有効、-1=住民ではない） */
    int residentY;  /**< @brief 住民のY座標（住民の場合のみ有効、-1=住民ではない） */

public:
    /**
     * @brief コンストラクタ
     * @param type 敵の種類
     */
    Enemy(EnemyType type);
    
    /**
     * @brief レベルアップ
     * @details Characterクラスの純粋仮想関数を実装。
     */
    void levelUp() override;
    
    /**
     * @brief 情報表示
     * @details Characterクラスの純粋仮想関数を実装。
     */
    void displayInfo() const override;
    
    /**
     * @brief 行動を実行
     * @details 敵のAIに基づいて行動（攻撃または呪文）を実行する。
     * 
     * @param target ターゲット（通常はプレイヤー）
     * @return 与えたダメージ
     */
    int performAction(Character& target);
    
    /**
     * @brief 呪文を唱える
     * @param target ターゲット（通常はプレイヤー）
     */
    void castMagic(Character& target);
    
    /**
     * @brief ゴールド報酬の取得
     * @return ゴールド報酬
     */
    int getGoldReward() const { return goldReward; }
    
    /**
     * @brief 経験値報酬の取得
     * @return 経験値報酬
     */
    int getExpReward() const { return expReward; }
    
    /**
     * @brief 敵の種類の取得
     * @return 敵の種類
     */
    EnemyType getType() const { return type; }
    
    /**
     * @brief 敵の種類名の取得
     * @return 敵の種類名の文字列
     */
    std::string getTypeName() const;
    
    /**
     * @brief ボス敵かどうかの判定
     * @return ボス敵かどうか
     */
    bool isBoss() const;
    
    /**
     * @brief ランダムな敵を生成
     * @details プレイヤーのレベルに応じたランダムな敵を生成する。
     * 
     * @param playerLevel プレイヤーのレベル
     * @return 生成された敵
     */
    static Enemy createRandomEnemy(int playerLevel);
    
    /**
     * @brief ボス敵を生成
     * @details プレイヤーのレベルに応じたボス敵を生成する。
     * 
     * @param playerLevel プレイヤーのレベル
     * @return 生成されたボス敵
     */
    static Enemy createBossEnemy(int playerLevel);
    
    /**
     * @brief 住民のテクスチャインデックスの設定
     * @param index 住民のテクスチャインデックス（0-5）
     */
    void setResidentTextureIndex(int index) { residentTextureIndex = index; }
    
    /**
     * @brief 住民のテクスチャインデックスの取得
     * @return 住民のテクスチャインデックス（-1=住民ではない）
     */
    int getResidentTextureIndex() const { return residentTextureIndex; }
    
    /**
     * @brief 住民かどうかの判定
     * @return 住民かどうか
     */
    bool isResident() const { return residentTextureIndex >= 0; }
    
    /**
     * @brief 住民の位置情報の設定
     * @param x 住民のX座標
     * @param y 住民のY座標
     */
    void setResidentPosition(int x, int y) { residentX = x; residentY = y; }
    
    /**
     * @brief 住民のX座標の取得
     * @return 住民のX座標（-1=住民ではない）
     */
    int getResidentX() const { return residentX; }
    
    /**
     * @brief 住民のY座標の取得
     * @return 住民のY座標（-1=住民ではない）
     */
    int getResidentY() const { return residentY; }
}; 