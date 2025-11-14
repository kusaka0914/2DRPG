#include "Inventory.h"
#include "../entities/Player.h"
#include <iostream>
#include <algorithm>
#include <fstream>

Inventory::Inventory(int maxSlots) : maxSlots(maxSlots) {
    slots.resize(maxSlots);
}

bool Inventory::addItem(std::unique_ptr<Item> item, int quantity) {
    if (!item || quantity <= 0) return false;
    
    if (item->isStackable()) {
        int stackableSlot = findStackableSlot(item.get());
        if (stackableSlot != -1) {
            int remainingSpace = item->getMaxStack() - slots[stackableSlot].quantity;
            int toAdd = std::min(quantity, remainingSpace);
            
            slots[stackableSlot].quantity += toAdd;
            quantity -= toAdd;
            
            if (quantity == 0) {
                return true;
            }
        }
    }
    
    while (quantity > 0) {
        int emptySlot = findEmptySlot();
        if (emptySlot == -1) {
            return false;
        }
        
        int toAdd = item->isStackable() ? std::min(quantity, item->getMaxStack()) : 1;
        slots[emptySlot].item = item->clone();
        slots[emptySlot].quantity = toAdd;
        quantity -= toAdd;
    }
    
    return true;
}

bool Inventory::removeItem(int slotIndex, int quantity) {
    if (slotIndex < 0 || slotIndex >= maxSlots || !slots[slotIndex].item) {
        return false;
    }
    
    if (quantity >= slots[slotIndex].quantity) {
        // 全て削除
        slots[slotIndex].item.reset();
        slots[slotIndex].quantity = 0;
    } else {
        // 一部削除
        slots[slotIndex].quantity -= quantity;
    }
    
    return true;
}

bool Inventory::useItem(int slotIndex, Player* player, Character* target) {
    if (slotIndex < 0 || slotIndex >= maxSlots || !slots[slotIndex].item || !player) {
        return false;
    }
    
    bool success = slots[slotIndex].item->use(player, target);
    if (success) {
        removeItem(slotIndex, 1);
    }
    
    return success;
}

int Inventory::findItem(const std::string& itemName) const {
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item && slots[i].item->getName() == itemName) {
            return i;
        }
    }
    return -1;
}

int Inventory::getItemCount(const std::string& itemName) const {
    int total = 0;
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item && slots[i].item->getName() == itemName) {
            total += slots[i].quantity;
        }
    }
    return total;
}

const InventorySlot* Inventory::getSlot(int index) const {
    if (index < 0 || index >= maxSlots) return nullptr;
    return &slots[index];
}

void Inventory::displayInventory() const {
    bool hasItems = false;
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item) {
            hasItems = true;
        }
    }
}

bool Inventory::isEmpty() const {
    return getUsedSlots() == 0;
}

bool Inventory::isFull() const {
    return getUsedSlots() >= maxSlots;
}

int Inventory::getUsedSlots() const {
    int used = 0;
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item) used++;
    }
    return used;
}

void Inventory::sortInventory() {
    compactInventory();
    
    std::vector<InventorySlot> tempSlots;
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item) {
            tempSlots.push_back(std::move(slots[i]));
            slots[i] = InventorySlot();
        }
    }
    
    std::sort(tempSlots.begin(), tempSlots.end(), 
        [](const InventorySlot& a, const InventorySlot& b) {
            if (a.item->getType() != b.item->getType()) {
                return a.item->getType() < b.item->getType();
            }
            return a.item->getName() < b.item->getName();
        });
    
    for (size_t i = 0; i < tempSlots.size() && i < maxSlots; ++i) {
        slots[i] = std::move(tempSlots[i]);
    }
}

void Inventory::compactInventory() {
    std::vector<InventorySlot> tempSlots;
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item) {
            tempSlots.push_back(std::move(slots[i]));
        }
        slots[i] = InventorySlot();
    }
    
    for (size_t i = 0; i < tempSlots.size() && i < maxSlots; ++i) {
        slots[i] = std::move(tempSlots[i]);
    }
}

int Inventory::findEmptySlot() const {
    for (int i = 0; i < maxSlots; ++i) {
        if (!slots[i].item) {
            return i;
        }
    }
    return -1;
}

int Inventory::findStackableSlot(const Item* item) const {
    if (!item || !item->isStackable()) return -1;
    
    for (int i = 0; i < maxSlots; ++i) {
        if (slots[i].item && canStackWith(slots[i].item.get(), item) &&
            slots[i].quantity < item->getMaxStack()) {
            return i;
        }
    }
    return -1;
}

bool Inventory::canStackWith(const Item* item1, const Item* item2) const {
    if (!item1 || !item2) return false;
    return item1->getName() == item2->getName() && 
           item1->getType() == item2->getType();
}

void Inventory::saveToFile(std::ofstream& file) {
    file.write(reinterpret_cast<const char*>(&maxSlots), sizeof(maxSlots));
    
    for (int i = 0; i < maxSlots; ++i) {
        bool hasItem = (slots[i].item != nullptr);
        file.write(reinterpret_cast<const char*>(&hasItem), sizeof(hasItem));
        
        if (hasItem) {
            ItemType itemType = slots[i].item->getType();
            file.write(reinterpret_cast<const char*>(&itemType), sizeof(itemType));
            
            std::string itemName = slots[i].item->getName();
            int nameLength = itemName.length();
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(itemName.c_str(), nameLength);
            
            file.write(reinterpret_cast<const char*>(&slots[i].quantity), sizeof(slots[i].quantity));
        }
    }
}

void Inventory::loadFromFile(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&maxSlots), sizeof(maxSlots));
    slots.resize(maxSlots);
    
    for (int i = 0; i < maxSlots; ++i) {
        bool hasItem;
        file.read(reinterpret_cast<char*>(&hasItem), sizeof(hasItem));
        
        if (hasItem) {
            ItemType itemType;
            file.read(reinterpret_cast<char*>(&itemType), sizeof(itemType));
            
            int nameLength;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            char* nameBuffer = new char[nameLength + 1];
            file.read(nameBuffer, nameLength);
            nameBuffer[nameLength] = '\0';
            std::string itemName(nameBuffer);
            delete[] nameBuffer;
            
            file.read(reinterpret_cast<char*>(&slots[i].quantity), sizeof(slots[i].quantity));
            
            switch (itemType) {
                case ItemType::CONSUMABLE:
                    slots[i].item = std::make_unique<ConsumableItem>(ConsumableType::YAKUSOU);
                    break;
                case ItemType::WEAPON:
                    slots[i].item = std::make_unique<Weapon>(WeaponType::COPPER_SWORD);
                    break;
                case ItemType::ARMOR:
                    slots[i].item = std::make_unique<Armor>(ArmorType::LEATHER_ARMOR);
                    break;
                default:
                    slots[i].item = nullptr;
                    break;
            }
        } else {
            slots[i] = InventorySlot();
        }
    }
} 