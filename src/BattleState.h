#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include "Enemy.h"
#include "Battle.h"
#include <memory>

enum class BattlePhase {
    INTRO,
    PLAYER_TURN,
    PLAYER_ATTACK_DISPLAY,  // プレイヤーの攻撃表示後、1秒待機
    SPELL_SELECTION,
    ITEM_SELECTION,
    ENEMY_TURN,
    ENEMY_TURN_DISPLAY,
    VICTORY_DISPLAY,        // 勝利メッセージ表示
    RESULT,
    LEVEL_UP_DISPLAY,
    END
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

public:
    BattleState(std::shared_ptr<Player> player, std::unique_ptr<Enemy> enemy);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::BATTLE; }
    
private:
    void setupUI();
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
}; 