#pragma once
#include <cmath>

namespace GameUtils {
    // 2点間のユークリッド距離を計算
    inline float calculateDistance(int x1, int y1, int x2, int y2) {
        float dx = static_cast<float>(x1 - x2);
        float dy = static_cast<float>(y1 - y2);
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // 2点間のユークリッド距離をチェック
    inline bool isNearPositionEuclidean(int playerX, int playerY, int targetX, int targetY, float maxDistance = 1.5f) {
        return calculateDistance(playerX, playerY, targetX, targetY) <= maxDistance;
    }
    
    // オブジェクトとの衝突チェック（矩形）
    inline bool isColliding(int obj1X, int obj1Y, int obj1Width, int obj1Height,
                           int obj2X, int obj2Y, int obj2Width, int obj2Height) {
        return (obj1X < obj2X + obj2Width &&
                obj1X + obj1Width > obj2X &&
                obj1Y < obj2Y + obj2Height &&
                obj1Y + obj1Height > obj2Y);
    }
    
    // 角度を計算（ラジアン）
    inline float calculateAngle(int fromX, int fromY, int toX, int toY) {
        return std::atan2(static_cast<float>(toY - fromY), static_cast<float>(toX - fromX));
    }
    
    // 指定された角度と距離で移動した位置を計算
    inline std::pair<int, int> calculatePosition(int startX, int startY, float angle, float distance) {
        int newX = startX + static_cast<int>(std::cos(angle) * distance);
        int newY = startY + static_cast<int>(std::sin(angle) * distance);
        return {newX, newY};
    }
    
    // ランダムな方向を生成
    inline float randomAngle() {
        return static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
    }
    
    // 指定された範囲内のランダムな位置を生成
    inline std::pair<int, int> randomPosition(int minX, int minY, int maxX, int maxY) {
        int x = minX + rand() % (maxX - minX);
        int y = minY + rand() % (maxY - minY);
        return {x, y};
    }
} 