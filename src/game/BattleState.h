/**
 * @file BattleState.h
 * @brief 戦闘状態を担当するクラス
 * @details 新しい読み合いシステム（3ターン分のコマンドを事前選択し、読み合いで勝敗を決める）を実装する。
 * 単一責任の原則に従い、戦闘ロジックはBattleLogic、アニメーション管理はBattleAnimationController、
 * エフェクト管理はBattleEffectManager、UI描画はBattleUI、フェーズ遷移管理はBattlePhaseManagerに分離している。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../core/Battle.h"
#include "BattleConstants.h"
#include "BattleLogic.h"
#include "BattleAnimationController.h"
#include "BattleEffectManager.h"
#include "BattleUI.h"
#include "BattlePhaseManager.h"
#include <memory>

/**
 * @brief 戦闘フェーズの種類
 * @details 戦闘の進行状況を表すフェーズを定義する。
 */
enum class BattlePhase {
    INTRO,
    COMMAND_SELECT,         // 3ターン分のコマンド選択（通常）
    JUDGE,                  // 読み合い判定（各ターン結果を順に表示）
    JUDGE_RESULT,           // 結果発表
    EXECUTE,                // 勝敗による実行
    DESPERATE_MODE_PROMPT,  // 窮地モード発動確認
    DESPERATE_COMMAND_SELECT, // 6ターン分のコマンド選択
    DESPERATE_JUDGE,        // 読み合い判定（6ターン、各ターン結果を順に表示）
    DESPERATE_JUDGE_RESULT, // 結果発表（窮地モード）
    DESPERATE_EXECUTE,      /**< @brief 実行（ダメージ倍率あり） */
    // 既存のフェーズ（後方互換性のため残す）
    PLAYER_TURN,            /**< @brief プレイヤーのターン（古いシステム） */
    PLAYER_ATTACK_DISPLAY,
    SPELL_SELECTION,
    ITEM_SELECTION,
    ENEMY_TURN,
    ENEMY_TURN_DISPLAY,
    VICTORY_DISPLAY,
    RESULT,
    LEVEL_UP_DISPLAY,
    END
};

/**
 * @brief ターンコマンドの種類
 * @details プレイヤーが選択可能なコマンドの種類を定義する。
 */
enum class TurnCommand {
    ATTACK,  /**< @brief 攻撃 */
    DEFEND,  /**< @brief 防御 */
    SPELL    /**< @brief 呪文 */
};

// JudgeSubPhaseはBattleConstants.hで定義されている

/**
 * @brief 戦闘状態を担当するクラス
 * @details 新しい読み合いシステム（3ターン分のコマンドを事前選択し、読み合いで勝敗を決める）を実装する。
 * 単一責任の原則に従い、戦闘ロジックはBattleLogic、アニメーション管理はBattleAnimationController、
 * エフェクト管理はBattleEffectManager、UI描画はBattleUI、フェーズ遷移管理はBattlePhaseManagerに分離している。
 */
class BattleState : public GameState {
private:
    std::shared_ptr<Player> player;
    std::unique_ptr<Enemy> enemy;
    std::unique_ptr<Battle> battle;
    UIManager ui;
    
    BattlePhase currentPhase;
    float phaseTimer;
    
    // UI要素への参照
    Label* battleLogLabel;
    Label* playerStatusLabel;
    Label* enemyStatusLabel;
    Label* messageLabel;  // 重要なメッセージ表示用
    
    // 戦闘ログ
    std::string battleLog;
    int logScrollOffset;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    
    BattleResult lastResult;
    
    // レベルアップ情報
    bool hasLeveledUp;
    int oldLevel;
    int oldMaxHp, oldMaxMp, oldAttack, oldDefense;
    
    // 防御状態
    bool playerDefending;

    std::vector<std::string> currentOptions;
    int selectedOption;
    bool isShowingOptions;
    bool isShowingMessage;
    
    // 戦闘ロジック（単一責任の原則）
    std::unique_ptr<BattleLogic> battleLogic;
    
    // アニメーション管理（単一責任の原則）
    std::unique_ptr<BattleAnimationController> animationController;
    
    // エフェクト管理（単一責任の原則）
    std::unique_ptr<BattleEffectManager> effectManager;
    
    // UI描画管理（単一責任の原則）
    std::unique_ptr<BattleUI> battleUI;
    
    // フェーズ遷移管理（単一責任の原則）
    std::unique_ptr<BattlePhaseManager> phaseManager;
    
    // 新しいコマンド選択システム
    // isDesperateModeはBattleLogicで管理（単一責任の原則）
    int currentSelectingTurn;          // 現在選択中のターン（0から開始）
    bool isFirstCommandSelection;      // 最初のコマンド選択か（窮地モード判定用）
    
    // EXECUTEフェーズ用
    int currentExecutingTurn;          // 現在実行中のターン（0から開始）
    std::vector<BattleLogic::DamageInfo> pendingDamages; // 処理待ちのダメージ
    float executeDelayTimer;           // 各ダメージ処理間の待機タイマー
    bool damageAppliedInAnimation;     // アニメーション中にダメージが適用されたか（ピーク時検出用）
    
    // 段階的な結果表示用
    int currentJudgingTurn;            // 現在表示中のターン（0から開始）
    std::vector<std::string> turnResults; // 各ターンの結果テキスト
    
    // 読み合い判定の視覚的演出用
    JudgeSubPhase judgeSubPhase;       // 現在のサブフェーズ
    float judgeDisplayTimer;           // 表示タイマー
    int currentJudgingTurnIndex;       // 現在判定中のターン（0から開始）

public:
    BattleState(std::shared_ptr<Player> player, std::unique_ptr<Enemy> enemy);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::BATTLE; }
    
private:
    void setupUI(Graphics& graphics);
    void updateStatus();
    void addBattleLog(const std::string& message);
    void showMessage(const std::string& message);
    void hideMessage();
    void loadBattleImages();
    void showSpellMenu();
    void handleSpellSelection(int spellChoice);
    void showItemMenu();
    void handleItemSelection(int itemChoice);
    void returnToPlayerTurn();
    void handlePlayerAction(int action);
    void executeEnemyTurn();
    void checkBattleEnd();
    void showResult();
    void endBattle();
    void handleOptionSelection(const InputManager& input);
    void showPlayerOptions();
    void showSpellOptions();
    void showItemOptions();
    void updateOptionDisplay();
    void executeSelectedOption();
    std::string getSpellDescription(int spellIndex);
    
    // 新しいコマンド選択システム用メソッド
    void initializeCommandSelection();
    void selectCommandForTurn(int turnIndex);
    void judgeBattle();
    void prepareJudgeResults();
    void showTurnResult(int turnIndex);
    void showFinalResult();
    void executeWinningTurns(float damageMultiplier);
    bool checkDesperateModeCondition();
    void showDesperateModePrompt();
    void prepareDamageList(float damageMultiplier = 1.0f);
    
    // 重複コードの共通化（DRY原則）
    void updateJudgePhase(float deltaTime, bool isDesperateMode);
    void updateExecutePhase(float deltaTime, bool isDesperateMode);
    void updateJudgeResultPhase(float deltaTime, bool isDesperateMode);
};