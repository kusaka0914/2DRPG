#include "Battle.h"
#include <iostream>
#include <limits>

Battle::Battle(Player* player, Enemy* enemy) 
    : player(player), enemy(enemy), playerDefending(false) {
}

BattleResult Battle::startBattle() {
    while (!isBattleOver()) {
        displayBattleStatus();
        
        // プレイヤーのターン
        PlayerAction action = getPlayerChoice();
        executePlayerAction(action);
        
        if (action == PlayerAction::ESCAPE) {
            return BattleResult::PLAYER_ESCAPED;
        }
        
        if (!enemy->getIsAlive()) {
            return BattleResult::PLAYER_VICTORY;
        }
        
        // 敵のターン
        executeEnemyAction();
        
        if (!player->getIsAlive()) {
            return BattleResult::PLAYER_DEFEAT;
        }
    }
    
    return BattleResult::PLAYER_DEFEAT;
}

void Battle::displayBattleStatus() const {
    
}

PlayerAction Battle::getPlayerChoice() const {
    int choice;
    while (!(std::cin >> choice) || choice < 1 || choice > 5) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    
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
    playerDefending = false;
    
    switch (action) {
        case PlayerAction::ATTACK:
            player->attack(*enemy);
            break;
            
        case PlayerAction::MAGIC:
            showMagicMenu();
            {
                SpellType spell = chooseMagic();
                if (spell == SpellType::KIZUGAIAERU) {
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
            playerDefending = true;
            break;
            
        case PlayerAction::ESCAPE:
            player->tryToEscape();
            break;
    }
}

void Battle::executeEnemyAction() {
    if (!enemy->getIsAlive()) return;
    
    enemy->performAction(*player);
    
    // プレイヤーが防御していた場合、ダメージを半減
    // （この実装は簡略化されており、実際のダメージ計算は各攻撃メソッド内で行われる）
}

bool Battle::isBattleOver() const {
    return !player->getIsAlive() || !enemy->getIsAlive();
}

void Battle::showMagicMenu() const {
    
}

SpellType Battle::chooseMagic() const {
    int choice;
    
    while (!(std::cin >> choice) || choice < 1 || choice > 8) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    
    switch (choice) {
        case 1: return SpellType::KIZUGAIAERU;
        case 2: return SpellType::ATSUIATSUI;
        case 3: return SpellType::BIRIBIRIDOKKAN;
        case 4: return SpellType::DARKNESSIMPACT;
        case 5: return SpellType::ICHIKABACHIKA;
        case 6: return SpellType::TSUGICHOTTOTSUYOI;
        case 7: return SpellType::TSUGIMECHATSUYOI;
        case 8: return SpellType::WANCHANTAOSERU;
        default: return SpellType::KIZUGAIAERU; // デフォルト
    }
}

void Battle::showItemMenu() const {
    player->showInventory();
}

int Battle::chooseItem() const {
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
            break;
            
        case BattleResult::PLAYER_ESCAPED:
            break;
    }
} 