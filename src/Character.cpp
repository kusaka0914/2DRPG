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
    
    std::cout << name << "は" << actualDamage << "のダメージを受けた！" << std::endl;
    
    if (hp <= 0) {
        hp = 0;
        isAlive = false;
        std::cout << name << "は倒れた..." << std::endl;
    }
}

void Character::heal(int amount) {
    if (!isAlive) return;
    
    hp = std::min(maxHp, hp + amount);
    std::cout << name << "のHPが" << amount << "回復した！" << std::endl;
}

void Character::restoreMp(int amount) {
    if (!isAlive) return;
    
    mp = std::min(maxMp, mp + amount);
    std::cout << name << "のMPが" << amount << "回復した！" << std::endl;
}

void Character::displayStatus() const {
    std::cout << "【" << name << "】" << std::endl;
    std::cout << "HP: " << hp << "/" << maxHp << std::endl;
    std::cout << "MP: " << mp << "/" << maxMp << std::endl;
    std::cout << "レベル: " << level << std::endl;
    std::cout << "経験値: " << exp << std::endl;
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
        std::cout << name << "は毒のダメージを" << poisonDamage << "受けた！" << std::endl;
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
                    std::cout << name << "の毒が治った！" << std::endl;
                    break;
                case StatusEffect::PARALYSIS:
                    std::cout << name << "の麻痺が治った！" << std::endl;
                    break;
                case StatusEffect::SLEEP:
                    std::cout << name << "が目を覚ました！" << std::endl;
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