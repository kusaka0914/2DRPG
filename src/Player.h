#pragma once
#include "Character.h"
#include "Inventory.h"
#include "Equipment.h"
#include <map>

enum class SpellType {
    HEAL,
    FIREBALL,
    LIGHTNING,
    POISON_DART  // 毒の針
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

    // 信頼度システム（新しいストーリー用）
    int trustLevel; // 王様からの信頼度 (0-100)
    bool isEvil; // 魔王の手下かどうか
    int evilActions; // 悪行の回数
    int goodActions; // 善行の回数
    
    // 夜間システム
    bool isNightTime; // 夜間かどうか

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
    
    // ストーリーシステム
    std::vector<std::string> getOpeningStory() const;
    std::vector<std::string> getLevelUpStory(int newLevel) const;
    bool hasLevelUpStory() const { return hasLevelUpStoryToShow; }
    int getLevelUpStoryLevel() const { return levelUpStoryLevel; }
    void clearLevelUpStoryFlag() { hasLevelUpStoryToShow = false; }

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
}; 