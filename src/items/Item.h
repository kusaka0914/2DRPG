#pragma once
#include <string>
#include <memory>

class Character;
class Player;

enum class ItemType {
    CONSUMABLE,     // 消費アイテム
    WEAPON,         // 武器
    ARMOR,          // 防具
    ACCESSORY,      // アクセサリー
    KEY_ITEM        // 重要アイテム
};

enum class ConsumableType {
    YAKUSOU,        // やくそう (HP回復)
    SEISUI,         // せいすい (MP回復 小)
    MAHOU_SEISUI,   // まほうのせいすい (MP回復 大)
    DOKUKESHI,      // どくけしそう (毒回復)
    CHIKARASUI,     // ちからのみず (攻撃力一時上昇)
    MAMORI_SEED     // まもりのたね (防御力一時上昇)
};

class Item {
protected:
    std::string name;
    std::string description;
    int price;
    ItemType type;
    bool stackable;
    int maxStack;

public:
    Item(const std::string& name, const std::string& description, int price, ItemType type, bool stackable = true, int maxStack = 99);
    virtual ~Item() = default;

    // ゲッター
    std::string getName() const { return name; }
    std::string getDescription() const { return description; }
    int getPrice() const { return price; }
    ItemType getType() const { return type; }
    bool isStackable() const { return stackable; }
    int getMaxStack() const { return maxStack; }

    // 純粋仮想関数
    virtual bool use(Player* player, Character* target = nullptr) = 0;
    virtual std::unique_ptr<Item> clone() const = 0;
};

class ConsumableItem : public Item {
private:
    ConsumableType consumableType;
    int effectValue;

public:
    ConsumableItem(ConsumableType type);
    
    bool use(Player* player, Character* target = nullptr) override;
    std::unique_ptr<Item> clone() const override;
    
    ConsumableType getConsumableType() const { return consumableType; }
    int getEffectValue() const { return effectValue; }
    
private:
    void setupItemData(ConsumableType type);
}; 