/**
 * @file CommonUI.h
 * @brief 共通UI描画を担当するクラス
 * @details 複数のGameStateで使用される共通のUI要素（夜のタイマー、目標レベル、信頼度など）の描画を管理する。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "../entities/Player.h"
#include <memory>

/**
 * @brief 共通UI描画を担当するクラス
 * @details 複数のGameStateで使用される共通のUI要素（夜のタイマー、目標レベル、信頼度など）の描画を管理する。
 */
class CommonUI {
public:
    /**
     * @brief 夜のタイマーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param nightTimer 夜のタイマー（秒）
     * @param nightTimerActive タイマーがアクティブか
     * @param showGameExplanation ゲーム説明を表示するか
     */
    static void drawNightTimer(Graphics& graphics, float nightTimer, bool nightTimerActive, bool showGameExplanation);
    
    /**
     * @brief 目標レベルの描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param targetLevel 目標レベル
     * @param levelGoalAchieved 目標達成済みか
     * @param currentLevel 現在のレベル
     */
    static void drawTargetLevel(Graphics& graphics, int targetLevel, bool levelGoalAchieved, int currentLevel);
    
    /**
     * @brief 信頼度の描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param player プレイヤーへの共有ポインタ
     * @param nightTimerActive タイマーがアクティブか
     * @param showGameExplanation ゲーム説明を表示するか
     */
    static void drawTrustLevels(Graphics& graphics, std::shared_ptr<Player> player, bool nightTimerActive, bool showGameExplanation);
    
    /**
     * @brief ゲームコントローラーの接続状態の描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param gameControllerConnected ゲームコントローラーが接続されているか
     */
    static void drawGameControllerStatus(Graphics& graphics, bool gameControllerConnected);
}; 