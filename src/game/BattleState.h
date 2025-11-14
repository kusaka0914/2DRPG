#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../core/Battle.h"
#include <memory>

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
    DESPERATE_EXECUTE,      // 実行（ダメージ倍率あり）
    // 既存のフェーズ（後方互換性のため残す）
    PLAYER_TURN,
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

enum class TurnCommand {
    ATTACK,  // 攻撃
    DEFEND,  // 防御
    SPELL    // 呪文
};

enum class JudgeSubPhase {
    SHOW_PLAYER_COMMAND,  // プレイヤーのコマンドを左に表示
    SHOW_ENEMY_COMMAND,   // 敵のコマンドを右に表示
    SHOW_RESULT           // 勝敗を真ん中に大きく表示
};

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
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒
    
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
    
    // 新しいコマンド選択システム
    bool isDesperateMode;              // 窮地モード中か
    int commandTurnCount;              // 3（通常）or 6（窮地）
    std::vector<int> playerCommands;   // 選択したコマンド（0=攻撃, 1=防御, 2=呪文）
    std::vector<int> enemyCommands;    // 敵のコマンド
    int currentSelectingTurn;          // 現在選択中のターン（0から開始）
    int playerWins;                    // プレイヤーの勝利数
    int enemyWins;                     // 敵の勝利数
    bool hasThreeWinStreak;            // 3連勝フラグ
    bool isFirstCommandSelection;      // 最初のコマンド選択か（窮地モード判定用）
    
    // 段階的な結果表示用
    int currentJudgingTurn;            // 現在表示中のターン（0から開始）
    std::vector<std::string> turnResults; // 各ターンの結果テキスト
    
    // 読み合い判定の視覚的演出用
    JudgeSubPhase judgeSubPhase;       // 現在のサブフェーズ
    float judgeDisplayTimer;           // 表示タイマー
    int currentJudgingTurnIndex;       // 現在判定中のターン（0から開始）
    
    // コマンド選択UIのアニメーション用
    float commandSelectAnimationTimer; // コマンド選択アニメーションタイマー
    float commandSelectSlideProgress;  // スライドアニメーションの進捗
    
    // 結果発表のアニメーション用
    float resultAnimationTimer;       // 結果発表アニメーションタイマー
    float resultScale;                 // 拡大アニメーションのスケール
    float resultRotation;              // 回転アニメーション（度）
    bool resultShakeActive;            // 結果発表時の画面揺れ
    
    // キャラクターアニメーション用
    float playerAttackOffsetX;         // プレイヤーの攻撃アニメーションXオフセット
    float playerAttackOffsetY;         // プレイヤーの攻撃アニメーションYオフセット
    float enemyAttackOffsetX;          // 敵の攻撃アニメーションXオフセット
    float enemyAttackOffsetY;          // 敵の攻撃アニメーションYオフセット
    float enemyHitOffsetX;             // 敵がダメージを受けるXオフセット
    float enemyHitOffsetY;             // 敵がダメージを受けるYオフセット
    float playerHitOffsetX;            // プレイヤーがダメージを受けるXオフセット
    float playerHitOffsetY;            // プレイヤーがダメージを受けるYオフセット
    int attackAnimationFrame;          // 攻撃アニメーションのフレーム（0-2）
    
    // 画面揺れ用
    float shakeOffsetX;                // X方向の揺れオフセット
    float shakeOffsetY;                // Y方向の揺れオフセット
    float shakeTimer;                  // 揺れの残り時間
    float shakeIntensity;              // 揺れの強度
    bool isVictoryShake;                // 勝利時の爽快感のある揺れか
    bool shakeTargetPlayer;             // true=プレイヤーを揺らす, false=敵を揺らす

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
    int judgeRound(int playerCmd, int enemyCmd);
    void judgeBattle();
    void prepareJudgeResults();
    void showTurnResult(int turnIndex);
    void showFinalResult();
    void executeWinningTurns(float damageMultiplier);
    bool checkDesperateModeCondition();
    void showDesperateModePrompt();
    void generateEnemyCommands();
    std::string getCommandName(int cmd);
    
    // 画面揺れ機能
    void triggerScreenShake(float intensity, float duration, bool victoryShake = false, bool targetPlayer = false);
    void updateScreenShake(float deltaTime);
    
    // 読み合い判定の視覚的演出
    void renderJudgeAnimation(Graphics& graphics);
    
    // コマンド選択UIの視覚的演出
    void renderCommandSelectionUI(Graphics& graphics);
    
    // 結果発表の視覚的演出
    void renderResultAnnouncement(Graphics& graphics);
    void updateResultCharacterAnimation(float deltaTime);
};