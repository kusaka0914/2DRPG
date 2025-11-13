#pragma once
#include "Character.h"
#include <vector>

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
    DEMON_LORD      // 魔王（最終ボス）
};

class Enemy : public Character {
private:
    EnemyType type;
    int goldReward;
    int expReward;
    bool canCastMagic;
    int magicDamage;

public:
    Enemy(EnemyType type);
    
    // オーバーライド
    void levelUp() override;
    void displayInfo() const override;
    
    // 敵専用メソッド
    int performAction(Character& target);
    void castMagic(Character& target);
    
    // ゲッター
    int getGoldReward() const { return goldReward; }
    int getExpReward() const { return expReward; }
    EnemyType getType() const { return type; }
    std::string getTypeName() const;
    bool isBoss() const;  // ボス判定用メソッド追加
    
    // 静的メソッド
    static Enemy createRandomEnemy(int playerLevel);
    static Enemy createBossEnemy(int playerLevel);  // ボス敵生成用追加
}; 