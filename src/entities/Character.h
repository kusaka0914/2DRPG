#pragma once
#include <string>
#include <iostream>
#include <map>

enum class StatusEffect {
    NONE,
    POISON,
    PARALYSIS,
    SLEEP
};
#include <vector>

class Character {
protected:
    std::string name;
    int hp;
    int maxHp;
    int mp;
    int maxMp;
    int attack;
    int defense;
    int level;
    int exp;
    bool isAlive;
    
    // 状態異常管理
    std::map<StatusEffect, int> statusEffects; // 効果と残りターン数

public:
    Character(const std::string& name, int hp, int mp, int attack, int defense, int level = 1);
    virtual ~Character() = default;

    // ゲッター
    std::string getName() const { return name; }
    int getHp() const { return hp; }
    int getMaxHp() const { return maxHp; }
    int getMp() const { return mp; }
    int getMaxMp() const { return maxMp; }
    int getAttack() const { return attack; }
    int getDefense() const { return defense; }
    int getLevel() const { return level; }
    int getExp() const { return exp; }
    bool getIsAlive() const { return isAlive; }
    
    // セッター（派生クラス用）
    void setMaxHp(int newMaxHp) { maxHp = newMaxHp; }
    void setMaxMp(int newMaxMp) { maxMp = newMaxMp; }
    void setAttack(int newAttack) { attack = newAttack; }
    void setDefense(int newDefense) { defense = newDefense; }

    // 基本的なアクション
    virtual void takeDamage(int damage);
    virtual void heal(int amount);
    virtual void restoreMp(int amount);
    virtual void useMp(int amount);
    virtual void displayStatus() const;
    virtual int calculateDamage(const Character& target) const;
    
    // 実効ステータス（装備ボーナス等を含む）
    virtual int getEffectiveAttack() const;
    virtual int getEffectiveDefense() const;
    
    // 状態異常関連
    void applyStatusEffect(StatusEffect effect, int duration);
    void removeStatusEffect(StatusEffect effect);
    bool hasStatusEffect(StatusEffect effect) const;
    int getStatusEffectDuration(StatusEffect effect) const;
    void processStatusEffects(); // ターン終了時処理
    std::string getStatusEffectString() const;
    
    // 純粋仮想関数
    virtual void levelUp() = 0;
    virtual void displayInfo() const = 0;
}; 