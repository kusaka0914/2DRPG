#pragma once
#include "Character.h"
#include "../items/Inventory.h"
#include "../items/Equipment.h"
#include <map>

enum class SpellType {
    KIZUGAIAERU,  // キズガイエール（体力20%回復）
    ATSUIATSUI,   // アツイアツーイ（低MP攻撃呪文）
    BIRIBIRIDOKKAN, // ビリビリドッカーン（中MP攻撃呪文）
    DARKNESSIMPACT, // ダークネスインパクト（高MP攻撃呪文）
    ICHIKABACHIKA,   // イチカバチーカ（カウンター技）
    TSUGICHOTTOTSUYOI,     // ツギチョットツヨーイ（次のターン2.5倍）
    TSUGIMECHATSUYOI,   // ツギメッチャツヨーイ（次のターン4倍）
    WANCHANTAOSERU  // ワンチャンタオセール（即死技）
};

class Player : public Character {
private:
    int gold;
    std::map<SpellType, int> spells; // 呪文とそのMP消費量
    Inventory inventory;
    EquipmentManager equipmentManager;
    
    // ストーリーシステム
    bool hasLevelUpStoryToShow;
    int levelUpStoryLevel;

    // 信頼度システム
    int trustLevel; // 王様からの信頼度 (0-100)
    bool isEvil; // 魔王の手下かどうか
    int evilActions; // 悪行の回数
    int goodActions; // 善行の回数
    
    // 夜間システム
    bool isNightTime; // 夜間かどうか

    // 新しいパラメータ
    int mental;           // メンタル（0-100）
    int demonTrust;       // 魔王からの信頼（0-100）
    int kingTrust;        // 王様からの信頼（0-100）
    
    // 夜の情報
    int currentNight;     // 現在の夜の回数
    std::vector<std::pair<int, int>> killedResidents; // 倒した住民の位置
    
    // 新しい呪文の効果管理
    bool hasCounterEffect;     // イチカバチーカの効果
    bool hasNextTurnBonus;     // 次のターンの攻撃ボーナス
    float nextTurnMultiplier;  // 次のターンの攻撃倍率
    int nextTurnBonusTurns;    // ボーナス効果の残りターン数

public:
    Player(const std::string& name);
    
    // オーバーライド
    void levelUp() override;
    void displayInfo() const override;
    int getEffectiveAttack() const override;
    int getEffectiveDefense() const override;
    
    // プレイヤー専用メソッド
    void gainExp(int expGained);
    void gainGold(int goldGained);
    bool canCastSpell(SpellType spell) const;
    int castSpell(SpellType spell, Character* target = nullptr);
    void learnSpell(SpellType spell, int mpCost);
    
    // ゲッター
    int getGold() const { return gold; }
    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }
    
    // アイテム関連
    void addStartingItems();
    void showInventory() const;
    bool useItem(int itemIndex, Character* target = nullptr);
    
    // 装備関連
    EquipmentManager& getEquipmentManager() { return equipmentManager; }
    const EquipmentManager& getEquipmentManager() const { return equipmentManager; }
    void showEquipment() const;
    bool equipItem(std::unique_ptr<Equipment> equipment);
    bool unequipItem(EquipmentSlot slot);
    
    // 装備ボーナス付きステータス
    int getTotalAttack() const;
    int getTotalDefense() const;
    int getTotalMaxHp() const;
    int getTotalMaxMp() const;
    
    // 戦闘アクション
    int attack(Character& target);
    void defend();
    bool tryToEscape();
    int calculateDamageWithBonus(const Character& target) const; // 攻撃ボーナスを考慮したダメージ計算
    
    // ストーリーシステム
    std::vector<std::string> getOpeningStory() const;
    std::vector<std::string> getLevelUpStory(int newLevel) const;
    bool hasLevelUpStory() const { return hasLevelUpStoryToShow; }
    int getLevelUpStoryLevel() const { return levelUpStoryLevel; }
    void clearLevelUpStoryFlag() { hasLevelUpStoryToShow = false; }
    
    // 呪文名取得
    static std::string getSpellName(SpellType spell);
    static std::vector<SpellType> getSpellsLearnedAtLevel(int level);

    // 信頼度システムのメソッド
    int getTrustLevel() const { return trustLevel; }
    void changeTrustLevel(int amount);
    bool isEvilCharacter() const { return isEvil; }
    void performEvilAction();
    void performGoodAction();
    int getEvilActions() const { return evilActions; }
    int getGoodActions() const { return goodActions; }
    
    // 夜間システム
    bool isNightTimeMode() const { return isNightTime; }
    void setNightTime(bool night);
    void toggleNightTime();
    
    // 新しいパラメータのgetter/setter
    int getMental() const { return mental; }
    void setMental(int value) { mental = std::max(0, std::min(100, value)); }
    
    int getDemonTrust() const { return demonTrust; }
    void setDemonTrust(int value) { demonTrust = std::max(0, std::min(100, value)); }
    
    int getKingTrust() const { return kingTrust; }
    void setKingTrust(int value) { kingTrust = std::max(0, std::min(100, value)); }
    
    // パラメータ変更メソッド
    void changeMental(int amount);
    void changeDemonTrust(int amount);
    void changeKingTrust(int amount);
    
    // 夜の情報のgetter/setter
    int getCurrentNight() const { return currentNight; }
    void setCurrentNight(int night) { currentNight = night; }
    const std::vector<std::pair<int, int>>& getKilledResidents() const { return killedResidents; }
    void addKilledResident(int x, int y) { killedResidents.push_back({x, y}); }
    void clearKilledResidents() { killedResidents.clear(); }
    
    // セーブ/ロード機能
    void saveGame(const std::string& filename, float nightTimer = 0.0f, bool nightTimerActive = false);
    bool loadGame(const std::string& filename, float& nightTimer, bool& nightTimerActive);
    void autoSave();
    bool autoLoad(float& nightTimer, bool& nightTimerActive);
    
    // レベルのsetter
    void setLevel(int newLevel);
    
    // 新しい呪文の効果管理メソッド
    bool hasCounterEffectActive() const { return hasCounterEffect; }
    void setCounterEffect(bool active) { hasCounterEffect = active; }
    bool hasNextTurnBonusActive() const { return hasNextTurnBonus; }
    float getNextTurnMultiplier() const { return nextTurnMultiplier; }
    void setNextTurnBonus(bool active, float multiplier = 1.0f, int turns = 1);
    void processNextTurnBonus();
    void clearNextTurnBonus();
}; 