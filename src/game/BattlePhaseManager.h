/**
 * @file BattlePhaseManager.h
 * @brief フェーズ遷移管理を担当するクラス
 * @details 戦闘フェーズの遷移条件を判定し、次のフェーズへの遷移を管理する。
 */

#pragma once
#include "BattleConstants.h"
#include "BattleLogic.h"
#include <memory>

class Player;
class Enemy;
enum class BattlePhase; // 前方宣言

/**
 * @brief フェーズ遷移管理を担当するクラス
 * @details 戦闘フェーズの遷移条件を判定し、次のフェーズへの遷移を管理する。
 * 単一責任の原則に従い、フェーズ遷移ロジックをBattleStateから分離している。
 */
class BattlePhaseManager {
public:
    /**
     * @brief フェーズ遷移結果を格納する構造体
     * @details フェーズ遷移の判定結果と、次のフェーズへの遷移情報を保持する。
     */
    struct PhaseTransitionResult {
        BattlePhase nextPhase;
        bool shouldTransition;
        bool resetTimer;
    };
    
    /**
     * @brief フェーズ更新コンテキストを格納する構造体
     * @details 現在のフェーズ状態と、遷移判定に必要な情報を保持する。
     */
    struct PhaseUpdateContext {
        BattlePhase currentPhase;
        float phaseTimer;  /**< @brief 経過時間（秒） */
        int currentSelectingTurn;
        int currentJudgingTurnIndex;
        int currentExecutingTurn;
        JudgeSubPhase judgeSubPhase;
        float judgeDisplayTimer;  /**< @brief タイマー（秒） */
        float executeDelayTimer;  /**< @brief タイマー（秒） */
        bool isFirstCommandSelection;
        bool isShowingOptions;
        bool damageAppliedInAnimation;
        size_t pendingDamagesSize;
    };

private:
    BattleLogic* battleLogic;
    Player* player;
    Enemy* enemy;

public:
    /**
     * @brief コンストラクタ
     * @details 戦闘ロジック、プレイヤー、敵への参照を保持し、フェーズ遷移管理の準備を行う。
     * 
     * @param battleLogic 戦闘ロジックへのポインタ
     * @param player プレイヤーへのポインタ
     * @param enemy 敵へのポインタ
     */
    BattlePhaseManager(BattleLogic* battleLogic, Player* player, Enemy* enemy);
    
    /**
     * @brief フェーズ更新（各フェーズの処理を実行し、必要に応じて遷移を返す）
     * @details 現在のフェーズに応じて処理を実行し、遷移条件を満たしている場合は
     * 次のフェーズへの遷移情報を返す。
     * 
     * @param context フェーズ更新コンテキスト（現在のフェーズ、タイマー、状態など）
     * @param deltaTime 前フレームからの経過時間（秒）
     * @return フェーズ遷移結果（次のフェーズ、遷移フラグ、タイマーリセットフラグ）
     */
    PhaseTransitionResult updatePhase(const PhaseUpdateContext& context, float deltaTime);
    
    /**
     * @brief フェーズ遷移の判定（各フェーズから次のフェーズへの遷移条件をチェック）
     * @details 現在のフェーズとコンテキストに基づいて、次のフェーズへの遷移条件を
     * 満たしているかどうかを判定する。
     * 
     * @param currentPhase 現在のフェーズ
     * @param context フェーズ更新コンテキスト
     * @return 遷移すべきか
     */
    bool shouldTransitionToNextPhase(BattlePhase currentPhase, const PhaseUpdateContext& context) const;
    
    /**
     * @brief 次のフェーズを取得
     * @details 現在のフェーズから、遷移先のフェーズを決定する。
     * 
     * @param currentPhase 現在のフェーズ
     * @param context フェーズ更新コンテキスト
     * @return 次のフェーズ
     */
    BattlePhase getNextPhase(BattlePhase currentPhase, const PhaseUpdateContext& context) const;
};

