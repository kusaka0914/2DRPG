#include "Enemy.h"
#include <iostream>
#include <random>

Enemy::Enemy(EnemyType type) : Character("", 0, 0, 0, 0, 1), type(type), canCastMagic(false), magicDamage(0), residentTextureIndex(-1), residentX(-1), residentY(-1), baseLevel(1), baseHp(0), baseAttack(0), baseDefense(0) {
    switch (type) {
        case EnemyType::SLIME:
            name = "スライム";
            hp = maxHp = 35;
            attack = 13;
            defense = 4;
            level = baseLevel = 1;
            baseHp = 35;
            baseAttack = 13;
            baseDefense = 4;
            goldReward = 5;
            expReward = 30;
            break;
            
        case EnemyType::GOBLIN:
            name = "ゴブリン";
            hp = maxHp = 50;
            attack = 18;
            defense = 6;
            level = baseLevel = 5;
            baseHp = 50;
            baseAttack = 18;
            baseDefense = 6;
            goldReward = 8;
            expReward = 50;
            break;
            
        case EnemyType::ORC:
            name = "オーク";
            hp = maxHp = 95;
            attack = 32;
            defense = 13;
            level = baseLevel = 10;
            baseHp = 95;
            baseAttack = 32;
            baseDefense = 13;
            goldReward = 15;
            expReward = 100;
            break;
            
        case EnemyType::DRAGON:
            name = "ドラゴン";
            hp = maxHp = 125;
            attack = 42;
            defense = 18;
            level = baseLevel = 15;
            baseHp = 125;
            baseAttack = 42;
            baseDefense = 18;
            goldReward = 50;
            expReward = 150;
            break;
            
        case EnemyType::SKELETON:
            name = "スケルトン";
            hp = maxHp = 160;
            attack = 52;
            defense = 23;
            level = baseLevel = 20;
            baseHp = 160;
            baseAttack = 52;
            baseDefense = 23;
            goldReward = 80;
            expReward = 200;
            break;
            
        case EnemyType::GHOST:
            name = "ゴースト";
            hp = maxHp = 190;
            attack = 62;
            defense = 28;
            level = baseLevel = 25;
            baseHp = 190;
            baseAttack = 62;
            baseDefense = 28;
            goldReward = 120;
            expReward = 250;
            break;
            
        case EnemyType::VAMPIRE:
            name = "ヴァンパイア";
            hp = maxHp = 220;
            attack = 72;
            defense = 33;
            level = baseLevel = 30;
            baseHp = 220;
            baseAttack = 72;
            baseDefense = 33;
            goldReward = 200;
            expReward = 300;
            break;
            
        case EnemyType::DEMON_SOLDIER:
            name = "デーモンソルジャー";
            hp = maxHp = 250;
            attack = 82;
            defense = 38;
            level = baseLevel = 35;
            baseHp = 250;
            baseAttack = 82;
            baseDefense = 38;
            goldReward = 300;
            expReward = 350;
            break;
            
        case EnemyType::WEREWOLF:
            name = "ウェアウルフ";
            hp = maxHp = 285;
            attack = 92;
            defense = 43;
            level = baseLevel = 40;
            baseHp = 285;
            baseAttack = 92;
            baseDefense = 43;
            goldReward = 400;
            expReward = 400;
            break;
            
        case EnemyType::MINOTAUR:
            name = "ミノタウロス";
            hp = maxHp = 315;
            attack = 102;
            defense = 48;
            level = baseLevel = 45;
            baseHp = 315;
            baseAttack = 102;
            baseDefense = 48;
            goldReward = 500;
            expReward = 450;
            break;
            
        case EnemyType::CYCLOPS:
            name = "サイクロプス";
            hp = maxHp = 345;
            attack = 112;
            defense = 53;
            level = baseLevel = 50;
            baseHp = 345;
            baseAttack = 112;
            baseDefense = 53;
            goldReward = 600;
            expReward = 500;
            break;
            
        case EnemyType::GARGOYLE:
            name = "ガーゴイル";
            hp = maxHp = 375;
            attack = 122;
            defense = 58;
            level = baseLevel = 55;
            baseHp = 375;
            baseAttack = 122;
            baseDefense = 58;
            goldReward = 650;
            expReward = 550;
            break;
            
        case EnemyType::PHANTOM:
            name = "ファントム";
            hp = maxHp = 405;
            attack = 132;
            defense = 63;
            level = baseLevel = 60;
            baseHp = 405;
            baseAttack = 132;
            baseDefense = 63;
            goldReward = 800;
            expReward = 600;
            break;
            
        case EnemyType::DARK_KNIGHT:
            name = "ダークナイト";
            hp = maxHp = 435;
            attack = 142;
            defense = 68;
            level = baseLevel = 65;
            baseHp = 435;
            baseAttack = 142;
            baseDefense = 68;
            goldReward = 1000;
            expReward = 650;
            break;
            
        case EnemyType::ICE_GIANT:
            name = "アイスジャイアント";
            hp = maxHp = 465;
            attack = 152;
            defense = 73;
            level = baseLevel = 70;
            baseHp = 465;
            baseAttack = 152;
            baseDefense = 73;
            goldReward = 1200;
            expReward = 700;
            break;
            
        case EnemyType::FIRE_DEMON:
            name = "ファイアデーモン";
            hp = maxHp = 500;
            attack = 162;
            defense = 78;
            level = baseLevel = 75;
            baseHp = 500;
            baseAttack = 162;
            baseDefense = 78;
            goldReward = 1400;
            expReward = 750;
            break;
            
        case EnemyType::SHADOW_LORD:
            name = "シャドウロード";
            hp = maxHp = 530;
            attack = 172;
            defense = 83;
            level = baseLevel = 80;
            baseHp = 530;
            baseAttack = 172;
            baseDefense = 83;
            goldReward = 1600;
            expReward = 800;
            break;
            
        case EnemyType::ANCIENT_DRAGON:
            name = "エンシェントドラゴン";
            hp = maxHp = 560;
            attack = 182;
            defense = 88;
            level = baseLevel = 85;
            baseHp = 560;
            baseAttack = 182;
            baseDefense = 88;
            goldReward = 2000;
            expReward = 850;
            break;
            
        case EnemyType::CHAOS_BEAST:
            name = "カオスビースト";
            hp = maxHp = 590;
            attack = 192;
            defense = 93;
            level = baseLevel = 90;
            baseHp = 590;
            baseAttack = 192;
            baseDefense = 93;
            goldReward = 2500;
            expReward = 900;
            break;
            
        case EnemyType::ELDER_GOD:
            name = "エルダーゴッド";
            hp = maxHp = 620;
            attack = 202;
            defense = 98;
            level = baseLevel = 95;
            baseHp = 620;
            baseAttack = 202;
            baseDefense = 98;
            goldReward = 3000;
            expReward = 950;
            break;
            
        case EnemyType::DEMON_LORD:
            name = "魔王";
            hp = maxHp = 700;
            attack = 215;
            defense = 105;
            level = baseLevel = 100;
            baseHp = 700;
            baseAttack = 215;
            baseDefense = 105;
            goldReward = 0;
            expReward = 0;
            break;
    }
    
    exp = 0;
    isAlive = true;
}

void Enemy::levelUp() {}

void Enemy::displayInfo() const {
    displayStatus();
}

std::string Enemy::getTypeName() const {
    switch (type) {
        case EnemyType::SLIME: return "スライム";
        case EnemyType::GOBLIN: return "ゴブリン";
        case EnemyType::ORC: return "オーク";
        case EnemyType::DRAGON: return "ドラゴン";
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
        case EnemyType::DEMON_LORD: return "魔王";
        default: return "";
    }
}

int Enemy::performAction(Character& target) {
    if (!isAlive) return 0;
    
    int damage = calculateDamage(target);
    return damage;
}

Enemy Enemy::createRandomEnemy(int playerLevel) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::vector<EnemyType> possibleEnemies;
    
    // 基本的な敵
    if(playerLevel >=1 && playerLevel <=15){
        possibleEnemies.push_back(EnemyType::SLIME);
    }
    
    if (playerLevel >= 5 && playerLevel <=20) {
        possibleEnemies.push_back(EnemyType::GOBLIN);
    }
    
    if (playerLevel >= 10 && playerLevel <=20) {
        possibleEnemies.push_back(EnemyType::ORC);
    }
    
    if (playerLevel >= 15 && playerLevel <=30) {
        possibleEnemies.push_back(EnemyType::DRAGON);
    }
    
    if (playerLevel >= 20 && playerLevel <=40) {
        possibleEnemies.push_back(EnemyType::SKELETON);
    }
    
    if (playerLevel >= 25 && playerLevel <=45) {
        possibleEnemies.push_back(EnemyType::GHOST);
    }
    
    if (playerLevel >= 30 && playerLevel <=50) {
        possibleEnemies.push_back(EnemyType::VAMPIRE);
    }
    
    if (playerLevel >= 35 && playerLevel <=55) {
        possibleEnemies.push_back(EnemyType::DEMON_SOLDIER);
    }
    
    if (playerLevel >= 40 && playerLevel <=60) {
        possibleEnemies.push_back(EnemyType::WEREWOLF);
    }
    
    if (playerLevel >= 45 && playerLevel <=65) {
        possibleEnemies.push_back(EnemyType::MINOTAUR);
    }
    
    if (playerLevel >= 50 && playerLevel <=70) {
        possibleEnemies.push_back(EnemyType::CYCLOPS);
    }
    
    if (playerLevel >= 55 && playerLevel <=75) {
        possibleEnemies.push_back(EnemyType::GARGOYLE);
    }
    
    if (playerLevel >= 60 && playerLevel <=80) {
        possibleEnemies.push_back(EnemyType::PHANTOM);
    }
    
    if (playerLevel >= 65 && playerLevel <=85) {
        possibleEnemies.push_back(EnemyType::DARK_KNIGHT);
    }
    
    if (playerLevel >= 70 && playerLevel <=90) {
        possibleEnemies.push_back(EnemyType::ICE_GIANT);
    }
    
    if (playerLevel >= 75 && playerLevel <=95) {
        possibleEnemies.push_back(EnemyType::FIRE_DEMON);
    }
    
    if (playerLevel >= 80 && playerLevel <=100) {
        possibleEnemies.push_back(EnemyType::SHADOW_LORD);
    }
    
    if (playerLevel >= 85 && playerLevel <=100) {
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

void Enemy::setLevel(int newLevel) {
    if (newLevel < 1) {
        newLevel = 1;
    }
    
    // 基本レベル+5を上限とする（スライムは特別に上限5）
    int maxLevel;
    if (type == EnemyType::SLIME) {
        maxLevel = 5;
    } else {
        maxLevel = baseLevel + 5;
    }
    if (newLevel > maxLevel) {
        newLevel = maxLevel;
    }
    
    // 基準レベルからの差分を計算
    int levelDiff = newLevel - baseLevel;
    
    // プレイヤーと同じ成長率を適用（1レベルあたり：HP+5、攻撃+2、防御+1）
    int hpIncrease = levelDiff * 5;
    int attackIncrease = levelDiff * 2;
    int defenseIncrease = levelDiff * 1;
    
    // ステータスを更新
    level = newLevel;
    maxHp = baseHp + hpIncrease;
    hp = maxHp; // HPも全回復
    attack = baseAttack + attackIncrease;
    defense = baseDefense + defenseIncrease;
}