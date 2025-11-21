#include "BattleLogic.h"
#include "../entities/Enemy.h"
#include <random>
#include <algorithm>

BattleLogic::BattleLogic(std::shared_ptr<Player> player, Enemy* enemy)
    : player(player), enemy(enemy), commandTurnCount(BattleConstants::NORMAL_TURN_COUNT),
      isDesperateMode(false), behaviorTypeDetermined(false) {
    playerCommands.resize(commandTurnCount);
    enemyCommands.resize(commandTurnCount);
    stats = {0, 0, false};
    determineEnemyBehaviorType();
}

int BattleLogic::judgeRound(int playerCmd, int enemyCmd) const {
    // 3すくみシステム: 攻撃 > 呪文 > 防御 > 攻撃
    
    if (playerCmd == BattleConstants::COMMAND_ATTACK && enemyCmd == BattleConstants::COMMAND_DEFEND) {
        return BattleConstants::JUDGE_RESULT_ENEMY_WIN;
    }
    if (playerCmd == BattleConstants::COMMAND_DEFEND && enemyCmd == BattleConstants::COMMAND_ATTACK) {
        return BattleConstants::JUDGE_RESULT_PLAYER_WIN;
    }
    
    if (playerCmd == BattleConstants::COMMAND_ATTACK && enemyCmd == BattleConstants::COMMAND_SPELL) {
        return BattleConstants::JUDGE_RESULT_PLAYER_WIN;
    }
    if (playerCmd == BattleConstants::COMMAND_SPELL && enemyCmd == BattleConstants::COMMAND_ATTACK) {
        return BattleConstants::JUDGE_RESULT_ENEMY_WIN;
    }
    
    if (playerCmd == BattleConstants::COMMAND_SPELL && enemyCmd == BattleConstants::COMMAND_DEFEND) {
        return BattleConstants::JUDGE_RESULT_PLAYER_WIN;
    }
    if (playerCmd == BattleConstants::COMMAND_DEFEND && enemyCmd == BattleConstants::COMMAND_SPELL) {
        return BattleConstants::JUDGE_RESULT_ENEMY_WIN;
    }
    
    // 同じコマンド同士 → 引き分け
    if (playerCmd == enemyCmd) {
        return BattleConstants::JUDGE_RESULT_DRAW;
    }
    
    return BattleConstants::JUDGE_RESULT_DRAW;
}

std::vector<BattleLogic::JudgeResult> BattleLogic::judgeAllRounds() const {
    std::vector<JudgeResult> results;
    results.reserve(commandTurnCount);
    
    for (int i = 0; i < commandTurnCount; i++) {
        JudgeResult result;
        result.playerCommand = playerCommands[i];
        result.enemyCommand = enemyCommands[i];
        result.result = judgeRound(playerCommands[i], enemyCommands[i]);
        results.push_back(result);
    }
    
    return results;
}

BattleLogic::BattleStats BattleLogic::calculateBattleStats() {
    stats = {0, 0, false};
    
    for (int i = 0; i < commandTurnCount; i++) {
        int result = judgeRound(playerCommands[i], enemyCommands[i]);
        if (result == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
            stats.playerWins++;
        } else if (result == BattleConstants::JUDGE_RESULT_ENEMY_WIN) {
            stats.enemyWins++;
        }
    }
    
    if (commandTurnCount >= 3 && stats.playerWins >= 3) {
        bool isThreeWinStreak = true;
        for (int i = 0; i < 3; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) != BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
                isThreeWinStreak = false;
                break;
            }
        }
        stats.hasThreeWinStreak = isThreeWinStreak;
    }
    
    return stats;
}

std::vector<BattleLogic::DamageInfo> BattleLogic::prepareDamageList(float damageMultiplier) {
    std::vector<DamageInfo> damages;
    BattleStats currentStats = calculateBattleStats();
    
    if (currentStats.playerWins > currentStats.enemyWins) {
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
                int cmd = playerCommands[i];
                if (cmd == BattleConstants::COMMAND_ATTACK) {
                    int baseDamage = calculatePlayerAttackDamage();
                    int damage = static_cast<int>(baseDamage * damageMultiplier);
                    damages.push_back({damage, false, false, 0, 0, BattleConstants::COMMAND_ATTACK, false, false, false, "", ""});
                } else if (cmd == BattleConstants::COMMAND_SPELL) {
                    // 呪文で勝利した場合、ダメージエントリを追加しない
                    // （後でプレイヤーが選択した呪文を実行するため）
                    // ダメージエントリは追加せず、記録だけする
                } else if (cmd == BattleConstants::COMMAND_DEFEND) {
                    // 防御で勝利した場合：カウンターラッシュ（攻撃/5のダメージ*5回）
                    int baseDamage = calculatePlayerAttackDamage();
                    int counterDamage = static_cast<int>(baseDamage / 5.0f * damageMultiplier);
                    // 5回のダメージエントリを追加
                    for (int j = 0; j < 5; j++) {
                        damages.push_back({counterDamage, false, false, 0, 0, BattleConstants::COMMAND_DEFEND, true, false, false, "", ""});
                    }
                }
            }
        }
    } else if (currentStats.enemyWins > currentStats.playerWins) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> skillDis(0, 99);
        
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == BattleConstants::JUDGE_RESULT_ENEMY_WIN) {
                int damage = calculateEnemyAttackDamage();
                bool isSpecialSkill = (skillDis(gen) < 50); // 50%の確率で特殊技
                std::string skillName = "";
                bool isSpecialSkill = false; // 専用技を無効化
                
                // if (isSpecialSkill) {
                //     // モンスタータイプに応じた特殊技名を取得
                //     skillName = getEnemySpecialSkillName(enemy->getType());
                //     // 特殊技名が取得できた場合のみ特殊技として扱う
                //     if (skillName.empty()) {
                //         isSpecialSkill = false;
                //     }
                // }
                
                damages.push_back({damage, true, false, 0, 0, -1, false, false, isSpecialSkill, skillName, ""}); // true = プレイヤーがダメージを受ける
            }
        }
    } else {
        // 引き分けの場合、全ての引き分けターンのダメージを合計して1つのエントリとして作成
        int totalPlayerDamage = 0;
        int totalEnemyDamage = 0;
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == BattleConstants::JUDGE_RESULT_DRAW) {
                totalPlayerDamage += calculateEnemyAttackDamage() / 2;
                totalEnemyDamage += calculatePlayerAttackDamage() / 2;
            }
        }
        if (totalPlayerDamage > 0 || totalEnemyDamage > 0) {
            damages.push_back({0, false, true, totalPlayerDamage, totalEnemyDamage, -1, false, false, false, "", ""}); // isDraw = true
        }
    }
    
    return damages;
}

int BattleLogic::calculatePlayerAttackDamage(float multiplier) const {
    int baseDamage = player->calculateDamageWithBonus(*enemy);
    return static_cast<int>(baseDamage * multiplier);
}

int BattleLogic::calculateEnemyAttackDamage() const {
    int damage = enemy->getAttack() - player->getTotalDefense();
    return std::max(0, damage);
}

int BattleLogic::calculateSpellDamage(int spellCommand, float multiplier) const {
    int baseDamage = player->calculateDamageWithBonus(*enemy);
    return static_cast<int>(baseDamage * 1.5f * multiplier);
}

void BattleLogic::setPlayerCommands(const std::vector<int>& commands) {
    playerCommands = commands;
    if (static_cast<int>(playerCommands.size()) < commandTurnCount) {
        playerCommands.resize(commandTurnCount);
    }
}

void BattleLogic::setEnemyCommands(const std::vector<int>& commands) {
    enemyCommands = commands;
    if (static_cast<int>(enemyCommands.size()) < commandTurnCount) {
        enemyCommands.resize(commandTurnCount);
    }
}

void BattleLogic::setCommandTurnCount(int count) {
    commandTurnCount = count;
    playerCommands.resize(commandTurnCount);
    enemyCommands.resize(commandTurnCount);
}

void BattleLogic::setDesperateMode(bool desperate) {
    isDesperateMode = desperate;
}

bool BattleLogic::checkDesperateModeCondition() const {
    float hpRatio = static_cast<float>(player->getHp()) / static_cast<float>(player->getMaxHp());
    return hpRatio <= 0.3f; // 30%以下
}

void BattleLogic::generateEnemyCommands() {
    // サイズを確実に合わせる
    if (static_cast<int>(enemyCommands.size()) != commandTurnCount) {
        enemyCommands.resize(commandTurnCount);
    }
    
    // ランダムに決定した行動タイプを使用
    // 固定の確率分布を設定（多い方から60%、30%、10%）
    int attackProb, defendProb, spellProb;
    switch (enemyBehaviorType) {
        case EnemyBehaviorType::ATTACK_TYPE:
            attackProb = 60;
            defendProb = 10;
            spellProb = 30;
            break;
        case EnemyBehaviorType::DEFEND_TYPE:
            attackProb = 30;
            defendProb = 60;
            spellProb = 10;
            break;
        case EnemyBehaviorType::SPELL_TYPE:
            attackProb = 10;
            defendProb = 30;
            spellProb = 60;
            break;
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    
    for (int i = 0; i < commandTurnCount; i++) {
        int cmd;
        int randVal = dis(gen);
        
        if (randVal < attackProb) {
            cmd = BattleConstants::COMMAND_ATTACK;
        } else if (randVal < attackProb + defendProb) {
            cmd = BattleConstants::COMMAND_DEFEND;
        } else {
            cmd = BattleConstants::COMMAND_SPELL;
        }
        
        enemyCommands[i] = cmd;
    }
}

std::string BattleLogic::getCommandName(int cmd) {
    switch (cmd) {
        case BattleConstants::COMMAND_ATTACK:
            return "攻撃";
        case BattleConstants::COMMAND_DEFEND:
            return "防御";
        case BattleConstants::COMMAND_SPELL:
            return "呪文";
        default:
            return "不明";
    }
}

bool BattleLogic::checkThreeWinStreak() const {
    if (commandTurnCount < 3) {
        return false;
    }
    
    for (int i = 0; i < 3; i++) {
        if (judgeRound(playerCommands[i], enemyCommands[i]) != BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
            return false;
        }
    }
    
    return true;
}

void BattleLogic::determineEnemyBehaviorType() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    int typeIndex = dis(gen);
    switch (typeIndex) {
        case 0:
            enemyBehaviorType = EnemyBehaviorType::ATTACK_TYPE;
            break;
        case 1:
            enemyBehaviorType = EnemyBehaviorType::DEFEND_TYPE;
            break;
        case 2:
            enemyBehaviorType = EnemyBehaviorType::SPELL_TYPE;
            break;
    }
    
    // 除外する型を決定（実際の型以外の2つからランダムに1つ選ぶ）
    std::vector<EnemyBehaviorType> allTypes = {
        EnemyBehaviorType::ATTACK_TYPE,
        EnemyBehaviorType::DEFEND_TYPE,
        EnemyBehaviorType::SPELL_TYPE
    };
    std::vector<EnemyBehaviorType> options;
    for (auto type : allTypes) {
        if (type != enemyBehaviorType) {
            options.push_back(type);
        }
    }
    
    std::uniform_int_distribution<> excludeDis(0, static_cast<int>(options.size()) - 1);
    excludedBehaviorType = options[excludeDis(gen)];
}

void BattleLogic::confirmBehaviorType() {
    behaviorTypeDetermined = true;
}

std::string BattleLogic::getBehaviorTypeName(EnemyBehaviorType type) {
    switch (type) {
        case EnemyBehaviorType::ATTACK_TYPE:
            return "攻撃型";
        case EnemyBehaviorType::DEFEND_TYPE:
            return "防御型";
        case EnemyBehaviorType::SPELL_TYPE:
            return "呪文型";
    }
    return "不明";
}

std::string BattleLogic::getBehaviorTypeHint(EnemyBehaviorType type) {
    switch (type) {
        case EnemyBehaviorType::ATTACK_TYPE:
            return "攻撃型みたいだ";
        case EnemyBehaviorType::DEFEND_TYPE:
            return "防御型みたいだ";
        case EnemyBehaviorType::SPELL_TYPE:
            return "呪文型みたいだ";
    }
    return "";
}

std::string BattleLogic::getNegativeBehaviorTypeHint(EnemyBehaviorType type) {
    switch (type) {
        case EnemyBehaviorType::ATTACK_TYPE:
            return "攻撃型ではないようだ";
        case EnemyBehaviorType::DEFEND_TYPE:
            return "防御型ではないようだ";
        case EnemyBehaviorType::SPELL_TYPE:
            return "呪文型ではないようだ";
    }
    return "";
}

std::string BattleLogic::getEnemySpecialSkillName(EnemyType enemyType) {
    switch (enemyType) {
        case EnemyType::SLIME:
            return "粘液の壁";
        case EnemyType::GOBLIN:
            return "連続突き";
        case EnemyType::ORC:
            return "怒りの一撃";
        case EnemyType::DRAGON:
            return "炎のブレス";
        case EnemyType::SKELETON:
            return "骨の壁";
        case EnemyType::GHOST:
            return "呪いの触手";
        case EnemyType::VAMPIRE:
            return "吸血";
        case EnemyType::DEMON_SOLDIER:
            return "地獄の一撃";
        case EnemyType::WEREWOLF:
            return "猛襲";
        case EnemyType::MINOTAUR:
            return "突進";
        case EnemyType::CYCLOPS:
            return "巨大な一撃";
        case EnemyType::GARGOYLE:
            return "石の守り";
        case EnemyType::PHANTOM:
            return "幻影の攻撃";
        case EnemyType::DARK_KNIGHT:
            return "暗黒の一撃";
        case EnemyType::ICE_GIANT:
            return "氷結の息吹";
        case EnemyType::FIRE_DEMON:
            return "業火";
        case EnemyType::SHADOW_LORD:
            return "影の呪縛";
        case EnemyType::ANCIENT_DRAGON:
            return "古の咆哮";
        case EnemyType::CHAOS_BEAST:
            return "混沌の力";
        case EnemyType::ELDER_GOD:
            return "神の裁き";
        case EnemyType::DEMON_LORD:
            return "魔王の力";
        case EnemyType::GOBLIN_KING:
            return "ゴブリンの怒り";
        case EnemyType::ORC_LORD:
            return "オークの猛攻";
        case EnemyType::DRAGON_LORD:
            return "ドラゴンの咆哮";
        case EnemyType::GUARD:
            return "衛兵の一撃";
        case EnemyType::KING:
            return "王の威厳";
        default:
            return "";
    }
}

