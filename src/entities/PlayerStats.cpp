#include "PlayerStats.h"
#include <algorithm>

PlayerStats::PlayerStats()
    : extendedStats{0, 100, 50, 50},
      spellEffects{false, false, 1.0f, 0},
      enemySkillEffects{0.0f, 0, 0.0f, false, 0.0f, 0.0f, 0, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f, 0, false, false, 0, 0, 0} {
}

void PlayerStats::changeGold(int amount) {
    extendedStats.gold = std::max(0, extendedStats.gold + amount);
}

void PlayerStats::changeMental(int amount) {
    extendedStats.mental = std::max(0, std::min(100, extendedStats.mental + amount));
}

void PlayerStats::changeDemonTrust(int amount) {
    extendedStats.demonTrust = std::max(0, std::min(100, extendedStats.demonTrust + amount));
}

void PlayerStats::changeKingTrust(int amount) {
    extendedStats.kingTrust = std::max(0, std::min(100, extendedStats.kingTrust + amount));
}

void PlayerStats::setNextTurnBonus(bool active, float multiplier, int turns) {
    spellEffects.hasNextTurnBonus = active;
    spellEffects.nextTurnMultiplier = multiplier;
    spellEffects.nextTurnBonusTurns = turns;
}

void PlayerStats::processNextTurnBonus() {
    if (spellEffects.hasNextTurnBonus && spellEffects.nextTurnBonusTurns > 0) {
        spellEffects.nextTurnBonusTurns--;
        if (spellEffects.nextTurnBonusTurns <= 0) {
            clearNextTurnBonus();
        }
    }
}

void PlayerStats::clearNextTurnBonus() {
    spellEffects.hasNextTurnBonus = false;
    spellEffects.nextTurnMultiplier = 1.0f;
    spellEffects.nextTurnBonusTurns = 0;
}

void PlayerStats::resetEnemySkillEffects() {
    enemySkillEffects = {0.0f, 0, 0.0f, false, 0.0f, 0.0f, 0, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f, 0, false, false, 0, 0, 0};
}

