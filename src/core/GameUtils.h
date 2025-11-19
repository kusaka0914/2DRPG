/**
 * @file GameUtils.h
 * @brief ゲームユーティリティ関数の名前空間
 * @details 距離計算、衝突判定、角度計算、位置計算などの汎用的な数学関数を提供する。
 */

#pragma once
#include <cmath>

/**
 * @brief ゲームユーティリティ関数の名前空間
 * @details 距離計算、衝突判定、角度計算、位置計算などの汎用的な数学関数を提供する。
 */
namespace GameUtils {
    /**
     * @brief 2点間のユークリッド距離を計算
     * @param x1 点1のX座標
     * @param y1 点1のY座標
     * @param x2 点2のX座標
     * @param y2 点2のY座標
     * @return 距離
     */
    inline float calculateDistance(int x1, int y1, int x2, int y2) {
        float dx = static_cast<float>(x1 - x2);
        float dy = static_cast<float>(y1 - y2);
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief 2点間のユークリッド距離をチェック
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param targetX ターゲットのX座標
     * @param targetY ターゲットのY座標
     * @param maxDistance 最大距離（デフォルト: 1.5）
     * @return 最大距離以内かどうか
     */
    inline bool isNearPositionEuclidean(int playerX, int playerY, int targetX, int targetY, float maxDistance = 1.5f) {
        return calculateDistance(playerX, playerY, targetX, targetY) <= maxDistance;
    }
    
    /**
     * @brief オブジェクトとの衝突チェック（矩形）
     * @param obj1X オブジェクト1のX座標
     * @param obj1Y オブジェクト1のY座標
     * @param obj1Width オブジェクト1の幅
     * @param obj1Height オブジェクト1の高さ
     * @param obj2X オブジェクト2のX座標
     * @param obj2Y オブジェクト2のY座標
     * @param obj2Width オブジェクト2の幅
     * @param obj2Height オブジェクト2の高さ
     * @return 衝突しているか
     */
    inline bool isColliding(int obj1X, int obj1Y, int obj1Width, int obj1Height,
                           int obj2X, int obj2Y, int obj2Width, int obj2Height) {
        return (obj1X < obj2X + obj2Width &&
                obj1X + obj1Width > obj2X &&
                obj1Y < obj2Y + obj2Height &&
                obj1Y + obj1Height > obj2Y);
    }
    
    /**
     * @brief 角度を計算（ラジアン）
     * @param fromX 始点のX座標
     * @param fromY 始点のY座標
     * @param toX 終点のX座標
     * @param toY 終点のY座標
     * @return 角度（ラジアン）
     */
    inline float calculateAngle(int fromX, int fromY, int toX, int toY) {
        return std::atan2(static_cast<float>(toY - fromY), static_cast<float>(toX - fromX));
    }
    
    /**
     * @brief 指定された角度と距離で移動した位置を計算
     * @param startX 開始X座標
     * @param startY 開始Y座標
     * @param angle 角度（ラジアン）
     * @param distance 距離
     * @return 新しい位置（X, Y）
     */
    inline std::pair<int, int> calculatePosition(int startX, int startY, float angle, float distance) {
        int newX = startX + static_cast<int>(std::cos(angle) * distance);
        int newY = startY + static_cast<int>(std::sin(angle) * distance);
        return {newX, newY};
    }
    
    /**
     * @brief ランダムな方向を生成
     * @return ランダムな角度（ラジアン、0～2π）
     */
    inline float randomAngle() {
        return static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
    }
    
    /**
     * @brief 指定された範囲内のランダムな位置を生成
     * @param minX 最小X座標
     * @param minY 最小Y座標
     * @param maxX 最大X座標
     * @param maxY 最大Y座標
     * @return ランダムな位置（X, Y）
     */
    inline std::pair<int, int> randomPosition(int minX, int minY, int maxX, int maxY) {
        int x = minX + rand() % (maxX - minX);
        int y = minY + rand() % (maxY - minY);
        return {x, y};
    }
} 