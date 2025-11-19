#include "Battle.h"
#include <iostream>
#include <limits>

Battle::Battle(Player* player, Enemy* enemy) 
    : player(player), enemy(enemy) {
}

BattleResult Battle::startBattle() {
    while (!isBattleOver()) {
        PlayerAction action = getPlayerChoice();
        executePlayerAction(action);
        
        if (action == PlayerAction::ESCAPE) {
            return BattleResult::PLAYER_ESCAPED;
        }
        
        if (!enemy->getIsAlive()) {
            return BattleResult::PLAYER_VICTORY;
        }
        
        executeEnemyAction();
        
        if (!player->getIsAlive()) {
            return BattleResult::PLAYER_DEFEAT;
        }
    }
    
    return BattleResult::PLAYER_DEFEAT;
}

int Battle::getValidInput(int min, int max) const {
    int choice;
    while (!(std::cin >> choice) || choice < min || choice > max) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "有効な値を入力してください (" << min << "-" << max << "): ";
    }
    return choice;
}

PlayerAction Battle::getPlayerChoice() const {
    std::cout << "1: 攻撃, 2: 魔法, 3: アイテム, 4: 防御, 5: 逃走\n";
    std::cout << "選択してください: ";
    
    int choice = getValidInput(1, 5);
    
    switch (choice) {
        case 1: return PlayerAction::ATTACK;
        case 2: return PlayerAction::MAGIC;
        case 3: return PlayerAction::ITEM;
        case 4: return PlayerAction::DEFEND;
        case 5: return PlayerAction::ESCAPE;
        default: return PlayerAction::ATTACK;
    }
}

void Battle::executePlayerAction(PlayerAction action) {
    switch (action) {
        case PlayerAction::ATTACK:
            player->attack(*enemy);
            break;
            
        case PlayerAction::MAGIC:
            {
                SpellType spell = chooseMagic();
                if (spell == SpellType::HEAL) {
                    player->castSpell(spell);
                } else {
                    player->castSpell(spell, enemy);
                }
            }
            break;
            
        case PlayerAction::ITEM:
            showItemMenu();
            {
                int itemChoice = chooseItem();
                if (itemChoice > 0) {
                    player->useItem(itemChoice, enemy);
                }
            }
            break;
            
        case PlayerAction::DEFEND:
            player->defend();
            break;
            
        case PlayerAction::ESCAPE:
            player->tryToEscape();
            break;
    }
}

void Battle::executeEnemyAction() {
    if (!enemy->getIsAlive()) return;
    enemy->performAction(*player);
}

bool Battle::isBattleOver() const {
    return !player->getIsAlive() || !enemy->getIsAlive();
}

SpellType Battle::chooseMagic() const {
    int choice = getValidInput(1, 8);
    
    switch (choice) {
        case 0: return SpellType::HEAL;
        case 1: return SpellType::STATUS_UP;
        case 2: return SpellType::ATTACK;
        default: return SpellType::HEAL;
    }
}

void Battle::showItemMenu() const {
    player->showInventory();
}

int Battle::chooseItem() const {
    std::cout << "アイテムを選択してください (0: キャンセル): ";
    int choice;
    std::cin >> choice;
    
    if (choice == 0) {
        return 0;
    }
    
    const Inventory& inventory = player->getInventory();
    if (choice < 1 || choice > inventory.getMaxSlots()) {
        return 0;
    }
    
    const InventorySlot* slot = inventory.getSlot(choice - 1);
    if (!slot || !slot->item) {
        return 0;
    }
    
    return choice;
}

void Battle::handleBattleEnd(BattleResult result) { 
    switch (result) {
        case BattleResult::PLAYER_VICTORY:
            player->gainExp(enemy->getExpReward());
            player->gainGold(enemy->getGoldReward());
            break;
            
        case BattleResult::PLAYER_DEFEAT:
            // ゲームオーバー処理は別途実装
            break;
            
        case BattleResult::PLAYER_ESCAPED:
            break;
    }
} 