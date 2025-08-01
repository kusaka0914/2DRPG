#pragma once
#include "Item.h"
#include <memory>

enum class WeaponType {
    COPPER_SWORD,      // どうのつるぎ
    IRON_SWORD,        // てつのつるぎ
    STEEL_SWORD,       // はがねのつるぎ
    FLAME_SWORD,       // ほのおのつるぎ
    WOODEN_STICK,      // たけのぼう
    CLUB,              // こんぼう
    IRON_SPEAR         // てつのやり
};

enum class ArmorType {
    CLOTH_ARMOR,       // ぬののふく
    LEATHER_ARMOR,     // かわのよろい
    CHAIN_ARMOR,       // くさりかたびら
    IRON_ARMOR,        // てつのよろい
    STEEL_ARMOR,       // はがねのよろい
    LEATHER_SHIELD,    // かわのたて
    IRON_SHIELD,       // てつのたて
    STEEL_SHIELD       // はがねのたて
};

enum class EquipmentSlot {
    WEAPON,
    ARMOR,
    SHIELD,
    ACCESSORY
};

class Equipment : public Item {
protected:
    int attackBonus;
    int defenseBonus;
    int hpBonus;
    int mpBonus;
    EquipmentSlot slot;

public:
    Equipment(const std::string& name, const std::string& description, int price, 
              EquipmentSlot slot, int attackBonus = 0, int defenseBonus = 0, 
              int hpBonus = 0, int mpBonus = 0);

    // ゲッター
    int getAttackBonus() const { return attackBonus; }
    int getDefenseBonus() const { return defenseBonus; }
    int getHpBonus() const { return hpBonus; }
    int getMpBonus() const { return mpBonus; }
    EquipmentSlot getSlot() const { return slot; }

    // オーバーライド
    bool use(Player* player, Character* target = nullptr) override;
    std::unique_ptr<Item> clone() const override;
};

class Weapon : public Equipment {
private:
    WeaponType weaponType;

public:
    Weapon(WeaponType type);
    WeaponType getWeaponType() const { return weaponType; }
    std::unique_ptr<Item> clone() const override;

private:
    void setupWeaponData(WeaponType type);
};

class Armor : public Equipment {
private:
    ArmorType armorType;

public:
    Armor(ArmorType type);
    ArmorType getArmorType() const { return armorType; }
    std::unique_ptr<Item> clone() const override;

private:
    void setupArmorData(ArmorType type);
};

struct EquippedItems {
    std::unique_ptr<Equipment> weapon;
    std::unique_ptr<Equipment> armor;
    std::unique_ptr<Equipment> shield;
    std::unique_ptr<Equipment> accessory;

    EquippedItems() = default;
    
    // コピー禁止、ムーブのみ許可
    EquippedItems(const EquippedItems&) = delete;
    EquippedItems& operator=(const EquippedItems&) = delete;
    EquippedItems(EquippedItems&&) = default;
    EquippedItems& operator=(EquippedItems&&) = default;
};

class EquipmentManager {
private:
    EquippedItems equipped;

public:
    EquipmentManager() = default;

    // 装備操作
    bool equipItem(std::unique_ptr<Equipment> equipment);
    std::unique_ptr<Equipment> unequipItem(EquipmentSlot slot);
    
    // 装備情報取得
    const Equipment* getEquippedItem(EquipmentSlot slot) const;
    bool hasEquippedItem(EquipmentSlot slot) const;
    
    // ボーナス計算
    int getTotalAttackBonus() const;
    int getTotalDefenseBonus() const;
    int getTotalHpBonus() const;
    int getTotalMpBonus() const;
    
    // 表示
    void displayEquipment() const;
}; 