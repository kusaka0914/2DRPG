/**
 * @file PlayerStats.h
 * @brief プレイヤーの拡張ステータス管理を担当するクラス
 * @details プレイヤーの拡張ステータス（ゴールド、メンタル、信頼度）と呪文効果を管理する。
 * 単一責任の原則に従い、ステータス管理をPlayerクラスから分離している。
 */

#pragma once

/**
 * @brief プレイヤーの拡張ステータス管理を担当するクラス
 * @details プレイヤーの拡張ステータス（ゴールド、メンタル、信頼度）と呪文効果を管理する。
 * 単一責任の原則に従い、ステータス管理をPlayerクラスから分離している。
 */
class PlayerStats {
public:
    /**
     * @brief プレイヤーの拡張ステータスを格納する構造体
     * @details ゴールド、メンタル、魔王・王様からの信頼度を保持する。
     */
    struct ExtendedStats {
        int gold;
        int mental;  /**< @brief メンタル（範囲: 0-100） */
        int demonTrust;  /**< @brief 魔王からの信頼度（範囲: 0-100） */
        int kingTrust;  /**< @brief 王様からの信頼度（範囲: 0-100） */
    };
    
    /**
     * @brief 呪文効果を格納する構造体
     * @details カウンター効果、次のターンボーナスなどの呪文による効果を保持する。
     */
    struct SpellEffects {
        bool hasCounterEffect;  /**< @brief イチカバチーカの効果が有効か */
        bool hasNextTurnBonus;
        float nextTurnMultiplier;
        int nextTurnBonusTurns;
    };

private:
    ExtendedStats extendedStats;
    SpellEffects spellEffects;

public:
    /**
     * @brief コンストラクタ
     */
    PlayerStats();
    
    // 拡張ステータス
    ExtendedStats& getExtendedStats() { return extendedStats; }
    const ExtendedStats& getExtendedStats() const { return extendedStats; }
    
    // 呪文効果
    SpellEffects& getSpellEffects() { return spellEffects; }
    const SpellEffects& getSpellEffects() const { return spellEffects; }
    
    /**
     * @brief ゴールド変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeGold(int amount);
    
    /**
     * @brief メンタル変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeMental(int amount);
    
    /**
     * @brief 魔王からの信頼度変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeDemonTrust(int amount);
    
    /**
     * @brief 王様からの信頼度変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeKingTrust(int amount);
    
    /**
     * @brief カウンター効果の設定
     * @param active カウンター効果を有効にするか
     */
    void setCounterEffect(bool active) { spellEffects.hasCounterEffect = active; }
    
    /**
     * @brief 次のターンボーナスの設定
     * @param active ボーナスを有効にするか
     * @param multiplier 攻撃倍率（デフォルト: 1.0f）
     * @param turns ボーナス効果の残りターン数（デフォルト: 1）
     */
    void setNextTurnBonus(bool active, float multiplier = 1.0f, int turns = 1);
    
    /**
     * @brief 次のターンボーナスの処理
     */
    void processNextTurnBonus();
    
    /**
     * @brief 次のターンボーナスのクリア
     */
    void clearNextTurnBonus();
};

