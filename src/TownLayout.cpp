#include "TownLayout.h"

// 建物の位置とタイプ
const std::vector<std::pair<int, int>> TownLayout::BUILDINGS = {
    {8, 10},   // 道具屋（2x2）
    {18, 10},  // 武器屋（2x2）
    {24, 10},  // 自室（2x2）
    {13, 2}    // 城（画面中央上部、2x2）
};

const std::vector<std::string> TownLayout::BUILDING_TYPES = {
    "shop", "weapon_shop", "house", "castle"
};

// 住人の家の位置
const std::vector<std::pair<int, int>> TownLayout::RESIDENT_HOMES = {
    {1, 6}, {5, 5}, {9, 7}, {3, 9}, {17, 7}, {21, 5}, {25, 6}, {4, 12}, {22, 12}
};

// 住民の位置
const std::vector<std::pair<int, int>> TownLayout::RESIDENTS = {
    {3, 7},   // 町の住人1
    {7, 6},   // 町の住人2
    {11, 8},  // 町の住人3
    {5, 10},  // 町の住人4
    {19, 8},  // 町の住人5
    {21, 13}  // 町の住人6
};

// 衛兵の位置（家の前の定点）
const std::vector<std::pair<int, int>> TownLayout::GUARDS = {
    {12, 2},  // 衛兵1
    {12, 3},  // 衛兵2
    {15, 2},   // 衛兵3
    {15, 3}   // 衛兵4
}; 