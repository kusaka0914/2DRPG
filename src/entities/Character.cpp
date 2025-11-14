#include "Character.h"
#include "Player.h"
#include <iostream>
#include <algorithm>

Character::Character(const std::string& name, int hp, int mp, int attack, int defense, int level)
    : name(name), hp(hp), maxHp(hp), mp(mp), maxMp(mp), attack(attack), defense(defense), level(level), exp(0), isAlive(true) {
}

void Character::takeDamage(int damage) {
    if (!isAlive) return;
    
    int actualDamage = std::max(1, damage);
    hp -= actualDamage;
    
    if (hp <= 0) {
        hp = 0;
        isAlive = false;
    }
}

void Character::heal(int amount) {
    hp = std::min(maxHp, hp + amount);
    
    if (hp > 0) {
        isAlive = true;
    }
}

void Character::restoreMp(int amount) {
    mp = std::min(maxMp, mp + amount);
    
    if (hp > 0) {
        isAlive = true;
    }
}

void Character::useMp(int amount) {
    mp = std::max(0, mp - amount);
}

void Character::displayStatus() const {
    
}

int Character::calculateDamage(const Character& target) const {
    int baseDamage = getEffectiveAttack() - target.getEffectiveDefense();
    
    if (dynamic_cast<const Player*>(this) != nullptr) {
        int adjustedDamage = std::max(1, static_cast<int>(baseDamage));
        return adjustedDamage;
    } else {
        return std::max(1, baseDamage);
    }
}

int Character::getEffectiveAttack() const {
    return attack;
}

int Character::getEffectiveDefense() const {
    return defense;
}

void Character::applyStatusEffect(StatusEffect effect, int duration) {
    if (effect != StatusEffect::NONE) {
        statusEffects[effect] = duration;
    }
}

void Character::removeStatusEffect(StatusEffect effect) {
    statusEffects.erase(effect);
}

bool Character::hasStatusEffect(StatusEffect effect) const {
    auto it = statusEffects.find(effect);
    return it != statusEffects.end() && it->second > 0;
}

int Character::getStatusEffectDuration(StatusEffect effect) const {
    auto it = statusEffects.find(effect);
    return it != statusEffects.end() ? it->second : 0;
}

void Character::processStatusEffects() {
    if (hasStatusEffect(StatusEffect::POISON)) {
        int poisonDamage = maxHp / 8; // 最大HPの1/8のダメージ
        poisonDamage = std::max(1, poisonDamage); // 最低1ダメージ
        takeDamage(poisonDamage);
    }
    
    for (auto it = statusEffects.begin(); it != statusEffects.end();) {
        it->second--;
        if (it->second <= 0) {
            StatusEffect effect = it->first;
            it = statusEffects.erase(it);
            
            // 状態異常解除メッセージ
            switch (effect) {
                case StatusEffect::POISON:
                    break;
                case StatusEffect::PARALYSIS:
                    break;
                case StatusEffect::SLEEP:
                    
                    break;
                default:
                    break;
            }
        } else {
            ++it;
        }
    }
}

std::string Character::getStatusEffectString() const {
    std::string result = "";
    for (const auto& pair : statusEffects) {
        if (pair.second > 0) {
            switch (pair.first) {
                case StatusEffect::POISON:
                    result += "[毒] ";
                    break;
                case StatusEffect::PARALYSIS:
                    result += "[麻痺] ";
                    break;
                case StatusEffect::SLEEP:
                    result += "[眠り] ";
                    break;
                default:
                    break;
            }
        }
    }
    return result;
} 