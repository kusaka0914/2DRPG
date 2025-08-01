#include "MapTerrain.h"
#include <random>
#include <algorithm>
#include <cmath>

std::vector<std::vector<MapTile>> MapGenerator::generateRealisticMap(int width, int height) {
    // 基本の草原マップを作成
    std::vector<std::vector<MapTile>> map(height, std::vector<MapTile>(width));
    
    // 基本地形を草原で初期化
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            map[y][x] = MapTile(TerrainType::GRASS);
        }
    }
    
    // 川を追加
    addRiver(map, width, height);
    
    // 森を追加
    addForest(map, width, height);
    
    // 山を追加
    addMountains(map, width, height);
    
    // 道路を追加
    addRoads(map, width, height);
    
    // ランダムオブジェクトを追加
    addRandomObjects(map, width, height);
    
    // 地形を滑らかにする
    smoothTerrain(map, width, height);
    
    // 街の入り口を設定
    if (width > 24 && height > 9) {
        map[9][24].terrain = TerrainType::TOWN_ENTRANCE;
    }
    
    return map;
}

void MapGenerator::addRiver(std::vector<std::vector<MapTile>>& map, int width, int height) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // 川を蛇行させながら描画
    int riverY = height / 3;
    for (int x = 0; x < width; x++) {
        // 蛇行パターン
        int offset = static_cast<int>(sin(x * 0.3) * 2);
        int currentY = riverY + offset;
        
        if (currentY >= 0 && currentY < height) {
            map[currentY][x].terrain = TerrainType::WATER;
            
            // 川幅を広げる
            if (currentY + 1 < height) {
                map[currentY + 1][x].terrain = TerrainType::WATER;
            }
        }
    }
    
    // 橋を数箇所に設置
    std::uniform_int_distribution<> bridgeDist(5, width - 5);
    for (int i = 0; i < 2; i++) {
        int bridgeX = bridgeDist(gen);
        for (int y = 0; y < height; y++) {
            if (map[y][bridgeX].terrain == TerrainType::WATER) {
                map[y][bridgeX].terrain = TerrainType::BRIDGE;
            }
        }
    }
}

void MapGenerator::addForest(std::vector<std::vector<MapTile>>& map, int width, int height) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // 森エリアを複数作成
    std::uniform_int_distribution<> forestCount(2, 4);
    int numForests = forestCount(gen);
    
    for (int f = 0; f < numForests; f++) {
        std::uniform_int_distribution<> xDist(2, width - 8);
        std::uniform_int_distribution<> yDist(2, height - 8);
        std::uniform_int_distribution<> sizeDist(3, 6);
        
        int centerX = xDist(gen);
        int centerY = yDist(gen);
        int forestSize = sizeDist(gen);
        
        // 円形の森を作成
        for (int y = centerY - forestSize; y <= centerY + forestSize; y++) {
            for (int x = centerX - forestSize; x <= centerX + forestSize; x++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    double distance = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
                    if (distance <= forestSize && map[y][x].terrain == TerrainType::GRASS) {
                        std::uniform_int_distribution<> forestChance(1, 100);
                        if (forestChance(gen) < 80) {  // 80%の確率で森にする
                            map[y][x].terrain = TerrainType::FOREST;
                        }
                    }
                }
            }
        }
    }
}

void MapGenerator::addMountains(std::vector<std::vector<MapTile>>& map, int width, int height) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // 山脈を上部に配置
    std::uniform_int_distribution<> mountainChance(1, 100);
    
    for (int y = 0; y < height / 4; y++) {
        for (int x = 0; x < width; x++) {
            if (map[y][x].terrain == TerrainType::GRASS && mountainChance(gen) < 30) {
                map[y][x].terrain = TerrainType::MOUNTAIN;
            }
        }
    }
    
    // 孤立した山も配置
    std::uniform_int_distribution<> isolatedCount(1, 3);
    int numMountains = isolatedCount(gen);
    
    for (int m = 0; m < numMountains; m++) {
        std::uniform_int_distribution<> xDist(0, width - 1);
        std::uniform_int_distribution<> yDist(height / 2, height - 1);
        
        int x = xDist(gen);
        int y = yDist(gen);
        
        if (map[y][x].terrain == TerrainType::GRASS) {
            map[y][x].terrain = TerrainType::MOUNTAIN;
        }
    }
}

void MapGenerator::addRoads(std::vector<std::vector<MapTile>>& map, int width, int height) {
    // 横道：中央付近に水平道路
    int roadY = height * 2 / 3;
    for (int x = 0; x < width; x++) {
        if (map[roadY][x].terrain != TerrainType::WATER && 
            map[roadY][x].terrain != TerrainType::MOUNTAIN) {
            map[roadY][x].terrain = TerrainType::ROAD;
        }
    }
    
    // 縦道：街への道
    int roadX = width - 5;
    for (int y = roadY; y < height; y++) {
        if (map[y][roadX].terrain != TerrainType::WATER && 
            map[y][roadX].terrain != TerrainType::MOUNTAIN) {
            map[y][roadX].terrain = TerrainType::ROAD;
        }
    }
}

void MapGenerator::addRandomObjects(std::vector<std::vector<MapTile>>& map, int width, int height) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> objChance(1, 100);
    std::uniform_int_distribution<> objType(1, 3);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (map[y][x].terrain == TerrainType::GRASS && objChance(gen) < 10) {
                map[y][x].hasObject = true;
                map[y][x].objectType = objType(gen);  // 1:岩, 2:花, 3:小さな木
            }
        }
    }
}

void MapGenerator::smoothTerrain(std::vector<std::vector<MapTile>>& map, int width, int height) {
    // 孤立した地形を隣接する地形に合わせる簡単なスムージング
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            if (map[y][x].terrain == TerrainType::GRASS) {
                // 周囲8マスの地形をチェック
                int forestCount = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        if (map[y + dy][x + dx].terrain == TerrainType::FOREST) {
                            forestCount++;
                        }
                    }
                }
                
                // 周囲に森が多ければ花畑にする
                if (forestCount >= 3) {
                    static std::random_device rd;
                    static std::mt19937 gen(rd());
                    std::uniform_int_distribution<> flowerChance(1, 100);
                    if (flowerChance(gen) < 40) {
                        map[y][x].terrain = TerrainType::FLOWER_FIELD;
                    }
                }
            }
        }
    }
}

TerrainRenderer::Color TerrainRenderer::getTerrainColor(TerrainType type) {
    switch (type) {
        case TerrainType::GRASS:
            return Color(34, 139, 34);  // 草原緑
        case TerrainType::FOREST:
            return Color(0, 100, 0);    // 深緑
        case TerrainType::MOUNTAIN:
            return Color(139, 137, 137); // グレー
        case TerrainType::WATER:
            return Color(0, 100, 200);   // 青
        case TerrainType::ROAD:
            return Color(139, 69, 19);   // 茶色
        case TerrainType::BRIDGE:
            return Color(160, 82, 45);   // 明るい茶色
        case TerrainType::ROCK:
            return Color(105, 105, 105); // 暗いグレー
        case TerrainType::FLOWER_FIELD:
            return Color(255, 182, 193); // ピンク
        case TerrainType::DESERT:
            return Color(238, 203, 173); // 砂色
        case TerrainType::TOWN_ENTRANCE:
            return Color(218, 165, 32);  // 金色
        default:
            return Color(34, 139, 34);   // デフォルト草原
    }
}

TerrainRenderer::Color TerrainRenderer::getObjectColor(int objectType) {
    switch (objectType) {
        case 1: return Color(105, 105, 105); // 岩 - グレー
        case 2: return Color(255, 20, 147);  // 花 - マゼンタ
        case 3: return Color(34, 139, 34);   // 小さな木 - 緑
        default: return Color(255, 255, 255); // 白
    }
}

TerrainData TerrainRenderer::getTerrainData(TerrainType type) {
    switch (type) {
        case TerrainType::GRASS:
            return TerrainData(type, true, 5);   // 20回に1回程度（5%）
        case TerrainType::FOREST:
            return TerrainData(type, true, 15);  // 森は敵が多い（約7回に1回）
        case TerrainType::MOUNTAIN:
            return TerrainData(type, false, 0);  // 山は歩けない
        case TerrainType::WATER:
            return TerrainData(type, false, 0);  // 水は歩けない
        case TerrainType::ROAD:
            return TerrainData(type, true, 2);   // 道路は非常に安全（50回に1回）
        case TerrainType::BRIDGE:
            return TerrainData(type, true, 0);   // 橋は完全に安全
        case TerrainType::ROCK:
            return TerrainData(type, false, 0);  // 岩は歩けない
        case TerrainType::FLOWER_FIELD:
            return TerrainData(type, true, 3);   // 花畑は安全（30回に1回）
        case TerrainType::DESERT:
            return TerrainData(type, true, 12);  // 砂漠は危険（約8回に1回）
        case TerrainType::TOWN_ENTRANCE:
            return TerrainData(type, true, 0);   // 街の入り口は安全
        default:
            return TerrainData(type, true, 5);
    }
} 