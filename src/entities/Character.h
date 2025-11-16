/**
 * @file Character.h
 * @brief キャラクター基底クラス
 * @details プレイヤーと敵の共通機能（HP、MP、攻撃、防御、状態異常など）を提供する基底クラス。
 */

#pragma once
#include <string>
#include <iostream>
#include <map>

/**
 * @brief 状態異常の種類
 */
enum class StatusEffect {
    NONE,
    POISON,
    PARALYSIS,
    SLEEP
};
#include <vector>

/**
 * @brief キャラクター基底クラス
 * @details プレイヤーと敵の共通機能（HP、MP、攻撃、防御、状態異常など）を提供する基底クラス。
 */
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
    /**
     * @brief コンストラクタ
     * @param name キャラクター名
     * @param hp 最大HP
     * @param mp 最大MP
     * @param attack 攻撃力
     * @param defense 防御力
     * @param level レベル（デフォルト: 1）
     */
    Character(const std::string& name, int hp, int mp, int attack, int defense, int level = 1);
    
    /**
     * @brief デストラクタ
     */
    virtual ~Character() = default;

    /**
     * @brief 名前の取得
     * @return キャラクター名
     */
    std::string getName() const { return name; }
    
    /**
     * @brief 名前の設定
     * @param newName 新しい名前
     */
    void setName(const std::string& newName) { name = newName; }
    
    /**
     * @brief HPの取得
     * @return 現在のHP
     */
    int getHp() const { return hp; }
    
    /**
     * @brief 最大HPの取得
     * @return 最大HP
     */
    int getMaxHp() const { return maxHp; }
    
    /**
     * @brief MPの取得
     * @return 現在のMP
     */
    int getMp() const { return mp; }
    
    /**
     * @brief 最大MPの取得
     * @return 最大MP
     */
    int getMaxMp() const { return maxMp; }
    
    /**
     * @brief 攻撃力の取得
     * @return 攻撃力
     */
    int getAttack() const { return attack; }
    
    /**
     * @brief 防御力の取得
     * @return 防御力
     */
    int getDefense() const { return defense; }
    
    /**
     * @brief レベルの取得
     * @return レベル
     */
    int getLevel() const { return level; }
    
    /**
     * @brief 経験値の取得
     * @return 経験値
     */
    int getExp() const { return exp; }
    
    /**
     * @brief 生存状態の取得
     * @return 生存しているか
     */
    bool getIsAlive() const { return isAlive; }
    
    /**
     * @brief 最大HPの設定
     * @param newMaxHp 新しい最大HP
     */
    void setMaxHp(int newMaxHp) { maxHp = newMaxHp; }
    
    /**
     * @brief 最大MPの設定
     * @param newMaxMp 新しい最大MP
     */
    void setMaxMp(int newMaxMp) { maxMp = newMaxMp; }
    
    /**
     * @brief 攻撃力の設定
     * @param newAttack 新しい攻撃力
     */
    void setAttack(int newAttack) { attack = newAttack; }
    
    /**
     * @brief 防御力の設定
     * @param newDefense 新しい防御力
     */
    void setDefense(int newDefense) { defense = newDefense; }

    /**
     * @brief ダメージを受ける
     * @param damage 受けるダメージ
     */
    virtual void takeDamage(int damage);
    
    /**
     * @brief HP回復
     * @param amount 回復量
     */
    virtual void heal(int amount);
    
    /**
     * @brief MP回復
     * @param amount 回復量
     */
    virtual void restoreMp(int amount);
    
    /**
     * @brief MP消費
     * @param amount 消費量
     */
    virtual void useMp(int amount);
    
    /**
     * @brief ステータス表示
     */
    virtual void displayStatus() const;
    
    /**
     * @brief ダメージ計算
     * @param target ターゲットへの参照
     * @return 与えるダメージ
     */
    virtual int calculateDamage(const Character& target) const;
    
    /**
     * @brief 実効攻撃力の取得
     * @return 実効攻撃力（装備ボーナス等を含む）
     */
    virtual int getEffectiveAttack() const;
    
    /**
     * @brief 実効防御力の取得
     * @return 実効防御力（装備ボーナス等を含む）
     */
    virtual int getEffectiveDefense() const;
    
    /**
     * @brief 状態異常の適用
     * @param effect 状態異常の種類
     * @param duration 持続ターン数
     */
    void applyStatusEffect(StatusEffect effect, int duration);
    
    /**
     * @brief 状態異常の解除
     * @param effect 状態異常の種類
     */
    void removeStatusEffect(StatusEffect effect);
    
    /**
     * @brief 状態異常の有無チェック
     * @param effect 状態異常の種類
     * @return 状態異常を持っているか
     */
    bool hasStatusEffect(StatusEffect effect) const;
    
    /**
     * @brief 状態異常の持続ターン数の取得
     * @param effect 状態異常の種類
     * @return 残りターン数
     */
    int getStatusEffectDuration(StatusEffect effect) const;
    
    /**
     * @brief 状態異常の処理（ターン終了時）
     */
    void processStatusEffects();
    
    /**
     * @brief 状態異常の文字列表現の取得
     * @return 状態異常の文字列
     */
    std::string getStatusEffectString() const;
    
    /**
     * @brief レベルアップ（純粋仮想関数）
     */
    virtual void levelUp() = 0;
    
    /**
     * @brief 情報表示（純粋仮想関数）
     */
    virtual void displayInfo() const = 0;
}; 