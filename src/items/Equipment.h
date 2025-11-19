/**
 * @file Equipment.h
 * @brief 装備アイテム関連のクラスと列挙型
 * @details 装備アイテムの基底クラス、武器クラス、防具クラス、装備管理クラスを定義する。
 */

#pragma once
#include "Item.h"
#include <memory>
#include <nlohmann/json.hpp>

/**
 * @brief 武器の種類
 */
enum class WeaponType {
    COPPER_SWORD,      // どうのつるぎ
    IRON_SWORD,        // てつのつるぎ
    STEEL_SWORD,       // はがねのつるぎ
    FLAME_SWORD,       // ほのおのつるぎ
    WOODEN_STICK,      // たけのぼう
    CLUB,              // こんぼう
    IRON_SPEAR         /**< @brief てつのやり */
};

/**
 * @brief 防具の種類
 */
enum class ArmorType {
    CLOTH_ARMOR,       // ぬののふく
    LEATHER_ARMOR,     // かわのよろい
    CHAIN_ARMOR,       // くさりかたびら
    IRON_ARMOR,        // てつのよろい
    STEEL_ARMOR,       // はがねのよろい
    LEATHER_SHIELD,    // かわのたて
    IRON_SHIELD,       // てつのたて
    STEEL_SHIELD       /**< @brief はがねのたて */
};

/**
 * @brief 装備スロットの種類
 */
enum class EquipmentSlot {
    WEAPON,     /**< @brief 武器 */
    ARMOR,      /**< @brief 防具 */
    SHIELD,     /**< @brief 盾 */
    ACCESSORY   /**< @brief アクセサリー */
};

/**
 * @brief 装備アイテムの基底クラス
 * @details 全ての装備アイテムの基底となるクラス。
 * 装備によるステータスボーナス（攻撃力、防御力、HP、MP）を管理する。
 */
class Equipment : public Item {
protected:
    int attackBonus;
    int defenseBonus;
    int hpBonus;
    int mpBonus;
    EquipmentSlot slot;

public:
    /**
     * @brief コンストラクタ
     * @param name アイテム名
     * @param description 説明
     * @param price 価格
     * @param slot 装備スロット
     * @param attackBonus 攻撃力ボーナス（デフォルト: 0）
     * @param defenseBonus 防御力ボーナス（デフォルト: 0）
     * @param hpBonus HPボーナス（デフォルト: 0）
     * @param mpBonus MPボーナス（デフォルト: 0）
     */
    Equipment(const std::string& name, const std::string& description, int price, 
              EquipmentSlot slot, int attackBonus = 0, int defenseBonus = 0, 
              int hpBonus = 0, int mpBonus = 0);

    /**
     * @brief 攻撃力ボーナスの取得
     * @return 攻撃力ボーナス
     */
    int getAttackBonus() const { return attackBonus; }
    
    /**
     * @brief 防御力ボーナスの取得
     * @return 防御力ボーナス
     */
    int getDefenseBonus() const { return defenseBonus; }
    
    /**
     * @brief HPボーナスの取得
     * @return HPボーナス
     */
    int getHpBonus() const { return hpBonus; }
    
    /**
     * @brief MPボーナスの取得
     * @return MPボーナス
     */
    int getMpBonus() const { return mpBonus; }
    
    /**
     * @brief 装備スロットの取得
     * @return 装備スロット
     */
    EquipmentSlot getSlot() const { return slot; }

    /**
     * @brief アイテムの使用
     * @param player プレイヤーへのポインタ
     * @param target ターゲット（nullptrの場合は自分自身）
     * @return 使用が成功したか
     */
    bool use(Player* player, Character* target = nullptr) override;
    
    /**
     * @brief アイテムのクローン
     * @return クローンされたアイテム
     */
    std::unique_ptr<Item> clone() const override;
};

/**
 * @brief 武器クラス
 * @details 武器アイテムを実装するクラス。
 */
class Weapon : public Equipment {
private:
    WeaponType weaponType;

public:
    /**
     * @brief コンストラクタ
     * @param type 武器の種類
     */
    Weapon(WeaponType type);
    
    /**
     * @brief 武器の種類の取得
     * @return 武器の種類
     */
    WeaponType getWeaponType() const { return weaponType; }
    
    /**
     * @brief アイテムのクローン
     * @return クローンされたアイテム
     */
    std::unique_ptr<Item> clone() const override;

private:
    /**
     * @brief 武器データのセットアップ
     * @param type 武器の種類
     */
    void setupWeaponData(WeaponType type);
};

/**
 * @brief 防具クラス
 * @details 防具アイテムを実装するクラス。
 */
class Armor : public Equipment {
private:
    ArmorType armorType;

public:
    /**
     * @brief コンストラクタ
     * @param type 防具の種類
     */
    Armor(ArmorType type);
    
    /**
     * @brief 防具の種類の取得
     * @return 防具の種類
     */
    ArmorType getArmorType() const { return armorType; }
    
    /**
     * @brief アイテムのクローン
     * @return クローンされたアイテム
     */
    std::unique_ptr<Item> clone() const override;

private:
    /**
     * @brief 防具データのセットアップ
     * @param type 防具の種類
     */
    void setupArmorData(ArmorType type);
};

/**
 * @brief 装備中のアイテムの構造体
 * @details 各装備スロットに装備されているアイテムを保持する。
 */
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

/**
 * @brief 装備管理を担当するクラス
 * @details プレイヤーの装備アイテムの装備、外す、ボーナス計算などの機能を管理する。
 */
class EquipmentManager {
private:
    EquippedItems equipped;

public:
    /**
     * @brief コンストラクタ
     */
    EquipmentManager() = default;

    /**
     * @brief アイテムの装備
     * @param equipment 装備するアイテム
     * @return 装備が成功したか
     */
    bool equipItem(std::unique_ptr<Equipment> equipment);
    
    /**
     * @brief アイテムの外す
     * @param slot 装備スロット
     * @return 外したアイテム（装備されていない場合はnullptr）
     */
    std::unique_ptr<Equipment> unequipItem(EquipmentSlot slot);
    
    /**
     * @brief 装備中のアイテムの取得
     * @param slot 装備スロット
     * @return 装備中のアイテムへのポインタ（装備されていない場合はnullptr）
     */
    const Equipment* getEquippedItem(EquipmentSlot slot) const;
    
    /**
     * @brief 装備中かどうかの確認
     * @param slot 装備スロット
     * @return 装備中かどうか
     */
    bool hasEquippedItem(EquipmentSlot slot) const;
    
    /**
     * @brief 総攻撃力ボーナスの取得
     * @return 総攻撃力ボーナス
     */
    int getTotalAttackBonus() const;
    
    /**
     * @brief 総防御力ボーナスの取得
     * @return 総防御力ボーナス
     */
    int getTotalDefenseBonus() const;
    
    /**
     * @brief 総HPボーナスの取得
     * @return 総HPボーナス
     */
    int getTotalHpBonus() const;
    
    /**
     * @brief 総MPボーナスの取得
     * @return 総MPボーナス
     */
    int getTotalMpBonus() const;
    
    /**
     * @brief 装備の表示
     */
    void displayEquipment() const;
    
    /**
     * @brief JSON形式への変換
     * @return JSONオブジェクト
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief JSON形式からの読み込み
     * @param j JSONオブジェクト
     */
    void fromJson(const nlohmann::json& j);
}; 