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
    DRAGON_LORD     // レベル8のボス（魔王）
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