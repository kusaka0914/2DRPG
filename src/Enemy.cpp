#include "Enemy.h"
#include <iostream>
#include <random>

Enemy::Enemy(EnemyType type) : Character("", 0, 0, 0, 0, 1), type(type), canCastMagic(false), magicDamage(0) {
    switch (type) {
        case EnemyType::SLIME:
            name = "スライム";
            hp = maxHp = 25;
            mp = maxMp = 0;
            attack = 8;
            defense = 2;
            level = 1;
            goldReward = 5;
            expReward = 3;
            break;
            
        case EnemyType::GOBLIN:
            name = "ゴブリン";
            hp = maxHp = 35;
            mp = maxMp = 5;
            attack = 12;
            defense = 4;
            level = 2;
            goldReward = 8;
            expReward = 5;
            canCastMagic = true;
            magicDamage = 6;
            break;
            
        case EnemyType::ORC:
            name = "オーク";
            hp = maxHp = 60;
            mp = maxMp = 8;
            attack = 18;
            defense = 6;
            level = 3;
            goldReward = 15;
            expReward = 8;
            canCastMagic = true;
            magicDamage = 10;
            break;
            
        case EnemyType::DRAGON:
            name = "ドラゴン";
            hp = maxHp = 120;
            mp = maxMp = 20;
            attack = 25;
            defense = 12;
            level = 5;
            goldReward = 50;
            expReward = 20;
            canCastMagic = true;
            magicDamage = 20;
            break;
            
        // === ボス敵のステータス ===
        case EnemyType::GOBLIN_KING:
            name = "ゴブリンキング";
            hp = maxHp = 120;
            mp = maxMp = 30;
            attack = 22;
            defense = 10;
            level = 4;
            goldReward = 100;
            expReward = 50;
            canCastMagic = true;
            magicDamage = 25;
            break;
            
        case EnemyType::ORC_LORD:
            name = "オークロード";
            hp = maxHp = 200;
            mp = maxMp = 50;
            attack = 35;
            defense = 18;
            level = 6;
            goldReward = 250;
            expReward = 100;
            canCastMagic = true;
            magicDamage = 40;
            break;
            
        case EnemyType::DRAGON_LORD:
            name = "魔王ドラゴンロード";
            hp = maxHp = 350;
            mp = maxMp = 100;
            attack = 50;
            defense = 25;
            level = 10;
            goldReward = 1000;
            expReward = 500;
            canCastMagic = true;
            magicDamage = 60;
            break;
    }
    
    exp = 0;
    isAlive = true;
}

void Enemy::levelUp() {
    // 敵は基本的にレベルアップしないが、必要に応じて実装
    level++;
    maxHp += 10;
    maxMp += 2;
    attack += 2;
    defense += 1;
    hp = maxHp;
    mp = maxMp;
}

void Enemy::displayInfo() const {
    std::cout << "【敵：" << getTypeName() << "】" << std::endl;
    displayStatus();
    std::cout << "攻撃力: " << getAttack() << " 防御力: " << getDefense() << std::endl;
    if (canCastMagic) {
        std::cout << "魔法を使用可能" << std::endl;
    }
}

std::string Enemy::getTypeName() const {
    switch (type) {
        case EnemyType::SLIME: return "スライム";
        case EnemyType::GOBLIN: return "ゴブリン";
        case EnemyType::ORC: return "オーク";
        case EnemyType::DRAGON: return "ドラゴン";
        // === ボス敵の名前 ===
        case EnemyType::GOBLIN_KING: return "ゴブリンキング";
        case EnemyType::ORC_LORD: return "オークロード";
        case EnemyType::DRAGON_LORD: return "魔王ドラゴンロード";
        default: return "謎の敵";
    }
}

int Enemy::performAction(Character& target) {
    if (!isAlive) return 0;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    // 魔法を使える敵は30%の確率で魔法を使用
    if (canCastMagic && mp >= 3 && dis(gen) <= 30) {
        castMagic(target);
        return 8; // 魔法のダメージ（簡易実装）
    } else {
        // 通常攻撃のダメージ計算のみ（ダメージ適用はBattleStateで行う）
        int damage = calculateDamage(target);
        std::cout << name << "の攻撃！" << std::endl;
        return damage;
    }
}

void Enemy::castMagic(Character& target) {
    if (!canCastMagic || mp < 3) return;
    
    mp -= 3;
    std::cout << name << "は呪文を唱えた！" << std::endl;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> spellChoice(1, 2);
    
    if (spellChoice(gen) == 1) {
        // 攻撃魔法
        std::cout << "炎の玉が" << target.getName() << "に向かって飛んできた！" << std::endl;
        target.takeDamage(magicDamage);
    } else {
        // 回復魔法（自分）
        int healAmount = 8 + level;
        heal(healAmount);
        std::cout << name << "は回復魔法を使った！" << std::endl;
    }
}

Enemy Enemy::createRandomEnemy(int playerLevel) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // プレイヤーのレベルに応じて敵の種類を決定
    std::vector<EnemyType> possibleEnemies;
    
    // 基本的な敵
    possibleEnemies.push_back(EnemyType::SLIME);
    
    if (playerLevel >= 2) {
        possibleEnemies.push_back(EnemyType::GOBLIN);
    }
    
    if (playerLevel >= 3) {
        possibleEnemies.push_back(EnemyType::ORC);
    }
    
    if (playerLevel >= 4) {
        possibleEnemies.push_back(EnemyType::DRAGON);
    }
    
    std::uniform_int_distribution<> dis(0, possibleEnemies.size() - 1);
    EnemyType selectedType = possibleEnemies[dis(gen)];
    
    return Enemy(selectedType);
}

bool Enemy::isBoss() const {
    return type == EnemyType::GOBLIN_KING || 
           type == EnemyType::ORC_LORD || 
           type == EnemyType::DRAGON_LORD;
}

Enemy Enemy::createBossEnemy(int playerLevel) {
    if (playerLevel >= 8) {
        return Enemy(EnemyType::DRAGON_LORD);  // 魔王
    } else if (playerLevel >= 5) {
        return Enemy(EnemyType::ORC_LORD);     // 山のボス
    } else if (playerLevel >= 3) {
        return Enemy(EnemyType::GOBLIN_KING);  // 森のボス
    } else {
        return Enemy(EnemyType::GOBLIN);       // まだボス戦には早い
    }
} 