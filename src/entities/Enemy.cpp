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
            expReward = 60;
            break;
            
        case EnemyType::GOBLIN:
            name = "ゴブリン";
            hp = maxHp = 50;
            attack = 50;
            defense = 6;
            level = baseLevel = 5;
            baseHp = 50;
            baseAttack = 26;
            baseDefense = 6;
            goldReward = 8;
            expReward = 100;
            break;
            
        case EnemyType::ORC:
            name = "オーク";
            hp = maxHp = 95;
            attack = 32;
            defense = 13;
            level = baseLevel = 10;
            baseHp = 95;
            baseAttack = 42;
            baseDefense = 13;
            goldReward = 15;
            expReward = 200;
            break;
            
        case EnemyType::DRAGON:
            name = "ドラゴン";
            hp = maxHp = 125;
            attack = 42;
            defense = 18;
            level = baseLevel = 15;
            baseHp = 125;
            baseAttack = 58;
            baseDefense = 18;
            goldReward = 50;
            expReward = 300;
            break;
            
        case EnemyType::SKELETON:
            name = "スケルトン";
            hp = maxHp = 160;
            attack = 74;
            defense = 23;
            level = baseLevel = 20;
            baseHp = 160;
            baseAttack = 74;
            baseDefense = 23;
            goldReward = 80;
            expReward = 400;
            break;
            
        case EnemyType::GHOST:
            name = "ゴースト";
            hp = maxHp = 190;
            attack = 90;
            defense = 28;
            level = baseLevel = 25;
            baseHp = 190;
            baseAttack = 90;
            baseDefense = 28;
            goldReward = 120;
            expReward = 500;
            break;
            
        case EnemyType::VAMPIRE:
            name = "ヴァンパイア";
            hp = maxHp = 220;
            attack = 106;
            defense = 33;
            level = baseLevel = 30;
            baseHp = 220;
            baseAttack = 106;
            baseDefense = 33;
            goldReward = 200;
            expReward = 600;
            break;
            
        case EnemyType::DEMON_SOLDIER:
            name = "デーモンソルジャー";
            hp = maxHp = 250;
            attack = 122;
            defense = 38;
            level = baseLevel = 35;
            baseHp = 250;
            baseAttack = 122;
            baseDefense = 38;
            goldReward = 300;
            expReward = 700;
            break;
            
        case EnemyType::WEREWOLF:
            name = "ウェアウルフ";
            hp = maxHp = 285;
            attack = 138;
            defense = 43;
            level = baseLevel = 40;
            baseHp = 285;
            baseAttack = 138;
            baseDefense = 43;
            goldReward = 400;
            expReward = 800;
            break;
            
        case EnemyType::MINOTAUR:
            name = "ミノタウロス";
            hp = maxHp = 315;
            attack = 154;
            defense = 48;
            level = baseLevel = 45;
            baseHp = 315;
            baseAttack = 154;
            baseDefense = 48;
            goldReward = 500;
            expReward = 900;
            break;
            
        case EnemyType::CYCLOPS:
            name = "サイクロプス";
            hp = maxHp = 345;
            attack = 170;
            defense = 53;
            level = baseLevel = 50;
            baseHp = 345;
            baseAttack = 170;
            baseDefense = 53;
            goldReward = 600;
            expReward = 1000;
            break;
            
        case EnemyType::GARGOYLE:
            name = "ガーゴイル";
            hp = maxHp = 375;
            attack = 186;
            defense = 58;
            level = baseLevel = 55;
            baseHp = 375;
            baseAttack = 186;
            baseDefense = 58;
            goldReward = 650;
            expReward = 1100;
            break;
            
        case EnemyType::PHANTOM:
            name = "ファントム";
            hp = maxHp = 405;
            attack = 202;
            defense = 63;
            level = baseLevel = 60;
            baseHp = 405;
            baseAttack = 202;
            baseDefense = 63;
            goldReward = 800;
            expReward = 1200;
            break;
            
        case EnemyType::DARK_KNIGHT:
            name = "ダークナイト";
            hp = maxHp = 435;
            attack = 218;
            defense = 68;
            level = baseLevel = 65;
            baseHp = 435;
            baseAttack = 218;
            baseDefense = 68;
            goldReward = 1000;
            expReward = 1300;
            break;
            
        case EnemyType::ICE_GIANT:
            name = "アイスジャイアント";
            hp = maxHp = 465;
            attack = 234;
            defense = 73;
            level = baseLevel = 70;
            baseHp = 465;
            baseAttack = 234;
            baseDefense = 73;
            goldReward = 1200;
            expReward = 1400;
            break;
            
        case EnemyType::FIRE_DEMON:
            name = "ファイアデーモン";
            hp = maxHp = 500;
            attack = 250;
            defense = 78;
            level = baseLevel = 75;
            baseHp = 500;
            baseAttack = 250;
            baseDefense = 78;
            goldReward = 1400;
            expReward = 1500;
            break;
            
        case EnemyType::SHADOW_LORD:
            name = "シャドウロード";
            hp = maxHp = 530;
            attack = 266;
            defense = 83;
            level = baseLevel = 80;
            baseHp = 530;
            baseAttack = 266;
            baseDefense = 83;
            goldReward = 1600;
            expReward = 1600;
            break;
            
        case EnemyType::ANCIENT_DRAGON:
            name = "エンシェントドラゴン";
            hp = maxHp = 560;
            attack = 282;
            defense = 88;
            level = baseLevel = 85;
            baseHp = 560;
            baseAttack = 282;
            baseDefense = 88;
            goldReward = 2000;
            expReward = 1700;
            break;
            
        case EnemyType::CHAOS_BEAST:
            name = "カオスビースト";
            hp = maxHp = 590;
            attack = 298;
            defense = 93;
            level = baseLevel = 90;
            baseHp = 590;
            baseAttack = 298;
            baseDefense = 93;
            goldReward = 2500;
            expReward = 1800;
            break;
            
        case EnemyType::ELDER_GOD:
            name = "エルダーゴッド";
            hp = maxHp = 620;
            attack = 314;
            defense = 98;
            level = baseLevel = 95;
            baseHp = 620;
            baseAttack = 314;
            baseDefense = 98;
            goldReward = 3000;
            expReward = 1900;
            break;
            
        case EnemyType::DEMON_LORD:
            name = "魔王";
            hp = maxHp = 700;
            attack = 330;
            defense = 105;
            level = baseLevel = 100;
            baseHp = 700;
            baseAttack = 353;
            baseDefense = 105;
            goldReward = 0;
            expReward = 0;
            break;
            
        case EnemyType::GUARD:
            name = "衛兵";
            hp = maxHp = 650;
            attack = 346;
            defense = 110;
            level = baseLevel = 105;
            baseHp = 650;
            baseAttack = 330;
            baseDefense = 110;
            goldReward = 5000;
            expReward = 0;
            break;
            
        case EnemyType::KING:
            name = "王様";
            hp = maxHp = 30;
            attack = 8;
            defense = 3;
            level = baseLevel = 1;
            baseHp = 30;
            baseAttack = 8;
            baseDefense = 3;
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
        case EnemyType::GUARD: return "衛兵";
        case EnemyType::KING: return "王様";
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
    
    // 敵の成長率（1レベルあたり：HP+5、攻撃+3、防御+1）
    // 攻撃成長率を+3にすることで、プレイヤーの防御成長（+2）を上回り、適切なダメージを確保
    int hpIncrease = levelDiff * 5;
    int attackIncrease = levelDiff * 3;
    int defenseIncrease = levelDiff * 1;
    
    // ステータスを更新
    level = newLevel;
    maxHp = baseHp + hpIncrease;
    hp = maxHp; // HPも全回復
    attack = baseAttack + attackIncrease;
    defense = baseDefense + defenseIncrease;
}

void Enemy::setLevelUnrestricted(int newLevel) {
    if (newLevel < 1) {
        newLevel = 1;
    }
    
    // 基準レベルからの差分を計算
    int levelDiff = newLevel - baseLevel;
    
    // 敵の成長率（1レベルあたり：HP+5、攻撃+3、防御+1）
    int hpIncrease = levelDiff * 5;
    int attackIncrease = levelDiff * 3;
    int defenseIncrease = levelDiff * 1;
    
    // ステータスを更新
    level = newLevel;
    maxHp = baseHp + hpIncrease;
    hp = maxHp; // HPも全回復
    attack = baseAttack + attackIncrease;
    defense = baseDefense + defenseIncrease;
}

Enemy Enemy::createTargetLevelEnemy(int targetLevel) {
    // 目標レベルに到達できる敵を選ぶ
    // 目標レベルが敵のbaseLevel以上で、maxLevel（baseLevel + 5）以下になる敵を選ぶ
    
    std::vector<EnemyType> possibleEnemies;
    
    // 各敵タイプのbaseLevelとmaxLevelを確認
    // SLIME: baseLevel=1, maxLevel=5
    if (targetLevel >= 1 && targetLevel <= 5) {
        possibleEnemies.push_back(EnemyType::SLIME);
    }
    // GOBLIN: baseLevel=5, maxLevel=10
    if (targetLevel >= 5 && targetLevel <= 10) {
        possibleEnemies.push_back(EnemyType::GOBLIN);
    }
    // ORC: baseLevel=10, maxLevel=15
    if (targetLevel >= 10 && targetLevel <= 15) {
        possibleEnemies.push_back(EnemyType::ORC);
    }
    // DRAGON: baseLevel=15, maxLevel=20
    if (targetLevel >= 15 && targetLevel <= 20) {
        possibleEnemies.push_back(EnemyType::DRAGON);
    }
    // SKELETON: baseLevel=20, maxLevel=25
    if (targetLevel >= 20 && targetLevel <= 25) {
        possibleEnemies.push_back(EnemyType::SKELETON);
    }
    // GHOST: baseLevel=25, maxLevel=30
    if (targetLevel >= 25 && targetLevel <= 30) {
        possibleEnemies.push_back(EnemyType::GHOST);
    }
    // VAMPIRE: baseLevel=30, maxLevel=35
    if (targetLevel >= 30 && targetLevel <= 35) {
        possibleEnemies.push_back(EnemyType::VAMPIRE);
    }
    // DEMON_SOLDIER: baseLevel=35, maxLevel=40
    if (targetLevel >= 35 && targetLevel <= 40) {
        possibleEnemies.push_back(EnemyType::DEMON_SOLDIER);
    }
    // WEREWOLF: baseLevel=40, maxLevel=45
    if (targetLevel >= 40 && targetLevel <= 45) {
        possibleEnemies.push_back(EnemyType::WEREWOLF);
    }
    // MINOTAUR: baseLevel=45, maxLevel=50
    if (targetLevel >= 45 && targetLevel <= 50) {
        possibleEnemies.push_back(EnemyType::MINOTAUR);
    }
    // CYCLOPS: baseLevel=50, maxLevel=55
    if (targetLevel >= 50 && targetLevel <= 55) {
        possibleEnemies.push_back(EnemyType::CYCLOPS);
    }
    // GARGOYLE: baseLevel=55, maxLevel=60
    if (targetLevel >= 55 && targetLevel <= 60) {
        possibleEnemies.push_back(EnemyType::GARGOYLE);
    }
    // PHANTOM: baseLevel=60, maxLevel=65
    if (targetLevel >= 60 && targetLevel <= 65) {
        possibleEnemies.push_back(EnemyType::PHANTOM);
    }
    // DARK_KNIGHT: baseLevel=65, maxLevel=70
    if (targetLevel >= 65 && targetLevel <= 70) {
        possibleEnemies.push_back(EnemyType::DARK_KNIGHT);
    }
    // ICE_GIANT: baseLevel=70, maxLevel=75
    if (targetLevel >= 70 && targetLevel <= 75) {
        possibleEnemies.push_back(EnemyType::ICE_GIANT);
    }
    // FIRE_DEMON: baseLevel=75, maxLevel=80
    if (targetLevel >= 75 && targetLevel <= 80) {
        possibleEnemies.push_back(EnemyType::FIRE_DEMON);
    }
    // SHADOW_LORD: baseLevel=80, maxLevel=85
    if (targetLevel >= 80 && targetLevel <= 85) {
        possibleEnemies.push_back(EnemyType::SHADOW_LORD);
    }
    // ANCIENT_DRAGON: baseLevel=85, maxLevel=90
    if (targetLevel >= 85 && targetLevel <= 90) {
        possibleEnemies.push_back(EnemyType::ANCIENT_DRAGON);
    }
    // CHAOS_BEAST: baseLevel=90, maxLevel=95
    if (targetLevel >= 90 && targetLevel <= 95) {
        possibleEnemies.push_back(EnemyType::CHAOS_BEAST);
    }
    // ELDER_GOD: baseLevel=95, maxLevel=100
    if (targetLevel >= 95 && targetLevel <= 100) {
        possibleEnemies.push_back(EnemyType::ELDER_GOD);
    }
    
    // 可能な敵が存在しない場合は、最も近い敵を選ぶ
    if (possibleEnemies.empty()) {
        // 目標レベルが100を超える場合は、ELDER_GODを選ぶ
        if (targetLevel > 100) {
            possibleEnemies.push_back(EnemyType::ELDER_GOD);
        } else {
            // 目標レベルが1未満の場合は、SLIMEを選ぶ
            possibleEnemies.push_back(EnemyType::SLIME);
        }
    }
    
    // 可能な敵の中からランダムに選ぶ（複数ある場合）
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, possibleEnemies.size() - 1);
    EnemyType selectedType = possibleEnemies[dis(gen)];
    
    // 敵を生成して目標レベルに設定
    Enemy enemy(selectedType);
    enemy.setLevel(targetLevel);
    
    return enemy;
}