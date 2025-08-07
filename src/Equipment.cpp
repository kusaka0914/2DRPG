#include "Equipment.h"
#include "Player.h"
#include <iostream>
#include <fstream>

Equipment::Equipment(const std::string& name, const std::string& description, int price, 
                     EquipmentSlot slot, int attackBonus, int defenseBonus, 
                     int hpBonus, int mpBonus)
    : Item(name, description, price, ItemType::WEAPON, false, 1),
      attackBonus(attackBonus), defenseBonus(defenseBonus), 
      hpBonus(hpBonus), mpBonus(mpBonus), slot(slot) {
    
    // 装備タイプによってItemTypeを調整
    if (slot == EquipmentSlot::ARMOR || slot == EquipmentSlot::SHIELD) {
        type = ItemType::ARMOR;
    } else if (slot == EquipmentSlot::ACCESSORY) {
        type = ItemType::ACCESSORY;
    }
}

bool Equipment::use(Player* player, Character* /*target*/) {
    if (!player) return false;
    
    // 装備アイテムは「使用」ではなく「装備」として処理
    std::cout << name << "は装備アイテムです。装備コマンドを使用してください。" << std::endl;
    return false;
}

std::unique_ptr<Item> Equipment::clone() const {
    // 基底クラスのcloneは派生クラスで実装
    return nullptr;
}

Weapon::Weapon(WeaponType type) 
    : Equipment("", "", 0, EquipmentSlot::WEAPON), weaponType(type) {
    setupWeaponData(type);
}

void Weapon::setupWeaponData(WeaponType type) {
    switch (type) {
        case WeaponType::WOODEN_STICK:
            name = "たけのぼう";
            description = "竹でできた素朴な棒";
            price = 10;
            attackBonus = 2;
            break;
            
        case WeaponType::COPPER_SWORD:
            name = "どうのつるぎ";
            description = "銅でできた剣";
            price = 100;
            attackBonus = 5;
            break;
            
        case WeaponType::CLUB:
            name = "こんぼう";
            description = "木でできた棍棒";
            price = 60;
            attackBonus = 4;
            break;
            
        case WeaponType::IRON_SWORD:
            name = "てつのつるぎ";
            description = "鉄でできた頑丈な剣";
            price = 300;
            attackBonus = 10;
            break;
            
        case WeaponType::IRON_SPEAR:
            name = "てつのやり";
            description = "鉄でできた槍";
            price = 350;
            attackBonus = 12;
            break;
            
        case WeaponType::STEEL_SWORD:
            name = "はがねのつるぎ";
            description = "鋼でできた鋭い剣";
            price = 800;
            attackBonus = 20;
            break;
            
        case WeaponType::FLAME_SWORD:
            name = "ほのおのつるぎ";
            description = "炎の力を宿した剣";
            price = 1500;
            attackBonus = 30;
            mpBonus = 5;
            break;
    }
}

std::unique_ptr<Item> Weapon::clone() const {
    return std::make_unique<Weapon>(weaponType);
}

Armor::Armor(ArmorType type) 
    : Equipment("", "", 0, EquipmentSlot::ARMOR), armorType(type) {
    setupArmorData(type);
}

void Armor::setupArmorData(ArmorType type) {
    switch (type) {
        case ArmorType::CLOTH_ARMOR:
            name = "ぬののふく";
            description = "布でできた簡単な服";
            price = 20;
            defenseBonus = 2;
            break;
            
        case ArmorType::LEATHER_ARMOR:
            name = "かわのよろい";
            description = "革でできた軽い鎧";
            price = 80;
            defenseBonus = 4;
            break;
            
        case ArmorType::CHAIN_ARMOR:
            name = "くさりかたびら";
            description = "鎖でできた鎧";
            price = 200;
            defenseBonus = 8;
            break;
            
        case ArmorType::IRON_ARMOR:
            name = "てつのよろい";
            description = "鉄でできた頑丈な鎧";
            price = 500;
            defenseBonus = 12;
            break;
            
        case ArmorType::STEEL_ARMOR:
            name = "はがねのよろい";
            description = "鋼でできた最高級の鎧";
            price = 1200;
            defenseBonus = 20;
            hpBonus = 10;
            break;
            
        case ArmorType::LEATHER_SHIELD:
            name = "かわのたて";
            description = "革でできた軽い盾";
            price = 50;
            defenseBonus = 3;
            slot = EquipmentSlot::SHIELD;
            break;
            
        case ArmorType::IRON_SHIELD:
            name = "てつのたて";
            description = "鉄でできた頑丈な盾";
            price = 150;
            defenseBonus = 6;
            slot = EquipmentSlot::SHIELD;
            break;
            
        case ArmorType::STEEL_SHIELD:
            name = "はがねのたて";
            description = "鋼でできた最高級の盾";
            price = 400;
            defenseBonus = 10;
            hpBonus = 5;
            slot = EquipmentSlot::SHIELD;
            break;
    }
}

std::unique_ptr<Item> Armor::clone() const {
    return std::make_unique<Armor>(armorType);
}

bool EquipmentManager::equipItem(std::unique_ptr<Equipment> equipment) {
    if (!equipment) return false;
    
    EquipmentSlot slot = equipment->getSlot();
    Equipment** targetSlot = nullptr;
    
    switch (slot) {
        case EquipmentSlot::WEAPON:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.weapon);
            break;
        case EquipmentSlot::ARMOR:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.armor);
            break;
        case EquipmentSlot::SHIELD:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.shield);
            break;
        case EquipmentSlot::ACCESSORY:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.accessory);
            break;
    }
    
    if (targetSlot) {
        if (*targetSlot) {
            std::cout << (*targetSlot)->getName() << "を外した。" << std::endl;
        }
        *targetSlot = equipment.release();
        std::cout << (*targetSlot)->getName() << "を装備した！" << std::endl;
        return true;
    }
    
    return false;
}

std::unique_ptr<Equipment> EquipmentManager::unequipItem(EquipmentSlot slot) {
    Equipment** targetSlot = nullptr;
    
    switch (slot) {
        case EquipmentSlot::WEAPON:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.weapon);
            break;
        case EquipmentSlot::ARMOR:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.armor);
            break;
        case EquipmentSlot::SHIELD:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.shield);
            break;
        case EquipmentSlot::ACCESSORY:
            targetSlot = reinterpret_cast<Equipment**>(&equipped.accessory);
            break;
    }
    
    if (targetSlot && *targetSlot) {
        std::unique_ptr<Equipment> unequipped(*targetSlot);
        *targetSlot = nullptr;
        std::cout << unequipped->getName() << "を外した。" << std::endl;
        return unequipped;
    }
    
    return nullptr;
}

const Equipment* EquipmentManager::getEquippedItem(EquipmentSlot slot) const {
    switch (slot) {
        case EquipmentSlot::WEAPON:
            return equipped.weapon.get();
        case EquipmentSlot::ARMOR:
            return equipped.armor.get();
        case EquipmentSlot::SHIELD:
            return equipped.shield.get();
        case EquipmentSlot::ACCESSORY:
            return equipped.accessory.get();
    }
    return nullptr;
}

bool EquipmentManager::hasEquippedItem(EquipmentSlot slot) const {
    return getEquippedItem(slot) != nullptr;
}

int EquipmentManager::getTotalAttackBonus() const {
    int total = 0;
    if (equipped.weapon) total += equipped.weapon->getAttackBonus();
    if (equipped.armor) total += equipped.armor->getAttackBonus();
    if (equipped.shield) total += equipped.shield->getAttackBonus();
    if (equipped.accessory) total += equipped.accessory->getAttackBonus();
    return total;
}

int EquipmentManager::getTotalDefenseBonus() const {
    int total = 0;
    if (equipped.weapon) total += equipped.weapon->getDefenseBonus();
    if (equipped.armor) total += equipped.armor->getDefenseBonus();
    if (equipped.shield) total += equipped.shield->getDefenseBonus();
    if (equipped.accessory) total += equipped.accessory->getDefenseBonus();
    return total;
}

int EquipmentManager::getTotalHpBonus() const {
    int total = 0;
    if (equipped.weapon) total += equipped.weapon->getHpBonus();
    if (equipped.armor) total += equipped.armor->getHpBonus();
    if (equipped.shield) total += equipped.shield->getHpBonus();
    if (equipped.accessory) total += equipped.accessory->getHpBonus();
    return total;
}

int EquipmentManager::getTotalMpBonus() const {
    int total = 0;
    if (equipped.weapon) total += equipped.weapon->getMpBonus();
    if (equipped.armor) total += equipped.armor->getMpBonus();
    if (equipped.shield) total += equipped.shield->getMpBonus();
    if (equipped.accessory) total += equipped.accessory->getMpBonus();
    return total;
}

void EquipmentManager::displayEquipment() const {
    std::cout << "\n=== 装備 ===" << std::endl;
    
    std::cout << "武器: ";
    if (equipped.weapon) {
        std::cout << equipped.weapon->getName() << " (攻撃力+" << equipped.weapon->getAttackBonus() << ")";
    } else {
        std::cout << "なし";
    }
    std::cout << std::endl;
    
    std::cout << "防具: ";
    if (equipped.armor) {
        std::cout << equipped.armor->getName() << " (防御力+" << equipped.armor->getDefenseBonus() << ")";
    } else {
        std::cout << "なし";
    }
    std::cout << std::endl;
    
    std::cout << "盾: ";
    if (equipped.shield) {
        std::cout << equipped.shield->getName() << " (防御力+" << equipped.shield->getDefenseBonus() << ")";
    } else {
        std::cout << "なし";
    }
    std::cout << std::endl;
    
    std::cout << "アクセサリー: ";
    if (equipped.accessory) {
        std::cout << equipped.accessory->getName();
    } else {
        std::cout << "なし";
    }
    std::cout << std::endl;
    
    int totalAttack = getTotalAttackBonus();
    int totalDefense = getTotalDefenseBonus();
    int totalHp = getTotalHpBonus();
    int totalMp = getTotalMpBonus();
    
    if (totalAttack > 0 || totalDefense > 0 || totalHp > 0 || totalMp > 0) {
        std::cout << "\n装備ボーナス合計: ";
        if (totalAttack > 0) std::cout << "攻撃力+" << totalAttack << " ";
        if (totalDefense > 0) std::cout << "防御力+" << totalDefense << " ";
        if (totalHp > 0) std::cout << "HP+" << totalHp << " ";
        if (totalMp > 0) std::cout << "MP+" << totalMp << " ";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// セーブ/ロード機能の実装
void EquipmentManager::saveToFile(std::ofstream& file) {
    // 各スロットの装備情報を保存
    for (int slot = 0; slot < 4; ++slot) {
        EquipmentSlot equipmentSlot = static_cast<EquipmentSlot>(slot);
        const Equipment* equipped = getEquippedItem(equipmentSlot);
        
        bool hasEquipment = (equipped != nullptr);
        file.write(reinterpret_cast<const char*>(&hasEquipment), sizeof(hasEquipment));
        
        if (hasEquipment) {
            // 装備の種類を保存
            ItemType itemType = equipped->getType();
            file.write(reinterpret_cast<const char*>(&itemType), sizeof(itemType));
            
            // 装備名の長さと名前を保存
            std::string equipmentName = equipped->getName();
            int nameLength = equipmentName.length();
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(equipmentName.c_str(), nameLength);
        }
    }
}

void EquipmentManager::loadFromFile(std::ifstream& file) {
    // 各スロットの装備情報を読み込み
    for (int slot = 0; slot < 4; ++slot) {
        EquipmentSlot equipmentSlot = static_cast<EquipmentSlot>(slot);
        
        bool hasEquipment;
        file.read(reinterpret_cast<char*>(&hasEquipment), sizeof(hasEquipment));
        
        if (hasEquipment) {
            // 装備の種類を読み込み
            ItemType itemType;
            file.read(reinterpret_cast<char*>(&itemType), sizeof(itemType));
            
            // 装備名の長さと名前を読み込み
            int nameLength;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            char* nameBuffer = new char[nameLength + 1];
            file.read(nameBuffer, nameLength);
            nameBuffer[nameLength] = '\0';
            std::string equipmentName(nameBuffer);
            delete[] nameBuffer;
            
            // 装備を再作成（簡易版）
            std::unique_ptr<Equipment> equipment = nullptr;
            if (itemType == ItemType::WEAPON) {
                equipment = std::make_unique<Weapon>(WeaponType::COPPER_SWORD);
            } else if (itemType == ItemType::ARMOR) {
                equipment = std::make_unique<Armor>(ArmorType::LEATHER_ARMOR);
            }
            
            if (equipment) {
                equipItem(std::move(equipment));
            }
        }
    }
} 