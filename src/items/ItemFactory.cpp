#include "ItemFactory.h"
#include "Item.h"
#include "Equipment.h"
#include <map>

std::unique_ptr<Item> ItemFactory::createItemByName(const std::string& itemName, ItemType itemType) {
    if (itemType == ItemType::CONSUMABLE) {
        // 消費アイテム
        if (itemName == "やくそう") {
            return std::make_unique<ConsumableItem>(ConsumableType::YAKUSOU);
        } else if (itemName == "せいすい") {
            return std::make_unique<ConsumableItem>(ConsumableType::SEISUI);
        } else if (itemName == "まほうのせいすい") {
            return std::make_unique<ConsumableItem>(ConsumableType::MAHOU_SEISUI);
        } else if (itemName == "どくけしそう") {
            return std::make_unique<ConsumableItem>(ConsumableType::DOKUKESHI);
        } else if (itemName == "ちからのみず") {
            return std::make_unique<ConsumableItem>(ConsumableType::CHIKARASUI);
        } else if (itemName == "まもりのたね") {
            return std::make_unique<ConsumableItem>(ConsumableType::MAMORI_SEED);
        }
    } else if (itemType == ItemType::WEAPON) {
        // 武器
        if (itemName == "たけのぼう") {
            return std::make_unique<Weapon>(WeaponType::WOODEN_STICK);
        } else if (itemName == "どうのつるぎ") {
            return std::make_unique<Weapon>(WeaponType::COPPER_SWORD);
        } else if (itemName == "こんぼう") {
            return std::make_unique<Weapon>(WeaponType::CLUB);
        } else if (itemName == "てつのつるぎ") {
            return std::make_unique<Weapon>(WeaponType::IRON_SWORD);
        } else if (itemName == "てつのやり") {
            return std::make_unique<Weapon>(WeaponType::IRON_SPEAR);
        } else if (itemName == "はがねのつるぎ") {
            return std::make_unique<Weapon>(WeaponType::STEEL_SWORD);
        } else if (itemName == "ほのおのつるぎ") {
            return std::make_unique<Weapon>(WeaponType::FLAME_SWORD);
        }
    } else if (itemType == ItemType::ARMOR) {
        // 防具・盾
        if (itemName == "ぬののふく") {
            return std::make_unique<Armor>(ArmorType::CLOTH_ARMOR);
        } else if (itemName == "かわのよろい") {
            return std::make_unique<Armor>(ArmorType::LEATHER_ARMOR);
        } else if (itemName == "くさりかたびら") {
            return std::make_unique<Armor>(ArmorType::CHAIN_ARMOR);
        } else if (itemName == "てつのよろい") {
            return std::make_unique<Armor>(ArmorType::IRON_ARMOR);
        } else if (itemName == "はがねのよろい") {
            return std::make_unique<Armor>(ArmorType::STEEL_ARMOR);
        } else if (itemName == "かわのたて") {
            return std::make_unique<Armor>(ArmorType::LEATHER_SHIELD);
        } else if (itemName == "てつのたて") {
            return std::make_unique<Armor>(ArmorType::IRON_SHIELD);
        } else if (itemName == "はがねのたて") {
            return std::make_unique<Armor>(ArmorType::STEEL_SHIELD);
        }
    }
    
    return nullptr;
}

