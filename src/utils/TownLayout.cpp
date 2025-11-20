#include "TownLayout.h"
#include <SDL2/SDL.h>
#include <tuple>

const std::vector<std::pair<int, int>> TownLayout::BUILDINGS = {
    {8, 10},   // 道具屋（2x2）
    {18, 10},  // 武器屋（2x2）
    {24, 10},  // 自室（2x2）
    {13, 2}    // 城（画面中央上部、2x2）
};

const std::vector<std::string> TownLayout::BUILDING_TYPES = {
    "shop", "weapon_shop", "house", "castle"
};

const std::vector<std::pair<int, int>> TownLayout::RESIDENT_HOMES = {
    {1, 6}, {5, 5}, {9, 7}, {3, 9}, {17, 7}, {21, 5}, {25, 6}, {4, 12}, {22, 12},{1,13},{17,13},{18,3}
};

const std::vector<std::pair<int, int>> TownLayout::RESIDENTS = {
    {3, 7},   // 町の住人1
    {7, 6},   // 町の住人2
    {11, 8},  // 町の住人3
    {5, 10},  // 町の住人4
    {19, 8},  // 町の住人5
    {23, 6},  // 町の住人6
    {27, 7},  // 町の住人7
    {6, 13},  // 町の住人8
    {24, 13},  // 町の住人9
    {3, 14},  // 町の住人10
    {19, 14},  // 町の住人11,
    {20, 4}  // 町の住人12
};

const std::vector<std::pair<int, int>> TownLayout::GUARDS = {
    {12, 2},  // 衛兵1
    {12, 3},  // 衛兵2
    {15, 2},   // 衛兵3
    {15, 3}   // 衛兵4
};

int TownLayout::findResidentIndex(int x, int y) {
    for (size_t i = 0; i < RESIDENTS.size(); ++i) {
        if (RESIDENTS[i].first == x && RESIDENTS[i].second == y) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int TownLayout::findGuardIndex(int x, int y) {
    for (size_t i = 0; i < GUARDS.size(); ++i) {
        if (GUARDS[i].first == x && GUARDS[i].second == y) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int TownLayout::getResidentTextureIndex(int x, int y) {
    int index = findResidentIndex(x, y);
    if (index >= 0) {
        return index % 6; // 6つの画像を循環使用
    }
    return 0; // デフォルト
}

std::string TownLayout::getResidentName(int x, int y) {
    static const std::vector<std::string> residentNames = {
        "タカヒロ", "セイカ", "トメコ", "レオ", "タイセイ", "ヒマリ",
        "シンジ", "アヤメ", "ウメコ", "ヒロユキ", "サトシ", "レイラ"
    };
    
    int index = findResidentIndex(x, y);
    if (index >= 0 && index < static_cast<int>(residentNames.size())) {
        return residentNames[index];
    }
    
    return "住民"; // デフォルト
}

std::string TownLayout::getResidentDialogue(int x, int y) {
    static const std::vector<std::string> residentDialogues = {
        "魔物？そんなの俺が追い払ってやるよ",
        "最近は魔物が増えてきてるんだって・・・",
        "そこの赤い鳥居をくぐると外へ出れるわよ",
        "これはこれは勇者様、いつも街を守っていただきありがとうございます！",
        "魔物がいるからってお外で遊べないんだぁ・・・",
        "本当に魔王がいるのかな・・・？",
        "勇者様、決�