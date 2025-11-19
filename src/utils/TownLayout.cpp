#include "TownLayout.h"

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
        "最近魔物が増えて困っているんだ...",
        "今日は良い天気だね。",
        "王様の城は立派だね。",
        "冒険者さん、頑張ってね！",
        "街は平和でいいね。",
        "今日も気持ちがいい天気！",
        "最近魔物が増えて困っているんだ...",
        "今日は良い天気だね。",
        "王様の城は立派だね。",
        "冒険者さん、頑張ってね！",
        "街は平和でいいね。",
        "今日も気持ちがいい天気！"
    };
    
    int index = findResidentIndex(x, y);
    if (index >= 0 && index < static_cast<int>(residentDialogues.size())) {
        return residentDialogues[index];
    }
    
    return "..."; // デフォルト
}

std::string TownLayout::getGuardName(int x, int y) {
    static const std::vector<std::string> guardNames = {
        "コバヤシ", "タナカ", "マツオ", "サカキバラ"
    };
    
    int index = findGuardIndex(x, y);
    if (index >= 0 && index < static_cast<int>(guardNames.size())) {
        return guardNames[index];
    }
    
    return "衛兵"; // デフォルト
}

std::string TownLayout::getGuardDialogue(int x, int y) {
    static const std::vector<std::string> guardDialogues = {
        "町の平和を守るのが私の仕事だ！",
        "何か困ったことがあれば声をかけてくれ。",
        "街の見回りは大切な仕事だ。",
        "町の平和を守るのが私の仕事だ！"
    };
    
    int index = findGuardIndex(x, y);
    if (index >= 0 && index < static_cast<int>(guardDialogues.size())) {
        return guardDialogues[index];
    }
    
    return "..."; // デフォルト
}

bool TownLayout::isResidentKilled(int x, int y, const std::vector<std::pair<int, int>>& killedResidents) {
    for (const auto& killedPos : killedResidents) {
        if (killedPos.first == x && killedPos.second == y) {
            return true;
        }
    }
    return false;
}

bool TownLayout::areAllResidentsKilled(const std::vector<std::pair<int, int>>& killedResidents) {
    for (const auto& residentPos : RESIDENTS) {
        if (!isResidentKilled(residentPos.first, residentPos.second, killedResidents)) {
            return false;
        }
    }
    return true;
}

void TownLayout::removeDuplicatePositions(std::vector<std::pair<int, int>>& positions) {
    std::vector<std::pair<int, int>> result;
    for (const auto& pos : positions) {
        bool found = false;
        for (const auto& existingPos : result) {
            if (existingPos.first == pos.first && existingPos.second == pos.second) {
                found = true;
                break;
            }
        }
        if (!found) {
            result.push_back(pos);
        }
    }
    positions = result;
} 