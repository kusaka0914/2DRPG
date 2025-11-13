#pragma once
#include "../entities/Player.h"
#include "../entities/Enemy.h"

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

public:
    Battle(Player* player, Enemy* enemy);
    
    BattleResult startBattle();
    int getValidInput(int min, int max) const;
    PlayerAction getPlayerChoice() const;
    void executePlayerAction(PlayerAction action);
    void executeEnemyAction();
    bool isBattleOver() const;
    SpellType chooseMagic() const;
    void showItemMenu() const;
    int chooseItem() const;
    void handleBattleEnd(BattleResult result);
}; 