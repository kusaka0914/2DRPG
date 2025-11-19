/**
 * @file BattleEffectManager.h
 * @brief エフェクト管理を担当するクラス
 * @details ヒットエフェクト、画面揺れ効果を管理する。
 * 単一責任の原則に従い、エフェクト関連の処理をBattleStateから分離している。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "BattleConstants.h"
#include <vector>
#include <memory>

/**
 * @brief エフェクト管理を担当するクラス
 * @details ヒットエフェクト、画面揺れ効果を管理する。
 * 単一責任の原則に従い、エフェクト関連の処理をBattleStateから分離している。
 */
class BattleEffectManager {
public:
    /**
     * @brief ヒットエフェクトの状態を格納する構造体
     * @details ダメージ表示、パーティクルエフェクト、アニメーション状態を保持する。
     */
    struct HitEffect {
        float timer;  /**< @brief 残り時間（秒） */
        float x;
        float y;
        int damage;
        bool isPlayerHit;
        float scale;
        float rotation;  /**< @brief 回転角度（度） */
        float alpha;  /**< @brief 透明度（0.0-1.0） */
        std::vector<float> particlesX;
        std::vector<float> particlesY;
        std::vector<float> particlesVX;
        std::vector<float> particlesVY;
        std::vector<float> particlesLife;
    };
    
    /**
     * @brief 画面揺れの状態を格納する構造体
     * @details 画面揺れのオフセット、タイマー、強度、種類を保持する。
     */
    struct ScreenShakeState {
        float shakeOffsetX;  /**< @brief X方向オフセット（ピクセル） */
        float shakeOffsetY;  /**< @brief Y方向オフセット（ピクセル） */
        float shakeTimer;  /**< @brief 残り時間（秒） */
        float shakeIntensity;  /**< @brief 強度（ピクセル単位） */
        bool isVictoryShake;  /**< @brief 勝利時の揺れか（パルス効果が適用される） */
        bool shakeTargetPlayer;
    };

private:
    std::vector<HitEffect> hitEffects;
    ScreenShakeState shakeState;

public:
    /**
     * @brief コンストラクタ
     * @details ヒットエフェクトと画面揺れの状態を初期化する。
     */
    BattleEffectManager();
    
    /**
     * @brief ヒットエフェクトのトリガー
     * @details ダメージを受けた位置にヒットエフェクトを生成する。
     * エフェクトには画面フラッシュ、パーティクル、ダメージ数値表示などが含まれる。
     * 
     * @param damage ダメージ値
     * @param x エフェクトのX座標
     * @param y エフェクトのY座標
     * @param isPlayerHit プレイヤーがダメージを受けたか
     */
    void triggerHitEffect(int damage, float x, float y, bool isPlayerHit);
    
    /**
     * @brief ヒットエフェクトの更新
     * @details 全てのアクティブなヒットエフェクトのタイマー、パーティクル、アニメーションを更新する。
     * 寿命が尽きたエフェクトは自動的に削除される。
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void updateHitEffects(float deltaTime);
    
    /**
     * @brief ヒットエフェクトの描画
     * @details 全てのアクティブなヒットエフェクトを描画する。
     * パーティクル、ダメージ数値、グロー効果などを描画する。
     * 
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void renderHitEffects(Graphics& graphics);
    
    /**
     * @brief ヒットエフェクトのクリア
     * @details 全てのアクティブなヒットエフェクトを削除する。
     */
    void clearHitEffects();
    
    /**
     * @brief 画面揺れのトリガー
     * @details 画面揺れ効果を開始する。勝利時はパルス効果、通常時はランダムな揺れが適用される。
     * 
     * @param intensity 揺れの強度（ピクセル単位）
     * @param duration 揺れの持続時間（秒）
     * @param victoryShake 勝利時の揺れか（デフォルト: false）
     * @param targetPlayer プレイヤーをターゲットにするか（デフォルト: false）
     */
    void triggerScreenShake(float intensity, float duration, bool victoryShake = false, bool targetPlayer = false);
    
    /**
     * @brief 画面揺れの更新
     * @details 画面揺れのタイマーとオフセットを更新する。
     * 持続時間が過ぎると自動的に停止する。
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void updateScreenShake(float deltaTime);
    ScreenShakeState& getShakeState() { return shakeState; }
    const ScreenShakeState& getShakeState() const { return shakeState; }
    
    /**
     * @brief 全エフェクト状態のリセット
     * @details 全てのヒットエフェクトと画面揺れの状態をリセットする。
     */
    void resetAll();
};

