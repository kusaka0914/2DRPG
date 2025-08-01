#include "Battle.h"
#include <iostream>
#include <limits>

Battle::Battle(Player* player, Enemy* enemy) 
    : player(player), enemy(enemy), playerDefending(false) {
}

BattleResult Battle::startBattle() {
    std::cout << "\n=== 戦闘開始！ ===" << std::endl;
    std::cout << enemy->getTypeName() << "が現れた！" << std::endl;
    
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
        
        std::cout << "\n--- 次のターン ---\n" << std::endl;
    }
    
    return BattleResult::PLAYER_DEFEAT;
}

void Battle::displayBattleStatus() const {
    std::cout << "\n=== 戦闘状況 ===" << std::endl;
    std::cout << "【" << player->getName() << "】 HP: " << player->getHp() << "/" << player->getMaxHp() 
              << " MP: " << player->getMp() << "/" << player->getMaxMp() << std::endl;
    std::cout << "【" << enemy->getName() << "】 HP: " << enemy->getHp() << "/" << enemy->getMaxHp() << std::endl;
    std::cout << std::endl;
}

PlayerAction Battle::getPlayerChoice() const {
    std::cout << "行動を選んでください：" << std::endl;
    std::cout << "1. 攻撃" << std::endl;
    std::cout << "2. 呪文" << std::endl;
    std::cout << "3. アイテム" << std::endl;
    std::cout << "4. 防御" << std::endl;
    std::cout << "5. 逃げる" << std::endl;
    std::cout << "選択 (1-5): ";
    
    int choice;
    while (!(std::cin >> choice) || choice < 1 || choice > 5) {
        std::cout << "無効な選択です。1-5で選んでください: ";
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
    std::cout << "\n使用する呪文を選んでください：" << std::endl;
    std::cout << "1. ホイミ (HP回復, 3MP)" << std::endl;
    
    if (player->canCastSpell(SpellType::FIREBALL)) {
        std::cout << "2. メラ (攻撃魔法, 5MP)" << std::endl;
    }
    
    if (player->canCastSpell(SpellType::LIGHTNING)) {
        std::cout << "3. いなずま (強力攻撃魔法, 8MP)" << std::endl;
    }
    
    std::cout << "4. やめる" << std::endl;
}

SpellType Battle::chooseMagic() const {
    int choice;
    std::cout << "選択 (1-4): ";
    
    while (!(std::cin >> choice) || choice < 1 || choice > 4) {
        std::cout << "無効な選択です。1-4で選んでください: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    
    switch (choice) {
        case 1: return SpellType::HEAL;
        case 2: return SpellType::FIREBALL;
        case 3: return SpellType::LIGHTNING;
        case 4: 
        default: return SpellType::HEAL; // デフォルト
    }
}

void Battle::showItemMenu() const {
    std::cout << "\n=== アイテムを使う ===" << std::endl;
    player->showInventory();
}

int Battle::chooseItem() const {
    std::cout << "使用するアイテムを選んでください (0で戻る): ";
    
    int choice;
    std::cin >> choice;
    
    if (choice == 0) {
        std::cout << "アイテム使用をキャンセルしました。" << std::endl;
        return 0;
    }
    
    const Inventory& inventory = player->getInventory();
    if (choice < 1 || choice > inventory.getMaxSlots()) {
        std::cout << "無効な選択です。" << std::endl;
        return 0;
    }
    
    const InventorySlot* slot = inventory.getSlot(choice - 1);
    if (!slot || !slot->item) {
        std::cout << "そのスロットにはアイテムがありません。" << std::endl;
        return 0;
    }
    
    return choice;
}

void Battle::handleBattleEnd(BattleResult result) {
    std::cout << "\n=== 戦闘終了 ===" << std::endl;
    
    switch (result) {
        case BattleResult::PLAYER_VICTORY:
            std::cout << "勝利！" << std::endl;
            player->gainExp(enemy->getExpReward());
            player->gainGold(enemy->getGoldReward());
            break;
            
        case BattleResult::PLAYER_DEFEAT:
            std::cout << "敗北..." << std::endl;
            std::cout << "ゲームオーバー" << std::endl;
            break;
            
        case BattleResult::PLAYER_ESCAPED:
            std::cout << "戦闘から逃走しました。" << std::endl;
            break;
    }
} 