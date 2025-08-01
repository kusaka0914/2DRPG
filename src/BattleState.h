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
    SPELL_SELECTION,
    ITEM_SELECTION,
    ENEMY_TURN,
    ENEMY_TURN_DISPLAY,
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
    
    std::string battleLog;
    BattleResult lastResult;
    
    // レベルアップ情報
    bool hasLeveledUp;
    int oldLevel;
    int oldMaxHp, oldMaxMp, oldAttack, oldDefense;
    
    // 防御状態
    bool playerDefending;

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
    void updatePlayerTurnUI();
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
}; 