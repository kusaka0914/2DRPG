#include "Item.h"
#include "Player.h"
#include "Character.h"
#include <iostream>

Item::Item(const std::string& name, const std::string& description, int price, ItemType type, bool stackable, int maxStack)
    : name(name), description(description), price(price), type(type), stackable(stackable), maxStack(maxStack) {
}

ConsumableItem::ConsumableItem(ConsumableType type) 
    : Item("", "", 0, ItemType::CONSUMABLE), consumableType(type), effectValue(0) {
    setupItemData(type);
}

void ConsumableItem::setupItemData(ConsumableType type) {
    switch (type) {
        case ConsumableType::YAKUSOU:
            name = "やくそう";
            description = "HPを少し回復する";
            price = 8;
            effectValue = 25;
            break;
            
        case ConsumableType::SEISUI:
            name = "せいすい";
            description = "MPを少し回復する";
            price = 20;
            effectValue = 10;
            break;
            
        case ConsumableType::MAHOU_SEISUI:
            name = "まほうのせいすい";
            description = "MPを大きく回復する";
            price = 80;
            effectValue = 30;
            break;
            
        case ConsumableType::DOKUKESHI:
            name = "どくけしそう";
            description = "毒状態を回復する";
            price = 10;
            effectValue = 0; // 状態異常回復なので値なし
            break;
            
        case ConsumableType::CHIKARASUI:
            name = "ちからのみず";
            description = "戦闘中、攻撃力が上がる";
            price = 50;
            effectValue = 5;
            break;
            
        case ConsumableType::MAMORI_SEED:
            name = "まもりのたね";
            description = "戦闘中、防御力が上がる";
            price = 50;
            effectValue = 3;
            break;
    }
}

bool ConsumableItem::use(Player* player, Character* target) {
    if (!player) return false;
    
    switch (consumableType) {
        case ConsumableType::YAKUSOU:
            if (player->getHp() >= player->getMaxHp()) {
                std::cout << "HPは既に最大です！" << std::endl;
                return false;
            }
            player->heal(effectValue);
            std::cout << player->getName() << "は" << name << "を使った！" << std::endl;
            return true;
            
        case ConsumableType::SEISUI:
        case ConsumableType::MAHOU_SEISUI:
            if (player->getMp() >= player->getMaxMp()) {
                std::cout << "MPは既に最大です！" << std::endl;
                return false;
            }
            player->restoreMp(effectValue);
            std::cout << player->getName() << "は" << name << "を使った！" << std::endl;
            return true;
            
        case ConsumableType::DOKUKESHI:
            // 将来の状態異常システム用
            std::cout << player->getName() << "は" << name << "を使った！" << std::endl;
            std::cout << "毒が治った！" << std::endl;
            return true;
            
        case ConsumableType::CHIKARASUI:
            // 戦闘中の一時的ステータス上昇（将来実装）
            std::cout << player->getName() << "は" << name << "を飲んだ！" << std::endl;
            std::cout << "攻撃力が上がった！" << std::endl;
            return true;
            
        case ConsumableType::MAMORI_SEED:
            // 戦闘中の一時的ステータス上昇（将来実装）
            std::cout << player->getName() << "は" << name << "を食べた！" << std::endl;
            std::cout << "防御力が上がった！" << std::endl;
            return true;
    }
    
    return false;
}

std::unique_ptr<Item> ConsumableItem::clone() const {
    return std::make_unique<ConsumableItem>(consumableType);
} 