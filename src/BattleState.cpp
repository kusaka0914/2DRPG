#include "BattleState.h"
#include "FieldState.h"
#include "MainMenuState.h"
#include <sstream>
#include <random>

BattleState::BattleState(std::shared_ptr<Player> player, std::unique_ptr<Enemy> enemy)
    : player(player), enemy(std::move(enemy)), currentPhase(BattlePhase::INTRO), phaseTimer(0),
      battleLogLabel(nullptr), playerStatusLabel(nullptr), enemyStatusLabel(nullptr), messageLabel(nullptr),
      hasLeveledUp(false), oldLevel(0), oldMaxHp(0), oldMaxMp(0), oldAttack(0), oldDefense(0), playerDefending(false) {
    
    battle = std::make_unique<Battle>(player.get(), this->enemy.get());
    
    // レベルアップチェック用に現在のステータスを保存
    oldLevel = player->getLevel();
    oldMaxHp = player->getMaxHp();
    oldMaxMp = player->getMaxMp();
    oldAttack = player->getAttack();
    oldDefense = player->getDefense();
}

void BattleState::enter() {
    setupUI();
    loadBattleImages();
    addBattleLog(enemy->getTypeName() + "が現れた！");
    currentPhase = BattlePhase::PLAYER_TURN;
    updateStatus();
}

void BattleState::exit() {
    ui.clear();
}

void BattleState::update(float deltaTime) {
    phaseTimer += deltaTime;
    ui.update(deltaTime);
    
    switch (currentPhase) {
        case BattlePhase::INTRO:
            if (phaseTimer > 1.5f) {
                currentPhase = BattlePhase::PLAYER_TURN;
                updatePlayerTurnUI();
                hideMessage();
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::ENEMY_TURN_DISPLAY:
            if (phaseTimer > 2.0f) {
                // 状態異常処理
                player->processStatusEffects();
                enemy->processStatusEffects();
                updateStatus();
                
                checkBattleEnd();
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::LEVEL_UP_DISPLAY:
            if (phaseTimer > 3.0f) {
                currentPhase = BattlePhase::RESULT;
                hideMessage();
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::RESULT:
            if (phaseTimer > 2.0f) {
                endBattle();
            }
            break;
    }
}

void BattleState::render(Graphics& graphics) {
    // 戦闘背景（黒）
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.clear();
    
    // 敵キャラクター描画
    SDL_Texture* enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTexture) {
        // 画像が存在する場合は画像を描画
        graphics.drawTexture(enemyTexture, 350, 150, 100, 100);
    } else {
        // 画像が存在しない場合は四角形で描画（フォールバック）
        graphics.setDrawColor(255, 0, 0, 255);
        graphics.drawRect(350, 150, 100, 100, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(350, 150, 100, 100, false);
    }
    
    // プレイヤーキャラクター描画
    SDL_Texture* playerTexture = graphics.getTexture("player");
    if (playerTexture) {
        // 画像が存在する場合は画像を描画
        graphics.drawTexture(playerTexture, 150, 300, 60, 60);
    } else {
        // 画像が存在しない場合は四角形で描画（フォールバック）
        graphics.setDrawColor(0, 0, 255, 255);
        graphics.drawRect(150, 300, 60, 60, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(150, 300, 60, 60, false);
    }
    
    ui.render(graphics);
    graphics.present();
}

void BattleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (currentPhase == BattlePhase::PLAYER_TURN) {
        if (input.isKeyJustPressed(InputKey::KEY_1)) {
            handlePlayerAction(1); // 攻撃
        } else if (input.isKeyJustPressed(InputKey::KEY_2)) {
            currentPhase = BattlePhase::SPELL_SELECTION;
            showSpellMenu();
        } else if (input.isKeyJustPressed(InputKey::KEY_3)) {
            currentPhase = BattlePhase::ITEM_SELECTION;
            showItemMenu();
        } else if (input.isKeyJustPressed(InputKey::KEY_4)) {
            handlePlayerAction(4); // 防御
        } else if (input.isKeyJustPressed(InputKey::KEY_5)) {
            handlePlayerAction(5); // 逃げる
        }
    } else if (currentPhase == BattlePhase::SPELL_SELECTION) {
        if (input.isKeyJustPressed(InputKey::KEY_1)) {
            handleSpellSelection(1);
        } else if (input.isKeyJustPressed(InputKey::KEY_2)) {
            handleSpellSelection(2);
        } else if (input.isKeyJustPressed(InputKey::KEY_3)) {
            handleSpellSelection(3);
        } else if (input.isKeyJustPressed(InputKey::KEY_4)) {
            handleSpellSelection(4);
        } else if (input.isKeyJustPressed(InputKey::KEY_5) || input.isKeyJustPressed(InputKey::ESCAPE)) {
            returnToPlayerTurn();
        }
    } else if (currentPhase == BattlePhase::ITEM_SELECTION) {
        if (input.isKeyJustPressed(InputKey::KEY_1)) {
            handleItemSelection(1);
        } else if (input.isKeyJustPressed(InputKey::ESCAPE)) {
            returnToPlayerTurn();
        }
    }
}

void BattleState::setupUI() {
    ui.clear();
    
    // 戦闘ログ
    auto battleLogLabelPtr = std::make_unique<Label>(50, 400, "", "default");
    battleLogLabel = battleLogLabelPtr.get();
    ui.addElement(std::move(battleLogLabelPtr));
    
    // プレイヤーステータス
    auto playerStatusLabelPtr = std::make_unique<Label>(50, 50, "", "default");
    playerStatusLabel = playerStatusLabelPtr.get();
    ui.addElement(std::move(playerStatusLabelPtr));
    
    // 敵ステータス
    auto enemyStatusLabelPtr = std::make_unique<Label>(400, 50, "", "default");
    enemyStatusLabel = enemyStatusLabelPtr.get();
    ui.addElement(std::move(enemyStatusLabelPtr));
    
    // メッセージラベル（中央に大きく表示）
    auto messageLabelPtr = std::make_unique<Label>(100, 250, "", "default");
    messageLabelPtr->setColor({255, 255, 0, 255});
    messageLabel = messageLabelPtr.get();
    ui.addElement(std::move(messageLabelPtr));
    
    updatePlayerTurnUI();
}

void BattleState::updatePlayerTurnUI() {
    // 既存のアクションラベルを削除（重複防止）
    // 注意: 簡易実装のため、現在はラベルの重複を許可
    
    // アクションボタンのラベル
    auto actionsLabel = std::make_unique<Label>(50, 500, 
        "1:攻撃 2:呪文 3:アイテム 4:防御 5:逃げる", "default");
    actionsLabel->setColor({255, 255, 0, 255});
    ui.addElement(std::move(actionsLabel));
}

void BattleState::updateStatus() {
    if (playerStatusLabel) {
        std::ostringstream playerInfo;
        playerInfo << "【" << player->getName() << "】\n";
        playerInfo << "HP: " << player->getHp() << "/" << player->getMaxHp() << "\n";
        playerInfo << "MP: " << player->getMp() << "/" << player->getMaxMp() << "\n";
        std::string playerStatus = player->getStatusEffectString();
        if (!playerStatus.empty()) {
            playerInfo << playerStatus;
        }
        playerStatusLabel->setText(playerInfo.str());
    }
    
    if (enemyStatusLabel) {
        std::ostringstream enemyInfo;
        enemyInfo << "【" << enemy->getTypeName() << "】\n";
        enemyInfo << "HP: " << enemy->getHp() << "/" << enemy->getMaxHp() << "\n";
        std::string enemyStatus = enemy->getStatusEffectString();
        if (!enemyStatus.empty()) {
            enemyInfo << enemyStatus;
        }
        enemyStatusLabel->setText(enemyInfo.str());
    }
}

void BattleState::addBattleLog(const std::string& message) {
    battleLog = message;
    if (battleLogLabel) {
        battleLogLabel->setText(battleLog);
    }
}

void BattleState::showMessage(const std::string& message) {
    if (messageLabel) {
        messageLabel->setText(message);
    }
}

void BattleState::hideMessage() {
    if (messageLabel) {
        messageLabel->setText("");
    }
}

void BattleState::loadBattleImages() {
    // 注意: 画像読み込みはSDL2Game::loadResourcesで行うため、ここでは何もしない
    // 実際の画像読み込みはゲーム初期化時に実行される
}

void BattleState::showSpellMenu() {
    hideMessage();
    std::string spellMenu = "呪文を選択:\n";
    
    if (player->canCastSpell(SpellType::HEAL)) {
        spellMenu += "1. ホイミ (HP回復, 3MP)\n";
    }
    if (player->canCastSpell(SpellType::FIREBALL)) {
        spellMenu += "2. メラ (攻撃魔法, 5MP)\n";
    }
    if (player->canCastSpell(SpellType::LIGHTNING)) {
        spellMenu += "3. いなずま (強力攻撃, 8MP)\n";
    }
    if (player->canCastSpell(SpellType::POISON_DART)) {
        spellMenu += "4. 毒の針 (毒付与, 6MP)\n";
    }
    spellMenu += "5. やめる";
    
    showMessage(spellMenu);
}

void BattleState::handleSpellSelection(int spellChoice) {
    SpellType selectedSpell;
    bool validSelection = false;
    
    switch (spellChoice) {
        case 1:
            if (player->canCastSpell(SpellType::HEAL)) {
                selectedSpell = SpellType::HEAL;
                validSelection = true;
            }
            break;
        case 2:
            if (player->canCastSpell(SpellType::FIREBALL)) {
                selectedSpell = SpellType::FIREBALL;
                validSelection = true;
            }
            break;
        case 3:
            if (player->canCastSpell(SpellType::LIGHTNING)) {
                selectedSpell = SpellType::LIGHTNING;
                validSelection = true;
            }
            break;
        case 4:
            if (player->canCastSpell(SpellType::POISON_DART)) {
                selectedSpell = SpellType::POISON_DART;
                validSelection = true;
            }
            break;
    }
    
    if (validSelection) {
        int result = player->castSpell(selectedSpell, enemy.get());
        
        switch (selectedSpell) {
            case SpellType::HEAL:
                addBattleLog(player->getName() + "はホイミを唱えた！HP" + std::to_string(result) + "回復！");
                break;
            case SpellType::FIREBALL:
                addBattleLog(player->getName() + "はメラを唱えた！" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！");
                break;
            case SpellType::LIGHTNING:
                addBattleLog(player->getName() + "はいなずまを唱えた！" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！");
                break;
            case SpellType::POISON_DART:
                addBattleLog(player->getName() + "は毒の針を唱えた！" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージ＋毒状態！");
                break;
        }
        
        // 呪文実行後は敵のターンへ
        updateStatus();
        if (enemy->getIsAlive()) {
            currentPhase = BattlePhase::ENEMY_TURN;
            executeEnemyTurn();
            currentPhase = BattlePhase::ENEMY_TURN_DISPLAY;
            phaseTimer = 0;
        } else {
            checkBattleEnd();
        }
    } else {
        addBattleLog("その呪文は使用できません！");
        returnToPlayerTurn();
    }
    
    hideMessage();
}

void BattleState::showItemMenu() {
    hideMessage();
    std::string itemMenu = "アイテムを選択:\n";
    
    if (player->getInventory().getItemCount("やくそう") > 0) {
        itemMenu += "1. やくそう (HP回復)\n";
    }
    itemMenu += "ESC. やめる";
    
    showMessage(itemMenu);
}

void BattleState::handleItemSelection(int itemChoice) {
    if (itemChoice == 1 && player->getInventory().getItemCount("やくそう") > 0) {
        int itemSlot = player->getInventory().findItem("やくそう");
        if (itemSlot != -1) {
            int oldHp = player->getHp();
            player->useItem(itemSlot + 1);
            int healAmount = player->getHp() - oldHp;
            addBattleLog(player->getName() + "はやくそうを使った！HP" + std::to_string(healAmount) + "回復！");
            
            // アイテム使用後は敵のターンへ
            updateStatus();
            if (enemy->getIsAlive()) {
                currentPhase = BattlePhase::ENEMY_TURN;
                executeEnemyTurn();
                currentPhase = BattlePhase::ENEMY_TURN_DISPLAY;
                phaseTimer = 0;
            } else {
                checkBattleEnd();
            }
        }
    } else {
        addBattleLog("そのアイテムは使用できません！");
        returnToPlayerTurn();
    }
    
    hideMessage();
}

void BattleState::returnToPlayerTurn() {
    currentPhase = BattlePhase::PLAYER_TURN;
    hideMessage();
    updatePlayerTurnUI();
}

void BattleState::handlePlayerAction(int action) {
    if (currentPhase != BattlePhase::PLAYER_TURN) return;
    
    switch (action) {
        case 1: // 攻撃
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> criticalDis(1, 100);
                
                int baseDamage = player->calculateDamage(*enemy);
                int damage = baseDamage;
                bool isCritical = false;
                
                // 会心の一撃判定（8%の確率）
                if (criticalDis(gen) <= 8) {
                    damage = baseDamage * 2;
                    isCritical = true;
                    addBattleLog(player->getName() + "の攻撃！【会心の一撃！】" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！");
                } else {
                    addBattleLog(player->getName() + "の攻撃！" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！");
                }
                
                enemy->takeDamage(damage);
            }
            break;
            
        case 2: // 呪文（このケースは使われなくなる）
            // 呪文選択は別フェーズで処理
            break;
            
        case 3: // アイテム
            // 簡易的なアイテム使用（やくそうを使用）
            if (player->getInventory().getItemCount("やくそう") > 0) {
                int itemSlot = player->getInventory().findItem("やくそう");
                if (itemSlot != -1) {
                    int oldHp = player->getHp();
                    player->useItem(itemSlot + 1);
                    int healAmount = player->getHp() - oldHp;
                    addBattleLog(player->getName() + "はやくそうを使った！HP" + std::to_string(healAmount) + "回復！");
                } else {
                    addBattleLog("アイテムが見つかりません！");
                    return;
                }
            } else {
                addBattleLog("使用できるアイテムがありません！");
                return;
            }
            break;
            
        case 4: // 防御
            playerDefending = true;
            addBattleLog(player->getName() + "は身を守っている...");
            break;
            
        case 5: // 逃げる
            if (player->tryToEscape()) {
                lastResult = BattleResult::PLAYER_ESCAPED;
                currentPhase = BattlePhase::RESULT;
                addBattleLog(player->getName() + "は逃げ出した！");
                phaseTimer = 0;
                return;
            } else {
                addBattleLog(player->getName() + "は逃げ出そうとしたが、失敗した！");
            }
            break;
    }
    
    updateStatus();
    
    // 敵が生きていればすぐに敵のターンを実行
    if (enemy->getIsAlive()) {
        currentPhase = BattlePhase::ENEMY_TURN;
        executeEnemyTurn();
        currentPhase = BattlePhase::ENEMY_TURN_DISPLAY;
        phaseTimer = 0;
    } else {
        checkBattleEnd();
    }
}

void BattleState::executeEnemyTurn() {
    if (!enemy->getIsAlive()) return;
    
    int baseDamage = enemy->performAction(*player);
    int actualDamage = baseDamage;
    
    // 防御中はダメージを半減
    if (playerDefending) {
        actualDamage = std::max(1, baseDamage / 2); // 最低1ダメージ
        addBattleLog(enemy->getTypeName() + "の攻撃！" + player->getName() + "は身を守った！" + std::to_string(actualDamage) + "のダメージ！");
        playerDefending = false; // 防御状態をリセット
    } else {
        addBattleLog(enemy->getTypeName() + "の攻撃！" + player->getName() + "は" + std::to_string(actualDamage) + "のダメージを受けた！");
    }
    
    // 実際のダメージを適用
    player->takeDamage(actualDamage);
    
    updateStatus();
}

void BattleState::checkBattleEnd() {
    if (!player->getIsAlive()) {
        lastResult = BattleResult::PLAYER_DEFEAT;
        currentPhase = BattlePhase::RESULT;
        addBattleLog("敗北...");
        showMessage("DEFEAT...");
    } else if (!enemy->getIsAlive()) {
        lastResult = BattleResult::PLAYER_VICTORY;
        
        // 戦闘勝利報酬
        int expGained = enemy->getExpReward();
        int goldGained = enemy->getGoldReward();
        player->gainExp(expGained);
        player->gainGold(goldGained);
        
        addBattleLog(enemy->getTypeName() + "は倒れた...");
        showMessage("VICTORY!\n" + std::to_string(expGained) + "の経験値を得た！\n" + std::to_string(goldGained) + "ゴールドを手に入れた！");
        
        // レベルアップチェック
        if (player->getLevel() > oldLevel) {
            hasLeveledUp = true;
            currentPhase = BattlePhase::LEVEL_UP_DISPLAY;
            
            int hpGain = player->getMaxHp() - oldMaxHp;
            int mpGain = player->getMaxMp() - oldMaxMp;
            int attackGain = player->getAttack() - oldAttack;
            int defenseGain = player->getDefense() - oldDefense;
            
            std::string levelUpMessage = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(player->getLevel()) + "になった！\n";
            levelUpMessage += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + "\n";
            levelUpMessage += "攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain);
            
            showMessage(levelUpMessage);
        } else {
            currentPhase = BattlePhase::RESULT;
        }
    } else if (currentPhase == BattlePhase::ENEMY_TURN_DISPLAY) {
        currentPhase = BattlePhase::PLAYER_TURN;
        updatePlayerTurnUI();
        hideMessage();
    }
    
    phaseTimer = 0;
}

void BattleState::showResult() {
    // 結果表示は既にaddBattleLogで行われている
}

void BattleState::endBattle() {
    if (stateManager) {
        if (lastResult == BattleResult::PLAYER_DEFEAT) {
            // ゲームオーバー（簡易実装でメニューに戻る）
            stateManager->changeState(std::make_unique<MainMenuState>(player));
        } else {
            // フィールドに戻る
            stateManager->changeState(std::make_unique<FieldState>(player));
        }
    }
} 