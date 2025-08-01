#include "BattleState.h"
#include "FieldState.h"
#include "MainMenuState.h"
#include <sstream>
#include <random>
#include <cmath> // abs関数のために追加
#include <iostream> // デバッグ情報のために追加

BattleState::BattleState(std::shared_ptr<Player> player, std::unique_ptr<Enemy> enemy)
    : player(player), enemy(std::move(enemy)), currentPhase(BattlePhase::INTRO),
      battleLog(""), playerDefending(false), currentOptions(), selectedOption(0), isShowingOptions(false) {
    
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
    
    // 敵出現メッセージを画面中央に表示
    std::string enemyAppearMessage = enemy->getTypeName() + "が現れた！";
    addBattleLog(enemyAppearMessage);
    showMessage(enemyAppearMessage);
    
    // INTROフェーズから開始
    currentPhase = BattlePhase::INTRO;
    phaseTimer = 0;
    updateStatus();
}

void BattleState::exit() {
    ui.clear();
}

void BattleState::update(float deltaTime) {
    phaseTimer += deltaTime;
    ui.update(deltaTime);
    
    // スティックタイマーを更新
    static float stickTimer = 0.0f;
    if (stickTimer > 0.0f) {
        stickTimer -= deltaTime;
    }
    
    switch (currentPhase) {
        case BattlePhase::INTRO:
            if (phaseTimer > 2.0f) { // 1.5秒から2.0秒に変更
                currentPhase = BattlePhase::PLAYER_TURN;
                hideMessage();
                isShowingOptions = false; // 選択肢表示をリセット
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
    
    // 敵の体力表示（敵の真上）
    graphics.setDrawColor(255, 255, 255, 255);
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    graphics.drawText(enemyHpText, 350, 120, "default");
    
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
    
    // プレイヤーの体力・MP表示（プレイヤーの真上）
    graphics.setDrawColor(255, 255, 255, 255);
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    std::string playerMpText = "MP: " + std::to_string(player->getMp()) + "/" + std::to_string(player->getMaxMp());
    graphics.drawText(playerHpText, 150, 270, "default");
    graphics.drawText(playerMpText, 150, 290, "default");
    
    ui.render(graphics);
    graphics.present();
}

void BattleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージボード形式の選択処理
    if (isShowingOptions) {
        handleOptionSelection(input);
        return;
    }
    
    // プレイヤーターンで選択肢が表示されていない場合は表示
    if (currentPhase == BattlePhase::PLAYER_TURN && !isShowingOptions) {
        showPlayerOptions();
    } else if (currentPhase == BattlePhase::SPELL_SELECTION && !isShowingOptions) {
        showSpellOptions();
    } else if (currentPhase == BattlePhase::ITEM_SELECTION && !isShowingOptions) {
        showItemOptions();
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
    
    // メッセージラベル（画面下部に表示）
    auto messageLabelPtr = std::make_unique<Label>(50, 450, "", "default");
    messageLabelPtr->setColor({255, 255, 255, 255}); // 白色
    messageLabel = messageLabelPtr.get();
    ui.addElement(std::move(messageLabelPtr));
    
    // 操作説明
    // auto controlsLabel = std::make_unique<Label>(10, 550, "上下キー/スティック: 選択, Aボタン/スペース: 決定, Bボタン/ESC: キャンセル", "default");
    // controlsLabel->setColor({200, 200, 200, 255});
    // ui.addElement(std::move(controlsLabel));
}

void BattleState::updateStatus() {
    // if (playerStatusLabel) {
    //     std::ostringstream playerInfo;
    //     playerInfo << "【" << player->getName() << "】\n";
    //     playerInfo << "HP: " << player->getHp() << "/" << player->getMaxHp() << "\n";
    //     playerInfo << "MP: " << player->getMp() << "/" << player->getMaxMp() << "\n";
    //     std::string playerStatus = player->getStatusEffectString();
    //     if (!playerStatus.empty()) {
    //         playerInfo << playerStatus;
    //     }
    //     playerStatusLabel->setText(playerInfo.str());
    // }
    
    // if (enemyStatusLabel) {
    //     std::ostringstream enemyInfo;
    //     enemyInfo << "【" << enemy->getTypeName() << "】\n";
    //     enemyInfo << "HP: " << enemy->getHp() << "/" << enemy->getMaxHp() << "\n";
    //     std::string enemyStatus = enemy->getStatusEffectString();
    //     if (!enemyStatus.empty()) {
    //         enemyInfo << enemyStatus;
    //     }
    //     enemyStatusLabel->setText(enemyInfo.str());
    // }
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
                {
                    std::string healMessage = player->getName() + "はホイミを唱えた！\nHP" + std::to_string(result) + "回復！";
                    addBattleLog(healMessage);
                    showMessage(healMessage);
                }
                break;
            case SpellType::FIREBALL:
                {
                    std::string damageMessage = player->getName() + "はメラを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                    showMessage(damageMessage);
                }
                break;
            case SpellType::LIGHTNING:
                {
                    std::string damageMessage = player->getName() + "はいなずまを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                    showMessage(damageMessage);
                }
                break;
            case SpellType::POISON_DART:
                {
                    std::string damageMessage = player->getName() + "は毒の針を唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージ＋毒状態！";
                    addBattleLog(damageMessage);
                    showMessage(damageMessage);
                }
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
                    std::string damageMessage = player->getName() + "の攻撃！【会心の一撃！】\n" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                    showMessage(damageMessage);
                } else {
                    std::string damageMessage = player->getName() + "の攻撃！\n" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                    showMessage(damageMessage);
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

void BattleState::handleOptionSelection(const InputManager& input) {
    // 上下キーで選択肢を変更
    if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
        selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
        updateOptionDisplay();
    } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
        selectedOption = (selectedOption + 1) % currentOptions.size();
        updateOptionDisplay();
    }
    
    // アナログスティックで選択肢を変更
    const float DEADZONE = 0.3f;
    float stickX = input.getLeftStickX();
    float stickY = input.getLeftStickY();
    
    static float lastStickX = 0.0f;
    static float lastStickY = 0.0f;
    static float stickTimer = 0.0f;
    const float STICK_DELAY = 0.15f; // スティック入力の遅延（短く設定）
    
    if (abs(stickX) > DEADZONE || abs(stickY) > DEADZONE) {
        if (stickTimer <= 0.0f) {
            // 上下移動
            if (stickY < -DEADZONE) {
                std::cout << "戦闘: スティック上入力 - 選択肢変更" << std::endl;
                selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
                updateOptionDisplay();
                stickTimer = STICK_DELAY;
            } else if (stickY > DEADZONE) {
                std::cout << "戦闘: スティック下入力 - 選択肢変更" << std::endl;
                selectedOption = (selectedOption + 1) % currentOptions.size();
                updateOptionDisplay();
                stickTimer = STICK_DELAY;
            }
            // 左右移動（選択肢をスキップ）
            else if (stickX < -DEADZONE) {
                std::cout << "戦闘: スティック左入力 - 選択肢スキップ" << std::endl;
                selectedOption = (selectedOption - 2 + currentOptions.size()) % currentOptions.size();
                updateOptionDisplay();
                stickTimer = STICK_DELAY;
            } else if (stickX > DEADZONE) {
                std::cout << "戦闘: スティック右入力 - 選択肢スキップ" << std::endl;
                selectedOption = (selectedOption + 2) % currentOptions.size();
                updateOptionDisplay();
                stickTimer = STICK_DELAY;
            }
        }
        lastStickX = stickX;
        lastStickY = stickY;
    } else {
        lastStickX = 0.0f;
        lastStickY = 0.0f;
        stickTimer = 0.0f;
    }
    
    // Aボタンまたはスペースキーで選択
    if (input.isKeyJustPressed(InputKey::GAMEPAD_A) || input.isKeyJustPressed(InputKey::SPACE)) {
        executeSelectedOption();
    }
    
    // BボタンまたはESCキーでキャンセル
    if (input.isKeyJustPressed(InputKey::GAMEPAD_B) || input.isKeyJustPressed(InputKey::ESCAPE)) {
        if (currentPhase == BattlePhase::SPELL_SELECTION || currentPhase == BattlePhase::ITEM_SELECTION) {
            returnToPlayerTurn();
        }
    }
}

void BattleState::showPlayerOptions() {
    if (!isShowingOptions) {
        currentOptions.clear();
        currentOptions.push_back("戦う");
        currentOptions.push_back("呪文");
        currentOptions.push_back("アイテム");
        currentOptions.push_back("防御");
        currentOptions.push_back("逃げる");
        selectedOption = 0; // 最初の選択肢（戦う）を選択
        isShowingOptions = true;
        updateOptionDisplay();
    }
}

void BattleState::showSpellOptions() {
    if (!isShowingOptions) {
        currentOptions.clear();
        if (player->canCastSpell(SpellType::HEAL)) {
            currentOptions.push_back("ホイミ (HP回復, 3MP)");
        }
        if (player->canCastSpell(SpellType::FIREBALL)) {
            currentOptions.push_back("メラ (攻撃魔法, 5MP)");
        }
        if (player->canCastSpell(SpellType::LIGHTNING)) {
            currentOptions.push_back("いなずま (強力攻撃, 8MP)");
        }
        if (player->canCastSpell(SpellType::POISON_DART)) {
            currentOptions.push_back("毒の針 (毒付与, 6MP)");
        }
        currentOptions.push_back("やめる");
        selectedOption = 0;
        isShowingOptions = true;
        updateOptionDisplay();
    }
}

void BattleState::showItemOptions() {
    if (!isShowingOptions) {
        currentOptions.clear();
        // アイテムリストを動的に生成
        for (int i = 0; i < player->getInventory().getMaxSlots(); ++i) {
            const InventorySlot* slot = player->getInventory().getSlot(i);
            if (slot && slot->item) {
                currentOptions.push_back(slot->item->getName() + " x" + std::to_string(slot->quantity));
            }
        }
        currentOptions.push_back("やめる");
        selectedOption = 0;
        isShowingOptions = true;
        updateOptionDisplay();
    }
}

void BattleState::updateOptionDisplay() {
    std::string displayText = "選択してください:\n";
    for (size_t i = 0; i < currentOptions.size(); ++i) {
        if (i == selectedOption) {
            displayText += "▶ " + currentOptions[i] + " ";
        } else {
            displayText += "   " + currentOptions[i] + " ";
        }
    }
    showMessage(displayText);
    
    // 選択肢の色を白色に設定
    if (messageLabel) {
        messageLabel->setColor({255, 255, 255, 255}); // 白色
    }
}

void BattleState::executeSelectedOption() {
    if (currentOptions.empty()) return;
    
    std::string selected = currentOptions[selectedOption];
    isShowingOptions = false;
    
    if (currentPhase == BattlePhase::PLAYER_TURN) {
        if (selected == "戦う") {
            handlePlayerAction(1);
        } else if (selected == "呪文") {
            currentPhase = BattlePhase::SPELL_SELECTION;
        } else if (selected == "アイテム") {
            currentPhase = BattlePhase::ITEM_SELECTION;
        } else if (selected == "防御") {
            handlePlayerAction(4);
        } else if (selected == "逃げる") {
            handlePlayerAction(5);
        }
    } else if (currentPhase == BattlePhase::SPELL_SELECTION) {
        if (selected == "やめる") {
            returnToPlayerTurn();
        } else {
            // 呪文の選択を処理
            int spellIndex = 0;
            if (player->canCastSpell(SpellType::HEAL)) {
                if (selected.find("ホイミ") != std::string::npos) {
                    handleSpellSelection(1);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::FIREBALL)) {
                if (selected.find("メラ") != std::string::npos) {
                    handleSpellSelection(2);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::LIGHTNING)) {
                if (selected.find("いなずま") != std::string::npos) {
                    handleSpellSelection(3);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::POISON_DART)) {
                if (selected.find("毒の針") != std::string::npos) {
                    handleSpellSelection(4);
                    return;
                }
            }
        }
    } else if (currentPhase == BattlePhase::ITEM_SELECTION) {
        if (selected == "やめる") {
            returnToPlayerTurn();
        } else {
            // アイテムの選択を処理
            int itemIndex = 0;
            for (int i = 0; i < player->getInventory().getMaxSlots(); ++i) {
                const InventorySlot* slot = player->getInventory().getSlot(i);
                if (slot && slot->item) {
                    if (itemIndex == selectedOption) {
                        handleItemSelection(i + 1);
                        return;
                    }
                    itemIndex++;
                }
            }
        }
    }
} 