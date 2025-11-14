/**
 * @file Item.h
 * @brief アイテム関連のクラスと列挙型
 * @details アイテムの基底クラス、消費アイテムクラス、アイテムの種類を定義する。
 */

#pragma once
#include <string>
#include <memory>

class Character;
class Player;

/**
 * @brief アイテムの種類
 */
enum class ItemType {
    CONSUMABLE,     /**< @brief 消費アイテム */
    WEAPON,         /**< @brief 武器 */
    ARMOR,          /**< @brief 防具 */
    ACCESSORY,      /**< @brief アクセサリー */
    KEY_ITEM        /**< @brief 重要アイテム */
};

/**
 * @brief 消費アイテムの種類
 */
enum class ConsumableType {
    YAKUSOU,        // やくそう (HP回復)
    SEISUI,         // せいすい (MP回復 小)
    MAHOU_SEISUI,   // まほうのせいすい (MP回復 大)
    DOKUKESHI,      // どくけしそう (毒回復)
    CHIKARASUI,     // ちからのみず (攻撃力一時上昇)
    MAMORI_SEED     /**< @brief まもりのたね (防御力一時上昇) */
};

/**
 * @brief アイテムの基底クラス
 * @details 全てのアイテムの基底となる抽象クラス。
 * アイテムの基本情報（名前、説明、価格など）と使用処理を定義する。
 */
class Item {
protected:
    std::string name;
    std::string description;
    int price;
    ItemType type;
    bool stackable;
    int maxStack;

public:
    /**
     * @brief コンストラクタ
     * @param name アイテム名
     * @param description 説明
     * @param price 価格
     * @param type アイテムタイプ
     * @param stackable スタック可能か（デフォルト: true）
     * @param maxStack 最大スタック数（デフォルト: 99）
     */
    Item(const std::string& name, const std::string& description, int price, ItemType type, bool stackable = true, int maxStack = 99);
    
    /**
     * @brief デストラクタ
     */
    virtual ~Item() = default;

    /**
     * @brief アイテム名の取得
     * @return アイテム名
     */
    std::string getName() const { return name; }
    
    /**
     * @brief 説明の取得
     * @return 説明
     */
    std::string getDescription() const { return description; }
    
    /**
     * @brief 価格の取得
     * @return 価格
     */
    int getPrice() const { return price; }
    
    /**
     * @brief アイテムタイプの取得
     * @return アイテムタイプ
     */
    ItemType getType() const { return type; }
    
    /**
     * @brief スタック可能か
     * @return スタック可能か
     */
    bool isStackable() const { return stackable; }
    
    /**
     * @brief 最大スタック数の取得
     * @return 最大スタック数
     */
    int getMaxStack() const { return maxStack; }

    /**
     * @brief アイテムの使用（純粋仮想関数）
     * @param player プレイヤーへのポインタ
     * @param target ターゲット（nullptrの場合は自分自身）
     * @return 使用が成功したか
     */
    virtual bool use(Player* player, Character* target = nullptr) = 0;
    
    /**
     * @brief アイテムのクローン（純粋仮想関数）
     * @return クローンされたアイテムへのユニークポインタ
     */
    virtual std::unique_ptr<Item> clone() const = 0;
};

/**
 * @brief 消費アイテムクラス
 * @details 消費アイテム（やくそう、せいすいなど）を実装するクラス。
 */
class ConsumableItem : public Item {
private:
    ConsumableType consumableType;
    int effectValue;

public:
    /**
     * @brief コンストラクタ
     * @param type 消費アイテムタイプ
     */
    ConsumableItem(ConsumableType type);
    
    /**
     * @brief アイテムの使用
     * @param player プレイヤーへのポインタ
     * @param target ターゲット（nullptrの場合は自分自身）
     * @return 使用が成功したか
     */
    bool use(Player* player, Character* target = nullptr) override;
    
    /**
     * @brief アイテムのクローン
     * @return クローンされたアイテムへのユニークポインタ
     */
    std::unique_ptr<Item> clone() const override;
    
    /**
     * @brief 消費アイテムタイプの取得
     * @return 消費アイテムタイプ
     */
    ConsumableType getConsumableType() const { return consumableType; }
    
    /**
     * @brief 効果値の取得
     * @return 効果値
     */
    int getEffectValue() const { return effectValue; }
    
private:
    /**
     * @brief アイテムデータのセットアップ
     * @param type 消費アイテムタイプ
     */
    void setupItemData(ConsumableType type);
}; 