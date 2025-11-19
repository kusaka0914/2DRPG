/**
 * @file TownLayout.h
 * @brief 街のレイアウトデータを定義する構造体
 * @details 街の建物、住民、衛兵、城などの位置情報を定数として定義する。
 */

#pragma once
#include <vector>
#include <string>
#include <utility>
#include <tuple>
#include <SDL2/SDL.h>
#include <nlohmann/json.hpp>

/**
 * @brief 街の共通配置データ
 * @details 街の建物、住民、衛兵、城などの位置情報を定数として定義する。
 */
struct TownLayout {
    // 建物の位置とタイプ
    static const std::vector<std::pair<int, int>> BUILDINGS;
    static const std::vector<std::string> BUILDING_TYPES;
    
    // 住人の家の位置
    static const std::vector<std::pair<int, int>> RESIDENT_HOMES;
    
    // 住民の位置
    static const std::vector<std::pair<int, int>> RESIDENTS;
    
    // 衛兵の位置
    static const std::vector<std::pair<int, int>> GUARDS;
    
    // 城の位置
    static const int CASTLE_X = 13;
    static const int CASTLE_Y = 2;
    
    // ゲート（出口）の位置
    static const int GATE_X = 14;
    static const int GATE_Y = 15;
    
    // プレイヤーの初期位置
    static const int PLAYER_START_X = 14;
    static const int PLAYER_START_Y = 14;
    
    /**
     * @brief 住民のテクスチャインデックスを取得
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @return テクスチャインデックス（0-5、6つの画像を循環使用）
     */
    static int getResidentTextureIndex(int x, int y);
    
    /**
     * @brief 住民の名前を取得
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @return 住民の名前
     */
    static std::string getResidentName(int x, int y);
    
    /**
     * @brief 住民の会話文を取得
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @return 住民の会話文
     */
    static std::string getResidentDialogue(int x, int y);
    
    /**
     * @brief 衛兵の名前を取得
     * @param x 衛兵のX座標
     * @param y 衛兵のY座標
     * @return 衛兵の名前
     */
    static std::string getGuardName(int x, int y);
    
    /**
     * @brief 衛兵の会話文を取得
     * @param x 衛兵のX座標
     * @param y 衛兵のY座標
     * @return 衛兵の会話文
     */
    static std::string getGuardDialogue(int x, int y);
    
    /**
     * @brief 指定された位置の住民が倒されたかどうかをチェック
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @param killedResidents 倒した住民の位置リスト
     * @return 倒されたかどうか
     */
    static bool isResidentKilled(int x, int y, const std::vector<std::pair<int, int>>& killedResidents);
    
    /**
     * @brief 全住民が倒されたかどうかをチェック
     * @param killedResidents 倒した住民の位置リスト
     * @return 全住民が倒されたかどうか
     */
    static bool areAllResidentsKilled(const std::vector<std::pair<int, int>>& killedResidents);
    
    /**
     * @brief 位置リストから重複を除去
     * @param positions 位置リスト（参照渡し、重複除去後のリストが格納される）
     */
    static void removeDuplicatePositions(std::vector<std::pair<int, int>>& positions);
    
    /**
     * @brief 建物タイプから色を取得
     * @param buildingType 建物タイプ（"shop", "weapon_shop", "house", "castle"など）
     * @return RGB色（r, g, b）
     */
    static std::tuple<Uint8, Uint8, Uint8> getBuildingColor(const std::string& buildingType);
    
    /**
     * @brief 位置リストをJSON配列に変換
     * @param positions 位置リスト
     * @return JSON配列
     */
    static nlohmann::json positionsToJson(const std::vector<std::pair<int, int>>& positions);
    
    /**
     * @brief JSON配列から位置リストに変換
     * @param j JSON配列
     * @return 位置リスト
     */
    static std::vector<std::pair<int, int>> positionsFromJson(const nlohmann::json& j);
    
private:
    /**
     * @brief 住民の位置からインデックスを取得（内部関数）
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @return インデックス（見つからない場合は-1）
     */
    static int findResidentIndex(int x, int y);
    
    /**
     * @brief 衛兵の位置からインデックスを取得（内部関数）
     * @param x 衛兵のX座標
     * @param y 衛兵のY座標
     * @return インデックス（見つからない場合は-1）
     */
    static int findGuardIndex(int x, int y);
}; 