/**
 * @file BattleUI.h
 * @brief UI描画を担当するクラス
 * @details 戦闘中のUI描画（読み合い判定、コマンド選択、結果発表など）を管理する。
 * 単一責任の原則に従い、UI描画関連の処理をBattleStateから分離している。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "BattleLogic.h"
#include "BattleAnimationController.h"
#include "BattleConstants.h"
#include <vector>
#include <string>
#include <memory>

// JudgeSubPhaseはBattleConstants.hで定義されている

/**
 * @brief UI描画を担当するクラス
 * @details 戦闘中のUI描画（読み合い判定、コマンド選択、結果発表など）を管理する。
 * 単一責任の原則に従い、UI描画関連の処理をBattleStateから分離している。
 */
class BattleUI {
public:
    /**
     * @brief 読み合い判定描画のパラメータを格納する構造体
     * @details 読み合い判定UIの描画に必要な情報を保持する。
     */
    struct JudgeRenderParams {
        int currentJudgingTurnIndex;  /**< @brief 現在判定中のターンインデックス（0から開始） */
        int commandTurnCount;
        JudgeSubPhase judgeSubPhase;
        float judgeDisplayTimer;  /**< @brief タイマー（秒） */
    };
    
    /**
     * @brief コマンド選択UI描画のパラメータを格納する構造体
     * @details コマンド選択UIの描画に必要な情報を保持する。
     */
    struct CommandSelectRenderParams {
        int currentSelectingTurn;  /**< @brief 現在選択中のターンインデックス（0から開始） */
        int commandTurnCount;
        bool isDesperateMode;
        int selectedOption;
        const std::vector<std::string>* currentOptions;  /**< @brief ポインタに変更（参照はデフォルトコンストラクタを削除するため） */
    };
    
    /**
     * @brief 結果発表描画のパラメータを格納する構造体
     * @details 結果発表UIの描画に必要な情報を保持する。
     */
    struct ResultAnnouncementRenderParams {
        bool isVictory;
        bool isDefeat;
        bool isDesperateMode;
        bool hasThreeWinStreak;
        int playerWins;
        int enemyWins;
    };

private:
    Graphics* graphics;
    std::shared_ptr<Player> player;
    Enemy* enemy;
    BattleLogic* battleLogic;
    BattleAnimationController* animationController;

public:
    /**
     * @brief コンストラクタ
     * @details グラフィックス、プレイヤー、敵、戦闘ロジック、アニメーションコントローラーへの
     * 参照を保持し、UI描画の準備を行う。
     * 
     * @param graphics グラフィックスオブジェクトへのポインタ
     * @param player プレイヤーへの共有ポインタ
     * @param enemy 敵へのポインタ
     * @param battleLogic 戦闘ロジックへのポインタ
     * @param animationController アニメーションコントローラーへのポインタ
     */
    BattleUI(Graphics* graphics, std::shared_ptr<Player> player, Enemy* enemy,
             BattleLogic* battleLogic, BattleAnimationController* animationController);
    
    /**
     * @brief 読み合い判定の視覚的演出
     * @details プレイヤーと敵のコマンドを左右に表示し、中央に勝敗結果を表示する。
     * スライドインアニメーションとVS表示を含む。
     * 
     * @param params 描画パラメータ（現在のターン、サブフェーズ、タイマーなど）
     */
    void renderJudgeAnimation(const JudgeRenderParams& params);
    
    /**
     * @brief コマンド選択UIの視覚的演出
     * @details コマンド選択画面を描画する。選択中のコマンドは強調表示され、
     * スライドインアニメーションが適用される。
     * 
     * @param params 描画パラメータ（現在のターン、選択オプションなど）
     */
    void renderCommandSelectionUI(const CommandSelectRenderParams& params);
    
    /**
     * @brief 結果発表の視覚的演出
     * @details 戦闘結果を大きく表示する。勝利時は星のエフェクト、敗北時は暗いオーバーレイが
     * 適用される。スケール、回転、揺れなどのアニメーション効果を含む。
     * 
     * @param params 描画パラメータ（勝利/敗北フラグ、勝利数など）
     */
    void renderResultAnnouncement(const ResultAnnouncementRenderParams& params);
    
    /**
     * @brief キャラクター描画（共通）
     * @details プレイヤーと敵のキャラクターを指定された位置とサイズで描画する。
     * 画面揺れのオフセットも適用される。
     * 
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param enemyX 敵のX座標
     * @param enemyY 敵のY座標
     * @param playerWidth プレイヤーの幅
     * @param playerHeight プレイヤーの高さ
     * @param enemyWidth 敵の幅
     * @param enemyHeight 敵の高さ
     */
    void renderCharacters(int playerX, int playerY, int enemyX, int enemyY,
                          int playerWidth, int playerHeight, int enemyWidth, int enemyHeight);
    
    /**
     * @brief HP表示（共通）
     * @details プレイヤーと敵のHPバーをキャラクターの頭上に表示する。
     * 現在HPと最大HPの比率に応じてバーの長さが変わる。
     * 
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param enemyX 敵のX座標
     * @param enemyY 敵のY座標
     * @param playerHeight プレイヤーの高さ
     * @param enemyHeight 敵の高さ
     */
    void renderHP(int playerX, int playerY, int enemyX, int enemyY,
                  int playerHeight, int enemyHeight);
    
    /**
     * @brief ターン数表示（共通）
     * @details 現在のターン数と総ターン数を画面左上に表示する。
     * 窮地モード時は特別な表示が適用される。
     * 
     * @param turnNumber 現在のターン数
     * @param totalTurns 総ターン数
     * @param isDesperateMode 窮地モードフラグ（デフォルト: false）
     */
    void renderTurnNumber(int turnNumber, int totalTurns, bool isDesperateMode = false);
};

