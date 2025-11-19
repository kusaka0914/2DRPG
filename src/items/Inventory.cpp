#include "Inventory.h"
#include "../entities/Player.h"
#include "ItemFactory.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

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

nlohmann::json Inventory::toJson() const {
    nlohmann::json j;
    j["maxSlots"] = maxSlots;
    j["slots"] = nlohmann::json::array();
    
    for (int i = 0; i < maxSlots; ++i) {
        nlohmann::json slotJson;
        if (slots[i].item) {
            slotJson["hasItem"] = true;
            slotJson["itemName"] = slots[i].item->getName();
            slotJson["itemType"] = static_cast<int>(slots[i].item->getType());
            slotJson["quantity"] = slots[i].quantity;
        } else {
            slotJson["hasItem"] = false;
        }
        j["slots"].push_back(slotJson);
    }
    
    return j;
}

void Inventory::fromJson(const nlohmann::json& j) {
    if (j.contains("maxSlots")) {
        maxSlots = j["maxSlots"];
        slots.resize(maxSlots);
    }
    
    if (j.contains("slots") && j["slots"].is_array()) {
        for (size_t i = 0; i < j["slots"].size() && i < static_cast<size_t>(maxSlots); ++i) {
            const auto& slotJson = j["slots"][i];
            if (slotJson.contains("hasItem") && slotJson["hasItem"].get<bool>()) {
                std::string itemName = slotJson["itemName"];
                ItemType itemType = static_cast<ItemType>(slotJson["itemType"]);
                int quantity = slotJson["quantity"];
                
                auto item = ItemFactory::createItemByName(itemName, itemType);
                if (item) {
                    slots[i].item = std::move(item);
                    slots[i].quantity = quantity;
                }
            } else {
                slots[i] = InventorySlot();
            }
        }
    }
} 