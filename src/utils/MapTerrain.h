/**
 * @file MapTerrain.h
 * @brief マップ地形関連のクラスと列挙型
 * @details 地形の種類、地形データ、マップタイル、マップ生成、地形レンダリングなどの機能を定義する。
 */

#pragma once
#include <vector>
#include <string>

/**
 * @brief 地形の種類
 */
enum class TerrainType {
    GRASS,          // 草原
    FOREST,         // 森
    MOUNTAIN,       // 山
    WATER,          // 川・湖
    ROAD,           // 道路
    BRIDGE,         // 橋
    ROCK,           // 岩
    FLOWER_FIELD,   // 花畑
    DESERT,         // 砂漠
    TOWN_ENTRANCE   /**< @brief 街の入り口 */
};

/**
 * @brief 地形データの構造体
 * @details 地形の種類、通行可否、エンカウント率を保持する。
 */
struct TerrainData {
    TerrainType type;
    bool walkable;
    int encounterRate;  // エンカウント率（0-100）
    
    TerrainData(TerrainType t, bool w, int e) 
        : type(t), walkable(w), encounterRate(e) {}
};

/**
 * @brief マップタイルの構造体
 * @details 地形の種類、オブジェクトの有無、オブジェクトの種類を保持する。
 */
struct MapTile {
    TerrainType terrain;
    bool hasObject;      // 木や岩などのオブジェクトがあるか
    int objectType;      // オブジェクトの種類
    
    MapTile(TerrainType t = TerrainType::GRASS, bool obj = false, int objType = 0)
        : terrain(t), hasObject(obj), objectType(objType) {}
};

/**
 * @brief マップ生成を担当するクラス
 * @details リアルな地形を持つマップを生成する。
 */
class MapGenerator {
public:
    /**
     * @brief リアルな地形を持つマップを生成
     * @param width マップ幅
     * @param height マップ高さ
     * @return 生成されたマップタイルの2次元ベクター
     */
    static std::vector<std::vector<MapTile>> generateRealisticMap(int width, int height);
    
private:
    /**
     * @brief 川を追加
     * @param map マップへの参照
     * @param width マップ幅
     * @param height マップ高さ
     */
    static void addRiver(std::vector<std::vector<MapTile>>& map, int width, int height);
    
    /**
     * @brief 森を追加
     * @param map マップへの参照
     * @param width マップ幅
     * @param height マップ高さ
     */
    static void addForest(std::vector<std::vector<MapTile>>& map, int width, int height);
    
    /**
     * @brief 山を追加
     * @param map マップへの参照
     * @param width マップ幅
     * @param height マップ高さ
     */
    static void addMountains(std::vector<std::vector<MapTile>>& map, int width, int height);
    
    /**
     * @brief 道路を追加
     * @param map マップへの参照
     * @param width マップ幅
     * @param height マップ高さ
     */
    static void addRoads(std::vector<std::vector<MapTile>>& map, int width, int height);
    
    /**
     * @brief ランダムなオブジェクトを追加
     * @param map マップへの参照
     * @param width マップ幅
     * @param height マップ高さ
     */
    static void addRandomObjects(std::vector<std::vector<MapTile>>& map, int width, int height);
    
    /**
     * @brief 地形を滑らかにする
     * @param map マップへの参照
     * @param width マップ幅
     * @param height マップ高さ
     */
    static void smoothTerrain(std::vector<std::vector<MapTile>>& map, int width, int height);
};

/**
 * @brief 地形レンダリングを担当するクラス
 * @details 地形の色、オブジェクトの色、地形データを提供する。
 */
class TerrainRenderer {
public:
    /**
     * @brief 色を格納する構造体
     */
    struct Color {
        int r, g, b, a;
        /**
         * @brief コンストラクタ
         * @param r 赤成分
         * @param g 緑成分
         * @param b 青成分
         * @param a アルファ成分（デフォルト: 255）
         */
        Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {}
    };
    
    /**
     * @brief 地形の色を取得
     * @param type 地形タイプ
     * @return 地形の色
     */
    static Color getTerrainColor(TerrainType type);
    
    /**
     * @brief オブジェクトの色を取得
     * @param objectType オブジェクトタイプ
     * @return オブジェクトの色
     */
    static Color getObjectColor(int objectType);
    
    /**
     * @brief 地形データを取得
     * @param type 地形タイプ
     * @return 地形データ
     */
    static TerrainData getTerrainData(TerrainType type);
}; 