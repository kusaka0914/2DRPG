#include "BattleLogic.h"
#include <random>
#include <algorithm>

BattleLogic::BattleLogic(std::shared_ptr<Player> player, Enemy* enemy)
    : player(player), enemy(enemy), commandTurnCount(BattleConstants::NORMAL_TURN_COUNT),
      isDesperateMode(false) {
    playerCommands.resize(commandTurnCount);
    enemyCommands.resize(commandTurnCount);
    stats = {0, 0, false};
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

BattleLogic::BattleStats BattleLogic::calculateBattleStats() const {
    BattleStats stats = {0, 0, false};
    
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

std::vector<BattleLogic::DamageInfo> BattleLogic::prepareDamageList(float damageMultiplier) const {
    std::vector<DamageInfo> damages;
    BattleStats currentStats = calculateBattleStats();
    
    if (currentStats.playerWins > currentStats.enemyWins) {
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
                int cmd = playerCommands[i];
                if (cmd == BattleConstants::COMMAND_ATTACK) {
                    int baseDamage = calculatePlayerAttackDamage();
                    int damage = static_cast<int>(baseDamage * damageMultiplier);
                    damages.push_back({damage, false}); // false = 敵がダメージを受ける
                } else if (cmd == BattleConstants::COMMAND_SPELL) {
                    if (player->getMp() > 0) {
                        int baseDamage = calculateSpellDamage(cmd);
                        int damage = static_cast<int>(baseDamage * damageMultiplier);
                        damages.push_back({damage, false}); // false = 敵がダメージを受ける
                    }
                }
            }
        }
    } else if (currentStats.enemyWins > currentStats.playerWins) {
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == BattleConstants::JUDGE_RESULT_ENEMY_WIN) {
                int damage = calculateEnemyAttackDamage();
                damages.push_back({damage, true}); // true = プレイヤーがダメージを受ける
            }
        }
    } else {
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == BattleConstants::JUDGE_RESULT_DRAW) {
                int playerDamage = calculateEnemyAttackDamage() / 2;
                int enemyDamage = calculatePlayerAttackDamage() / 2;
                if (playerDamage > 0) {
                    damages.push_back({playerDamage, true}); // true = プレイヤーがダメージを受ける
                }
                if (enemyDamage > 0) {
                    damages.push_back({enemyDamage, false}); // false = 敵がダメージを受ける
                }
            }
        }
    }
    
    return damages;
}

int BattleLogic::calculatePlayerAttackDamage(float multiplier) const {
    int baseDamage = player->calculateDamageWithBonus(*enemy);
    return static_cast<int>(baseDamage * multiplier);
}

int BattleLogic::calculateEnemyAttackDamage() const {
    int damage = enemy->getAttack() - player->getDefense();
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
    float playerHpRatio = static_cast<float>(player->getHp()) / static_cast<float>(player->getMaxHp());
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    
    for (int i = 0; i < commandTurnCount; i++) {
        int cmd;
        int randVal = dis(gen);
        
        if (playerHpRatio < 0.3f) {
            if (randVal < 60) {
                cmd = BattleConstants::COMMAND_ATTACK;
            } else if (randVal < 90) {
                cmd = BattleConstants::COMMAND_DEFEND;
            } else {
                cmd = BattleConstants::COMMAND_SPELL;
            }
        } else if (playerHpRatio < 0.6f) {
            if (randVal < 40) {
                cmd = BattleConstants::COMMAND_ATTACK;
            } else if (randVal < 80) {
                cmd = BattleConstants::COMMAND_DEFEND;
            } else {
                cmd = BattleConstants::COMMAND_SPELL;
            }
        } else {
            if (randVal < 30) {
                cmd = BattleConstants::COMMAND_ATTACK;
            } else if (randVal < 80) {
                cmd = BattleConstants::COMMAND_DEFEND;
            } else {
                cmd = BattleConstants::COMMAND_SPELL;
            }
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

