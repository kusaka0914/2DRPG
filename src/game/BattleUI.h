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
        
        // 住民戦用：コマンド名を直接指定（空文字列の場合は従来通りbattleLogicから取得）
        std::string playerCommandName;  /**< @brief プレイヤーコマンド名（住民戦用） */
        std::string enemyCommandName;   /**< @brief 敵コマンド名（住民戦用） */
        int judgeResult;  /**< @brief 判定結果（住民戦用、-999=未設定の場合はbattleLogicから取得） */
        std::string residentBehaviorHint;  /**< @brief 住民の様子（住民戦用、空文字列の場合は通常のヒントを表示） */
        int residentTurnCount;  /**< @brief 住民戦の現在のターン数（住民戦用、0の場合は通常の表示） */
        int residentHitCount;  /**< @brief 住民戦で攻撃が成功した回数（住民戦用、0の場合は通常の表示） */
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
        std::string residentBehaviorHint;  /**< @brief 住民の様子（住民戦用、空文字列の場合は通常のヒントを表示） */
        int residentTurnCount;  /**< @brief 住民戦の現在のターン数（住民戦用、0の場合は通常の表示） */
        int residentHitCount;  /**< @brief 住民戦で攻撃が成功した回数（住民戦用、0の場合は通常の表示） */
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
        std::string residentBehaviorHint;  /**< @brief 住民の様子（住民戦用、空文字列の場合は通常のヒントを表示） */
        int residentHitCount;  /**< @brief 住民戦で攻撃が成功した回数（住民戦用、0の場合は通常の表示） */
    };

private:
    Graphics* graphics;
    std::shared_ptr<Player> player;
    Enemy* enemy;
    BattleLogic* battleLogic;
    BattleAnimationController* animationController;
    bool hasUsedLastChanceMode;  /**< @brief 最後のチャンスモードを使用したか */

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
     * @brief 最後のチャンスモード使用フラグを設定
     * @param hasUsed 最後のチャンスモードを使用したか
     */
    void setHasUsedLastChanceMode(bool hasUsed);
    
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
     * @brief 戦闘背景画像の取得
     * @return 背景画像のテクスチャ（住民の場合は夜の背景、それ以外は通常の戦闘背景）
     */
    SDL_Texture* getBattleBackgroundTexture() const;
    
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
     * 住民戦の場合は、通常のヒント表示の代わりに住民の様子を表示する。
     * 住民戦の場合は、住民名を少し上に移動し、その下にlife.pngを3つ横並びで表示する。
     * 
     * @param playerX プレイヤーのX座標
     * @param playerY プレイヤーのY座標
     * @param enemyX 敵のX座標
     * @param enemyY 敵のY座標
     * @param playerHeight プレイヤーの高さ
     * @param enemyHeight 敵の高さ
     * @param residentBehaviorHint 住民の様子（住民戦の場合のみ、デフォルトは空文字列）
     * @param hideEnemyUI 敵のUIを非表示にするか（デフォルト: false）
     * @param residentHitCount 住民戦で攻撃が成功した回数（住民戦の場合のみ、デフォルト: 0）
     */
    void renderHP(int playerX, int playerY, int enemyX, int enemyY,
                  int playerHeight, int enemyHeight, const std::string& residentBehaviorHint = "", bool hideEnemyUI = false, int residentHitCount = 0);
    
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
    
    /**
     * @brief コマンド名から画像テクスチャを取得
     * @param commandName コマンド名（"攻撃"、"防御"、"呪文"など）
     * @return コマンド画像のテクスチャ（見つからない場合はnullptr）
     */
    SDL_Texture* getCommandTexture(const std::string& commandName) const;
    
    /**
     * @brief コマンド画像を描画
     * @param commandName コマンド名（"攻撃"、"防御"、"呪文"など）
     * @param x X座標
     * @param y Y座標
     * @param width 表示幅（0の場合は元のサイズ）
     * @param height 表示高さ（0の場合は元のサイズ）
     */
    void drawCommandImage(const std::string& commandName, int x, int y, int width = 0, int height = 0) const;
};

