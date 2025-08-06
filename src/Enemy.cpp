#include "Enemy.h"
#include <iostream>
#include <random>

Enemy::Enemy(EnemyType type) : Character("", 0, 0, 0, 0, 1), type(type), canCastMagic(false), magicDamage(0) {
    switch (type) {
        case EnemyType::SLIME:
            name = "スライム";
            hp = maxHp = 25;
            mp = maxMp = 0;
            attack = 12; // 8 → 12に増加
            defense = 2;
            level = 1;
            goldReward = 5;
            expReward = 10;
            break;
            
        case EnemyType::GOBLIN:
            name = "ゴブリン";
            hp = maxHp = 50;
            mp = maxMp = 5;
            attack = 16; // 12 → 18に増加
            defense = 4;
            level = 2;
            goldReward = 8;
            expReward = 15;
            canCastMagic = true;
            magicDamage = 6;
            break;
            
        case EnemyType::ORC:
            name = "オーク";
            hp = maxHp = 100;
            mp = maxMp = 8;
            attack = 24; // 18 → 25に増加
            defense = 6;
            level = 5;
            goldReward = 15;
            expReward = 20;
            canCastMagic = true;
            magicDamage = 10;
            break;
            
        case EnemyType::DRAGON:
            name = "ドラゴン";
            hp = maxHp = 150;
            mp = maxMp = 20;
            attack = 42; // 25 → 35に増加
            defense = 12;
            level = 10;
            goldReward = 50;
            expReward = 25;
            canCastMagic = true;
            magicDamage = 20;
            break;
            
        // === ボス敵のステータス ===
        case EnemyType::GOBLIN_KING:
            name = "ゴブリンキング";
            hp = maxHp = 120;
            mp = maxMp = 30;
            attack = 30; // 22 → 30に増加
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
            attack = 45; // 35 → 45に増加
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
            attack = 65; // 50 → 65に増加
            defense = 25;
            level = 10;
            goldReward = 1000;
            expReward = 500;
            canCastMagic = true;
            magicDamage = 60;
            break;
            
        // === 新しいモンスタータイプ ===
        case EnemyType::SKELETON:
            name = "スケルトン";
            hp = maxHp = 200;
            mp = maxMp = 15;
            attack = 52;
            defense = 15;
            level = 15;
            goldReward = 80;
            expReward = 30;
            canCastMagic = true;
            magicDamage = 25;
            break;
            
        case EnemyType::GHOST:
            name = "ゴースト";
            hp = maxHp = 250;
            mp = maxMp = 40;
            attack = 62;
            defense = 20;
            level = 20;
            goldReward = 120;
            expReward = 35;
            canCastMagic = true;
            magicDamage = 35;
            break;
            
        case EnemyType::VAMPIRE:
            name = "ヴァンパイア";
            hp = maxHp = 300;
            mp = maxMp = 60;
            attack = 72;
            defense = 25;
            level = 25;
            goldReward = 200;
            expReward = 40;
            canCastMagic = true;
            magicDamage = 45;
            break;
            
        case EnemyType::DEMON_SOLDIER:
            name = "デーモンソルジャー";
            hp = maxHp = 350;
            attack = 82;
            defense = 30;
            level = 30;
            goldReward = 300;
            expReward = 45;
            canCastMagic = true;
            magicDamage = 55;
            break;
            
        case EnemyType::WEREWOLF:
            name = "ウェアウルフ";
            hp = maxHp = 400;
            mp = maxMp = 100;
            attack = 92;
            defense = 35;
            level = 35;
            goldReward = 400;
            expReward = 50;
            canCastMagic = true;
            magicDamage = 65;
            break;
            
        case EnemyType::MINOTAUR:
            name = "ミノタウロス";
            hp = maxHp = 450;
            mp = maxMp = 120;
            attack = 102;
            defense = 40;
            level = 40;
            goldReward = 500;
            expReward = 55;
            canCastMagic = true;
            magicDamage = 75;
            break;
            
        case EnemyType::CYCLOPS:
            name = "サイクロプス";
            hp = maxHp = 500;
            mp = maxMp = 140;
            attack = 112;
            defense = 45;
            level = 45;
            goldReward = 600;
            expReward = 60;
            canCastMagic = true;
            magicDamage = 85;
            break;
            
        case EnemyType::GARGOYLE:
            name = "ガーゴイル";
            hp = maxHp = 550;
            mp = maxMp = 160;
            attack = 122;
            defense = 50;
            level = 50;
            goldReward = 65;
            expReward = 350;
            canCastMagic = true;
            magicDamage = 95;
            break;
            
        case EnemyType::PHANTOM:
            name = "ファントム";
            hp = maxHp = 650;
            mp = maxMp = 180;
            attack = 132;
            defense = 55;
            level = 55;
            goldReward = 800;
            expReward = 70;
            canCastMagic = true;
            magicDamage = 105;
            break;
            
        case EnemyType::DARK_KNIGHT:
            name = "ダークナイト";
            hp = maxHp = 700;
            mp = maxMp = 200;
            attack = 142;
            defense = 60;
            level = 60;
            goldReward = 1000;
            expReward = 75;
            canCastMagic = true;
            magicDamage = 115;
            break;
            
        case EnemyType::ICE_GIANT:
            name = "アイスジャイアント";
            hp = maxHp = 750;
            mp = maxMp = 250;
            attack = 152;
            defense = 70;
            level = 65;
            goldReward = 1200;
            expReward = 80;
            canCastMagic = true;
            magicDamage = 130;
            break;
            
        case EnemyType::FIRE_DEMON:
            name = "ファイアデーモン";
            hp = maxHp = 800;
            mp = maxMp = 300;
            attack = 162;
            defense = 80;
            level = 70;
            goldReward = 1400;
            expReward = 85;
            canCastMagic = true;
            magicDamage = 145;
            break;
            
        case EnemyType::SHADOW_LORD:
            name = "シャドウロード";
            hp = maxHp = 850;
            mp = maxMp = 350;
            attack =172;
            defense = 90;
            level = 75;
            goldReward = 1600;
            expReward = 90;
            canCastMagic = true;
            magicDamage = 160;
            break;
            
        case EnemyType::ANCIENT_DRAGON:
            name = "エンシェントドラゴン";
            hp = maxHp = 900;
            mp = maxMp = 400;
            attack = 182;
            defense = 100;
            level = 80;
            goldReward = 2000;
            expReward = 95;
            canCastMagic = true;
            magicDamage = 180;
            break;
            
        case EnemyType::CHAOS_BEAST:
            name = "カオスビースト";
            hp = maxHp = 950;
            mp = maxMp = 500;
            attack = 192;
            defense = 120;
            level = 85;
            goldReward = 2500;
            expReward = 100;
            canCastMagic = true;
            magicDamage = 200;
            break;
            
        case EnemyType::ELDER_GOD:
            name = "エルダーゴッド";
            hp = maxHp = 999;
            mp = maxMp = 600;
            attack = 202;
            defense = 150;
            level = 90;
            goldReward = 3000;
            expReward = 100;
            canCastMagic = true;
            magicDamage = 250;
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
        // === 新しいモンスタータイプ ===
        case EnemyType::SKELETON: return "スケルトン";
        case EnemyType::GHOST: return "ゴースト";
        case EnemyType::VAMPIRE: return "ヴァンパイア";
        case EnemyType::DEMON_SOLDIER: return "デーモンソルジャー";
        case EnemyType::WEREWOLF: return "ウェアウルフ";
        case EnemyType::MINOTAUR: return "ミノタウロス";
        case EnemyType::CYCLOPS: return "サイクロプス";
        case EnemyType::GARGOYLE: return "ガーゴイル";
        case EnemyType::PHANTOM: return "ファントム";
        case EnemyType::DARK_KNIGHT: return "ダークナイト";
        case EnemyType::ICE_GIANT: return "アイスジャイアント";
        case EnemyType::FIRE_DEMON: return "ファイアデーモン";
        case EnemyType::SHADOW_LORD: return "シャドウロード";
        case EnemyType::ANCIENT_DRAGON: return "エンシェントドラゴン";
        case EnemyType::CHAOS_BEAST: return "カオスビースト";
        case EnemyType::ELDER_GOD: return "エルダーゴッド";
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
    if(playerLevel >=1 && playerLevel <=5){
        possibleEnemies.push_back(EnemyType::SLIME);
    }
    
    if (playerLevel >= 3 && playerLevel <=10) {
        possibleEnemies.push_back(EnemyType::GOBLIN);
    }
    
    if (playerLevel >= 10 && playerLevel <=15) {
        possibleEnemies.push_back(EnemyType::ORC);
    }
    
    if (playerLevel >= 15 && playerLevel <=25) {
        possibleEnemies.push_back(EnemyType::DRAGON);
    }
    
    // === 新しいモンスタータイプ ===
    if (playerLevel >= 20 && playerLevel <=30) {
        possibleEnemies.push_back(EnemyType::SKELETON);
    }
    
    if (playerLevel >= 25 && playerLevel <=35) {
        possibleEnemies.push_back(EnemyType::GHOST);
    }
    
    if (playerLevel >= 30 && playerLevel <=40) {
        possibleEnemies.push_back(EnemyType::VAMPIRE);
    }
    
    if (playerLevel >= 35 && playerLevel <=45) {
        possibleEnemies.push_back(EnemyType::DEMON_SOLDIER);
    }
    
    if (playerLevel >= 40 && playerLevel <=50) {
        possibleEnemies.push_back(EnemyType::WEREWOLF);
    }
    
    if (playerLevel >= 45 && playerLevel <=55) {
        possibleEnemies.push_back(EnemyType::MINOTAUR);
    }
    
    if (playerLevel >= 50 && playerLevel <=60) {
        possibleEnemies.push_back(EnemyType::CYCLOPS);
    }
    
    if (playerLevel >= 55 && playerLevel <=65) {
        possibleEnemies.push_back(EnemyType::GARGOYLE);
    }
    
    if (playerLevel >= 60 && playerLevel <=70) {
        possibleEnemies.push_back(EnemyType::PHANTOM);
    }
    
    if (playerLevel >= 65 && playerLevel <=75) {
        possibleEnemies.push_back(EnemyType::DARK_KNIGHT);
    }
    
    if (playerLevel >= 70 && playerLevel <=80) {
        possibleEnemies.push_back(EnemyType::ICE_GIANT);
    }
    
    if (playerLevel >= 75 && playerLevel <=85) {
        possibleEnemies.push_back(EnemyType::FIRE_DEMON);
    }
    
    if (playerLevel >= 80 && playerLevel <=90) {
        possibleEnemies.push_back(EnemyType::SHADOW_LORD);
    }
    
    if (playerLevel >= 85 && playerLevel <=95) {
        possibleEnemies.push_back(EnemyType::ANCIENT_DRAGON);
    }
    
    if (playerLevel >= 90 && playerLevel <=100) {
        possibleEnemies.push_back(EnemyType::CHAOS_BEAST);
    }
    
    if (playerLevel >= 95 && playerLevel <=100) {
        possibleEnemies.push_back(EnemyType::ELDER_GOD);
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