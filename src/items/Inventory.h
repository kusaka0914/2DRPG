/**
 * @file Inventory.h
 * @brief インベントリ管理を担当するクラス
 * @details プレイヤーのアイテム所持、追加、削除、使用などの機能を管理する。
 */

#pragma once
#include "Item.h"
#include <vector>
#include <memory>
#include <map>

/**
 * @brief インベントリスロットの構造体
 * @details アイテムとその数量を保持する。
 */
struct InventorySlot {
    std::unique_ptr<Item> item;
    int quantity;
    
    InventorySlot() : item(nullptr), quantity(0) {}
    InventorySlot(std::unique_ptr<Item> item, int quantity) 
        : item(std::move(item)), quantity(quantity) {}
};

/**
 * @brief インベントリ管理を担当するクラス
 * @details プレイヤーのアイテム所持、追加、削除、使用などの機能を管理する。
 */
class Inventory {
private:
    std::vector<InventorySlot> slots;
    int maxSlots;
    
public:
    /**
     * @brief コンストラクタ
     * @param maxSlots 最大スロット数（デフォルト: 20）
     */
    Inventory(int maxSlots = 20);
    
    /**
     * @brief アイテムの追加
     * @param item 追加するアイテム
     * @param quantity 数量（デフォルト: 1）
     * @return 追加が成功したか
     */
    bool addItem(std::unique_ptr<Item> item, int quantity = 1);
    
    /**
     * @brief アイテムの削除
     * @param slotIndex スロットインデックス
     * @param quantity 削除する数量（デフォルト: 1）
     * @return 削除が成功したか
     */
    bool removeItem(int slotIndex, int quantity = 1);
    
    /**
     * @brief アイテムの使用
     * @param slotIndex スロットインデックス
     * @param player プレイヤーへのポインタ
     * @param target ターゲット（nullptrの場合は自分自身）
     * @return 使用が成功したか
     */
    bool useItem(int slotIndex, Player* player, Character* target = nullptr);
    
    /**
     * @brief アイテムの検索
     * @param itemName アイテム名
     * @return 見つかったスロットインデックス（見つからない場合は-1）
     */
    int findItem(const std::string& itemName) const;
    
    /**
     * @brief アイテムの数量取得
     * @param itemName アイテム名
     * @return 所持数量
     */
    int getItemCount(const std::string& itemName) const;
    
    /**
     * @brief スロットの取得
     * @param index スロットインデックス
     * @return スロットへのポインタ（存在しない場合はnullptr）
     */
    const InventorySlot* getSlot(int index) const;
    
    /**
     * @brief インベントリの表示
     */
    void displayInventory() const;
    
    /**
     * @brief インベントリが空かどうか
     * @return 空かどうか
     */
    bool isEmpty() const;
    
    /**
     * @brief インベントリが満杯かどうか
     * @return 満杯かどうか
     */
    bool isFull() const;
    
    /**
     * @brief 使用中のスロット数の取得
     * @return 使用中のスロット数
     */
    int getUsedSlots() const;
    
    /**
     * @brief 最大スロット数の取得
     * @return 最大スロット数
     */
    int getMaxSlots() const { return maxSlots; }
    
    /**
     * @brief インベントリのソート
     */
    void sortInventory();
    
    /**
     * @brief インベントリの圧縮（空スロットを詰める）
     */
    void compactInventory();
    
    /**
     * @brief ファイルへの保存
     * @param file 出力ファイルストリーム
     */
    void saveToFile(std::ofstream& file);
    
    /**
     * @brief ファイルからの読み込み
     * @param file 入力ファイルストリーム
     */
    void loadFromFile(std::ifstream& file);
    
private:
    int findEmptySlot() const;
    int findStackableSlot(const Item* item) const;
    bool canStackWith(const Item* item1, const Item* item2) const;
}; 