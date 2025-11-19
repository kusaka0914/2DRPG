/**
 * @file BattleAnimationController.h
 * @brief アニメーション管理を担当するクラス
 * @details 戦闘中のキャラクターアニメーション、結果発表アニメーション、コマンド選択アニメーションを管理する。
 * 単一責任の原則に従い、アニメーション関連の処理をBattleStateから分離している。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "BattleConstants.h"
#include <memory>

class Player;
class Enemy;

/**
 * @brief アニメーション管理を担当するクラス
 * @details 戦闘中のキャラクターアニメーション、結果発表アニメーション、コマンド選択アニメーションを管理する。
 * 単一責任の原則に従い、アニメーション関連の処理をBattleStateから分離している。
 */
class BattleAnimationController {
public:
    /**
     * @brief キャラクターアニメーションの状態を格納する構造体
     * @details プレイヤーと敵の攻撃・被弾時のオフセット、アニメーションフレームを保持する。
     */
    struct CharacterAnimationState {
        float playerAttackOffsetX;
        float playerAttackOffsetY;
        float enemyAttackOffsetX;
        float enemyAttackOffsetY;
        float enemyHitOffsetX;
        float enemyHitOffsetY;
        float playerHitOffsetX;
        float playerHitOffsetY;
        int attackAnimationFrame;
    };
    
    /**
     * @brief 結果発表アニメーションの状態を格納する構造体
     * @details 結果発表画面のタイマー、スケール、回転、揺れ状態を保持する。
     */
    struct ResultAnimationState {
        float resultAnimationTimer;  /**< @brief タイマー（秒） */
        float resultScale;
        float resultRotation;  /**< @brief 回転角度（度） */
        bool resultShakeActive;
    };
    
    /**
     * @brief コマンド選択アニメーションの状態を格納する構造体
     * @details コマンド選択UIのアニメーションタイマーとスライド進行度を保持する。
     */
    struct CommandSelectAnimationState {
        float commandSelectAnimationTimer;  /**< @brief タイマー（秒） */
        float commandSelectSlideProgress;  /**< @brief スライド進行度（0.0-1.0） */
    };

private:
    CharacterAnimationState characterState;
    ResultAnimationState resultState;
    CommandSelectAnimationState commandSelectState;

public:
    /**
     * @brief コンストラクタ
     * @details 全てのアニメーション状態を初期値にリセットする。
     */
    BattleAnimationController();
    
    /**
     * @brief 結果発表時のキャラクターアニメーション更新
     * @details 戦闘結果に応じて、プレイヤーと敵の攻撃・被弾アニメーションを更新する。
     * 勝利時はプレイヤーが攻撃し、敗北時は敵が攻撃する。アニメーションのピーク時に
     * ダメージを適用するタイミングを検出する。
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     * @param isVictory プレイヤー勝利フラグ
     * @param isDefeat プレイヤー敗北フラグ
     * @param resultTimer 結果発表タイマー
     * @param damageAppliedInAnimation アニメーション中にダメージが適用されたか（参照渡し）
     * @param hasThreeWinStreak 3連勝フラグ（アニメーションの強度に影響）
     * @param animStartTime アニメーション開始時間（デフォルト: 0.5f）
     * @param animDuration アニメーション継続時間（デフォルト: 1.0f）
     */
    void updateResultCharacterAnimation(float deltaTime, bool isVictory, bool isDefeat, 
                                        float resultTimer, bool& damageAppliedInAnimation,
                                        bool hasThreeWinStreak,
                                        float animStartTime = 0.5f, float animDuration = 1.0f);
    CharacterAnimationState& getCharacterState() { return characterState; }
    const CharacterAnimationState& getCharacterState() const { return characterState; }
    
    /**
     * @brief 結果発表アニメーション更新
     * @details 結果発表画面のスケール、回転、揺れなどのアニメーション効果を更新する。
     * 窮地モード時はより激しいアニメーション効果が適用される。
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     * @param isDesperateMode 窮地モードフラグ
     */
    void updateResultAnimation(float deltaTime, bool isDesperateMode);
    ResultAnimationState& getResultState() { return resultState; }
    const ResultAnimationState& getResultState() const { return resultState; }
    
    /**
     * @brief 結果発表アニメーションのリセット
     * @details 結果発表アニメーションの状態を初期値にリセットする。
     */
    void resetResultAnimation();
    
    /**
     * @brief コマンド選択アニメーション更新
     * @details コマンド選択UIのスライドインアニメーションを更新する。
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void updateCommandSelectAnimation(float deltaTime);
    CommandSelectAnimationState& getCommandSelectState() { return commandSelectState; }
    const CommandSelectAnimationState& getCommandSelectState() const { return commandSelectState; }
    
    /**
     * @brief コマンド選択アニメーションのリセット
     * @details コマンド選択アニメーションの状態を初期値にリセットする。
     */
    void resetCommandSelectAnimation();
    
    /**
     * @brief 全アニメーション状態のリセット
     * @details 全てのアニメーション状態（キャラクター、結果発表、コマンド選択）を
     * 初期値にリセットする。
     */
    void resetAll();
};

