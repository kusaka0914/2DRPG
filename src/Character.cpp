#include "Character.h"
#include "Player.h"
#include <iostream>
#include <algorithm>

Character::Character(const std::string& name, int hp, int mp, int attack, int defense, int level)
    : name(name), hp(hp), maxHp(hp), mp(mp), maxMp(mp), attack(attack), defense(defense), level(level), exp(0), isAlive(true) {
}

void Character::takeDamage(int damage) {
    if (!isAlive) return;
    
    // ダメージは既にcalculateDamageで計算済みなので、そのまま適用
    int actualDamage = std::max(1, damage);
    hp -= actualDamage;
    
    if (hp <= 0) {
        hp = 0;
        isAlive = false;
    }
}

void Character::heal(int amount) {
    // isAliveがfalseでもHP回復処理を実行
    hp = std::min(maxHp, hp + amount);
    
    // HPが回復した場合は生存状態に戻す
    if (hp > 0) {
        isAlive = true;
    }
}

void Character::restoreMp(int amount) {
    // isAliveがfalseでもMP回復処理を実行
    mp = std::min(maxMp, mp + amount);
    
    // HPが0より大きい場合は生存状態に戻す
    if (hp > 0) {
        isAlive = true;
    }
}

void Character::displayStatus() const {
    
}

int Character::calculateDamage(const Character& target) const {
    // 基本ダメージ計算（攻撃力 - 相手の防御力）
    int baseDamage = getEffectiveAttack() - target.getEffectiveDefense();
    
    // プレイヤーの攻撃のみダメージを調整（敵の攻撃は調整しない）
    if (dynamic_cast<const Player*>(this) != nullptr) {
        // プレイヤーの攻撃は0.8倍に調整
        int adjustedDamage = std::max(1, static_cast<int>(baseDamage));
        return adjustedDamage;
    } else {
        // 敵の攻撃は調整なし
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
    // 毒のダメージ処理
    if (hasStatusEffect(StatusEffect::POISON)) {
        int poisonDamage = maxHp / 8; // 最大HPの1/8のダメージ
        poisonDamage = std::max(1, poisonDamage); // 最低1ダメージ
        takeDamage(poisonDamage);
    }
    
    // ターン数を減らす
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