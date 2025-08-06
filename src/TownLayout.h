#pragma once
#include <vector>
#include <string>
#include <utility>

// 街の共通配置データ
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
}; 