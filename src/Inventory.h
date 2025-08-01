#pragma once
#include "Item.h"
#include <vector>
#include <memory>
#include <map>

struct InventorySlot {
    std::unique_ptr<Item> item;
    int quantity;
    
    InventorySlot() : item(nullptr), quantity(0) {}
    InventorySlot(std::unique_ptr<Item> item, int quantity) 
        : item(std::move(item)), quantity(quantity) {}
};

class Inventory {
private:
    std::vector<InventorySlot> slots;
    int maxSlots;
    
public:
    Inventory(int maxSlots = 20);
    
    // アイテム操作
    bool addItem(std::unique_ptr<Item> item, int quantity = 1);
    bool removeItem(int slotIndex, int quantity = 1);
    bool useItem(int slotIndex, Player* player, Character* target = nullptr);
    
    // 検索・取得
    int findItem(const std::string& itemName) const;
    int getItemCount(const std::string& itemName) const;
    const InventorySlot* getSlot(int index) const;
    
    // 情報表示
    void displayInventory() const;
    bool isEmpty() const;
    bool isFull() const;
    int getUsedSlots() const;
    int getMaxSlots() const { return maxSlots; }
    
    // スロット管理
    void sortInventory();
    void compactInventory();
    
private:
    int findEmptySlot() const;
    int findStackableSlot(const Item* item) const;
    bool canStackWith(const Item* item1, const Item* item2) const;
}; 