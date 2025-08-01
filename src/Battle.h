#pragma once
#include "Player.h"
#include "Enemy.h"

enum class BattleResult {
    PLAYER_VICTORY,
    PLAYER_DEFEAT,
    PLAYER_ESCAPED
};

enum class PlayerAction {
    ATTACK,
    MAGIC,
    ITEM,
    DEFEND,
    ESCAPE
};

class Battle {
private:
    Player* player;
    Enemy* enemy;
    bool playerDefending;

public:
    Battle(Player* player, Enemy* enemy);
    
    BattleResult startBattle();
    void displayBattleStatus() const;
    PlayerAction getPlayerChoice() const;
    void executePlayerAction(PlayerAction action);
    void executeEnemyAction();
    bool isBattleOver() const;
    void showMagicMenu() const;
    SpellType chooseMagic() const;
    void showItemMenu() const;
    int chooseItem() const;
    void handleBattleEnd(BattleResult result);
}; 