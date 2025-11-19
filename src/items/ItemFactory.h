/**
 * @file ItemFactory.h
 * @brief アイテム作成を担当するファクトリークラス
 * @details アイテム名から適切なアイテムを作成する機能を提供する。
 */

#pragma once
#include "Item.h"
#include "Equipment.h"
#include <memory>
#include <string>

/**
 * @brief アイテム作成を担当するファクトリークラス
 * @details アイテム名から適切なアイテムを作成する機能を提供する。
 */
class ItemFactory {
public:
    /**
     * @brief アイテム名からアイテムを作成
     * @param itemName アイテム名
     * @param itemType アイテムタイプ
     * @return 作成されたアイテムへのユニークポインタ（失敗時はnullptr）
     */
    static std::unique_ptr<Item> createItemByName(const std::string& itemName, ItemType itemType);
};

