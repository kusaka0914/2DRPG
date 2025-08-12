#include "Enemy.h"
#include <iostream>
#include <random>

Enemy::Enemy(EnemyType type) : Character("", 0, 0, 0, 0, 1), type(type), canCastMagic(false), magicDamage(0) {
    switch (type) {
        case EnemyType::SLIME:
            name = "スライム";
            hp = maxHp = 20;
            attack = 12;
            defense = 2;
            level = 1;
            goldReward = 5;
            expReward = 10;
            break;
            
        case EnemyType::GOBLIN:
            name = "ゴブリン";
            hp = maxHp = 40;
            attack = 21;
            defense = 4;
            level = 3;
            goldReward = 8;
            expReward = 15;
            break;
            
        case EnemyType::ORC:
            name = "オーク";
            hp = maxHp = 120;
            attack = 38;
            defense = 8;
            level = 10;
            goldReward = 15;
            expReward = 20;
            break;
            
        case EnemyType::DRAGON:
            name = "ドラゴン";
            hp = maxHp = 160;
            attack = 61;
            defense = 12;
            level = 15;
            goldReward = 50;
            expReward = 25;
            break;
            
        case EnemyType::SKELETON:
            name = "スケルトン";
            hp = maxHp = 210;
            attack = 67;    
            defense = 15;
            level = 20;
            goldReward = 80;
            expReward = 30;
            break;
            
        case EnemyType::GHOST:
            name = "ゴースト";
            hp = maxHp = 300;
            attack = 73;
            defense = 20;
            level = 25;
            goldReward = 120;
            expReward = 35;
            break;
            
        case EnemyType::VAMPIRE:
            name = "ヴァンパイア";
            hp = maxHp = 450;
            attack = 79;
            defense = 25;
            level = 30;
            goldReward = 200;
            expReward = 40;
            break;
            
        case EnemyType::DEMON_SOLDIER:
            name = "デーモンソルジャー";
            hp = maxHp = 550;
            attack = 85;
            defense = 30;
            level = 35;
            goldReward = 300;
            expReward = 45;

            break;
            
        case EnemyType::WEREWOLF:
            name = "ウェアウルフ";
            hp = maxHp = 650;
            attack = 91;
            defense = 35;
            level = 40;
            goldReward = 400;
            expReward = 50;
            break;
            
        case EnemyType::MINOTAUR:
            name = "ミノタウロス";
            hp = maxHp = 800;
            attack = 99;
            defense = 40;
            level = 45;
            goldReward = 500;
            expReward = 55;
            break;
            
        case EnemyType::CYCLOPS:
            name = "サイクロプス";
            hp = maxHp = 950;
            attack = 110;
            defense = 45;
            level = 50;
            goldReward = 600;
            expReward = 60;
            break;
            
        case EnemyType::GARGOYLE:
            name = "ガーゴイル";
            hp = maxHp = 1200;
            attack = 115;
            defense = 50;
            level = 55;
            goldReward = 650;
            expReward = 65;
            break;
            
        case EnemyType::PHANTOM:
            name = "ファントム";
            hp = maxHp = 1350;
            attack = 120;
            defense = 55;
            level = 60;
            goldReward = 800;
            expReward = 70;
            break;
            
        case EnemyType::DARK_KNIGHT:
            name = "ダークナイト";
            hp = maxHp = 1500;
            attack = 125;
            defense = 60;
            level = 65;
            goldReward = 1000;
            expReward = 75;
            break;
            
        case EnemyType::ICE_GIANT:
            name = "アイスジャイアント";
            hp = maxHp = 1650;
            attack = 130;
            defense = 70;
            level = 70;
            goldReward = 1200;
            expReward = 80;
            break;
            
        case EnemyType::FIRE_DEMON:
            name = "ファイアデーモン";
            hp = maxHp = 1800;
            attack = 135;
            defense = 80;
            level = 75;
            goldReward = 1400;
            expReward = 85;
            break;
            
        case EnemyType::SHADOW_LORD:
            name = "シャドウロード";
            hp = maxHp = 2100;
            attack = 140;
            defense = 90;
            level = 80;
            goldReward = 1600;
            expReward = 90;
            break;
            
        case EnemyType::ANCIENT_DRAGON:
            name = "エンシェントドラゴン";
            hp = maxHp = 2300;
            attack = 150;
            defense = 100;
            level = 85;
            goldReward = 2000;
            expReward = 95;
            break;
            
        case EnemyType::CHAOS_BEAST:
            name = "カオスビースト";
            hp = maxHp = 2500;
            attack = 158;
            defense = 110;
            level = 90;
            goldReward = 2500;
            expReward = 100;
            break;
            
        case EnemyType::ELDER_GOD:
            name = "エルダーゴッド";
            hp = maxHp = 2700;
            attack = 162;
            defense = 120;
            level = 95;
            goldReward = 3000;
            expReward = 100;
            break;
            
        case EnemyType::DEMON_LORD:
            name = "魔王";
            hp = maxHp = 4000;
            attack = 180;
            defense = 150;
            level = 100;
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
    
    // 通常攻撃のダメージ計算のみ（ダメージ適用はBattleStateで行う）
    int damage = calculateDamage(target);
    return damage;
}

Enemy Enemy::createRandomEnemy(int playerLevel) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // プレイヤーのレベルに応じて敵の種類を決定
    std::vector<EnemyType> possibleEnemies;
    
    // 基本的な敵
    if(playerLevel >=1 && playerLevel <=15){
        possibleEnemies.push_back(EnemyType::SLIME);
    }
    
    if (playerLevel >= 3 && playerLevel <=20) {
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