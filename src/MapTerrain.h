#pragma once
#include <vector>
#include <string>

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
    TOWN_ENTRANCE   // 街の入り口
};

struct TerrainData {
    TerrainType type;
    bool walkable;
    int encounterRate;  // エンカウント率（0-100）
    
    TerrainData(TerrainType t, bool w, int e) 
        : type(t), walkable(w), encounterRate(e) {}
};

struct MapTile {
    TerrainType terrain;
    bool hasObject;      // 木や岩などのオブジェクトがあるか
    int objectType;      // オブジェクトの種類
    
    MapTile(TerrainType t = TerrainType::GRASS, bool obj = false, int objType = 0)
        : terrain(t), hasObject(obj), objectType(objType) {}
};

class MapGenerator {
public:
    static std::vector<std::vector<MapTile>> generateRealisticMap(int width, int height);
    
private:
    static void addRiver(std::vector<std::vector<MapTile>>& map, int width, int height);
    static void addForest(std::vector<std::vector<MapTile>>& map, int width, int height);
    static void addMountains(std::vector<std::vector<MapTile>>& map, int width, int height);
    static void addRoads(std::vector<std::vector<MapTile>>& map, int width, int height);
    static void addRandomObjects(std::vector<std::vector<MapTile>>& map, int width, int height);
    static void smoothTerrain(std::vector<std::vector<MapTile>>& map, int width, int height);
};

class TerrainRenderer {
public:
    struct Color {
        int r, g, b, a;
        Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {}
    };
    
    static Color getTerrainColor(TerrainType type);
    static Color getObjectColor(int objectType);
    static TerrainData getTerrainData(TerrainType type);
}; 