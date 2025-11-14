#include "BattleState.h"
#include "NightState.h"
#include "MainMenuState.h"
#include "TownState.h"
#include "FieldState.h"
#include "GameOverState.h"
#include "EndingState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <sstream>
#include <random>
#include <cmath> // abs関数のために追加
#include <iostream> // デバッグ情報のために追加

BattleState::BattleState(std::shared_ptr<Player> player, std::unique_ptr<Enemy> enemy)
    : player(player), enemy(std::move(enemy)), currentPhase(BattlePhase::INTRO),
      selectedOption(0), messageLabel(nullptr), isShowingMessage(false),
      phaseTimer(0), oldLevel(0), oldMaxHp(0), oldMaxMp(0), oldAttack(0), oldDefense(0),
      nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer) {
    
    battle = std::make_unique<Battle>(player.get(), this->enemy.get());
    
    // レベルアップチェック用に現在のステータスを保存
    oldLevel = player->getLevel();
    oldMaxHp = player->getMaxHp();
    oldMaxMp = player->getMaxMp();
    oldAttack = player->getAttack();
    oldDefense = player->getDefense();
}

void BattleState::enter() {
    // setupUI()はrender()でGraphicsが利用可能になってから呼ばれる
    loadBattleImages();
    
    // 敵出現メッセージをバトルログに保存（render()でUI初期化後に表示される）
    std::string enemyAppearMessage = enemy->getTypeName() + "が現れた！";
    battleLog = enemyAppearMessage; // battleLogLabelがまだ初期化されていないので、直接battleLogに保存
    
    // INTROフェーズから開始
    currentPhase = BattlePhase::INTRO;
    phaseTimer = 0;
    updateStatus();
}

void BattleState::exit() {
    ui.clear();
}

void BattleState::update(float deltaTime) {
    ui.update(deltaTime);
    
    // 夜のタイマーを更新
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        // 静的変数に状態を保存
        TownState::s_nightTimerActive = true;
        TownState::s_nightTimer = nightTimer;
        
        // 目標レベルチェック
        if (player->getLevel() >= TownState::s_targetLevel && !TownState::s_levelGoalAchieved) {
            TownState::s_levelGoalAchieved = true;
            // メッセージ表示はTownStateで行うため、ここではコンソール出力のみ
        }
        
        if (nightTimer <= 0.0f) {
            // 5分経過したら夜の街に移動
            nightTimerActive = false;
            TownState::s_nightTimerActive = false;
            TownState::s_nightTimer = 0.0f;
            
            // 目標達成していない場合はゲームオーバー
            if (!TownState::s_levelGoalAchieved) {
                // ゲームオーバー処理（メイン画面に戻る）
                if (stateManager) {
                    auto newPlayer = std::make_shared<Player>("勇者");
                    stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
                }
            } else {
                if (stateManager) {
                    stateManager->changeState(std::make_unique<NightState>(player));
                    player->setCurrentNight(player->getCurrentNight() + 1);
                }
            }
        }
    } else {
        // タイマーが非アクティブな場合、静的変数もクリア
        TownState::s_nightTimerActive = false;
        TownState::s_nightTimer = 0.0f;
    }
    
    // 戦闘の更新処理
    phaseTimer += deltaTime;
    
    // スティックタイマーを更新
    static float stickTimer = 0.0f;
    if (stickTimer > 0.0f) {
        stickTimer -= deltaTime;
    }
    
    switch (currentPhase) {
        case BattlePhase::INTRO:
            if (phaseTimer > 1.0f) { // 1秒後に選択肢を表示
                currentPhase = BattlePhase::PLAYER_TURN;
                isShowingOptions = false; // 選択肢表示をリセット
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::PLAYER_TURN:
            // プレイヤーのターンで選択肢が表示されていない場合は表示
            if (!isShowingOptions) {
                showPlayerOptions();
            }
            break;
            
        case BattlePhase::PLAYER_ATTACK_DISPLAY:
            if (phaseTimer > 1.0f) { // 1秒待機後、敵のターンへ
                if (enemy->getIsAlive()) {
                    currentPhase = BattlePhase::ENEMY_TURN;
                    executeEnemyTurn();
                    currentPhase = BattlePhase::ENEMY_TURN_DISPLAY;
                    phaseTimer = 0;
                } else {
                    checkBattleEnd();
                }
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
            
        case BattlePhase::VICTORY_DISPLAY:
            if (phaseTimer > 2.0f) {
                // 勝利メッセージ表示後、レベルアップチェック
                if (player->getLevel() > oldLevel) {
                    hasLeveledUp = true;
                    currentPhase = BattlePhase::LEVEL_UP_DISPLAY;
                    
                    int hpGain = player->getMaxHp() - oldMaxHp;
                    int mpGain = player->getMaxMp() - oldMaxMp;
                    int attackGain = player->getAttack() - oldAttack;
                    int defenseGain = player->getDefense() - oldDefense;
                    
                    std::string levelUpMessage = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(player->getLevel()) + "になった！\n";
                    levelUpMessage += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain) + "\n";
                    
                    // 複数レベルアップした場合の表示
                    int levelGained = player->getLevel() - oldLevel;
                    if (levelGained > 1) {
                        levelUpMessage = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(oldLevel) + "からレベル" + std::to_string(player->getLevel()) + "になった！\n";
                        levelUpMessage += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain) + "\n";
                    }
                    
                    // 覚えた呪文を表示
                    std::vector<SpellType> learnedSpells;
                    for (int level = oldLevel + 1; level <= player->getLevel(); level++) {
                        auto spellsAtLevel = Player::getSpellsLearnedAtLevel(level);
                        learnedSpells.insert(learnedSpells.end(), spellsAtLevel.begin(), spellsAtLevel.end());
                    }
                    
                    if (!learnedSpells.empty()) {
                        for (const auto& spell : learnedSpells) {
                            levelUpMessage += Player::getSpellName(spell) + " ";
                        }
                        levelUpMessage += "を覚えた！";
                    }
                    
                    addBattleLog(levelUpMessage);
                } else {
                    currentPhase = BattlePhase::RESULT;
                }
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::LEVEL_UP_DISPLAY:
            if (phaseTimer > 3.0f) {
                currentPhase = BattlePhase::RESULT;
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
    // UIが未初期化の場合は初期化
    bool uiJustInitialized = false;
    if (!battleLogLabel) {
        setupUI(graphics);
        uiJustInitialized = true;
    }
    
    // UI初期化後、battleLogの内容をbattleLogLabelに反映
    if (uiJustInitialized && !battleLog.empty() && battleLogLabel) {
        battleLogLabel->setText(battleLog);
    }
    
    // 戦闘背景（黒）
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.clear();
    
    // 敵キャラクター描画（JSONから座標を取得）
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    int enemyX, enemyY;
    config.calculatePosition(enemyX, enemyY, battleConfig.enemyPosition, graphics.getScreenWidth(), graphics.getScreenHeight());
    
    SDL_Texture* enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTexture) {
        // 画像が存在する場合は画像を描画
        graphics.drawTexture(enemyTexture, enemyX, enemyY, battleConfig.enemyWidth, battleConfig.enemyHeight);
    } else {
        // 画像が存在しない場合は四角形で描画（フォールバック）
        graphics.setDrawColor(255, 0, 0, 255);
        graphics.drawRect(enemyX - 150, enemyY, battleConfig.enemyWidth, battleConfig.enemyHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(enemyX - 150, enemyY, battleConfig.enemyWidth, battleConfig.enemyHeight, false);
    }
    
    // 敵の体力表示（JSONから座標を取得）
    int enemyHpX, enemyHpY;
    config.calculatePosition(enemyHpX, enemyHpY, battleConfig.enemyHp.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    graphics.drawText(enemyHpText, enemyHpX, enemyHpY, "default", battleConfig.enemyHp.color);
    
    // プレイヤーキャラクター描画（JSONから座標を取得）
    int playerX, playerY;
    config.calculatePosition(playerX, playerY, battleConfig.playerPosition, graphics.getScreenWidth(), graphics.getScreenHeight());
    
    SDL_Texture* playerTexture = graphics.getTexture("player");
    if (playerTexture) {
        // 画像が存在する場合は画像を描画
        graphics.drawTexture(playerTexture, playerX, playerY, battleConfig.playerWidth, battleConfig.playerHeight);
    } else {
        // 画像が存在しない場合は四角形で描画（フォールバック）
        graphics.setDrawColor(0, 0, 255, 255);
        graphics.drawRect(playerX - 350, playerY, battleConfig.playerWidth, battleConfig.playerHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(playerX - 350, playerY, battleConfig.playerWidth, battleConfig.playerHeight, false);
    }
    
    // プレイヤーの体力・MP表示（JSONから座標を取得）
    int playerHpX, playerHpY, playerMpX, playerMpY;
    config.calculatePosition(playerHpX, playerHpY, battleConfig.playerHp.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    config.calculatePosition(playerMpX, playerMpY, battleConfig.playerMp.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    std::string playerMpText = "MP: " + std::to_string(player->getMp()) + "/" + std::to_string(player->getMaxMp());
    graphics.drawText(playerHpText, playerHpX, playerHpY, "default", battleConfig.playerHp.color);
    graphics.drawText(playerMpText, playerMpX, playerMpY, "default", battleConfig.playerMp.color);
    
    // UI描画
    ui.render(graphics);
    
    // 夜のタイマーを表示
    if (nightTimerActive) {
        CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
        CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    }
    
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

void BattleState::setupUI(Graphics& graphics) {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    
    // 戦闘ログ（JSONから座標を取得）
    int battleLogX, battleLogY;
    config.calculatePosition(battleLogX, battleLogY, battleConfig.battleLog.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto battleLogLabelPtr = std::make_unique<Label>(battleLogX, battleLogY, "", "default");
    battleLogLabelPtr->setColor(battleConfig.battleLog.color);
    battleLogLabel = battleLogLabelPtr.get();
    ui.addElement(std::move(battleLogLabelPtr));
    
    // プレイヤーステータス（JSONから座標を取得）
    int playerStatusX, playerStatusY;
    config.calculatePosition(playerStatusX, playerStatusY, battleConfig.playerStatus.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto playerStatusLabelPtr = std::make_unique<Label>(playerStatusX, playerStatusY, "", "default");
    playerStatusLabelPtr->setColor(battleConfig.playerStatus.color);
    playerStatusLabel = playerStatusLabelPtr.get();
    ui.addElement(std::move(playerStatusLabelPtr));
    
    // 敵ステータス（JSONから座標を取得）
    int enemyStatusX, enemyStatusY;
    config.calculatePosition(enemyStatusX, enemyStatusY, battleConfig.enemyStatus.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto enemyStatusLabelPtr = std::make_unique<Label>(enemyStatusX, enemyStatusY, "", "default");
    enemyStatusLabelPtr->setColor(battleConfig.enemyStatus.color);
    enemyStatusLabel = enemyStatusLabelPtr.get();
    ui.addElement(std::move(enemyStatusLabelPtr));
    
    // メッセージラベル（JSONから座標を取得）
    int messageX, messageY;
    config.calculatePosition(messageX, messageY, battleConfig.message.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto messageLabelPtr = std::make_unique<Label>(messageX, messageY, "", "default");
    messageLabelPtr->setColor(battleConfig.message.color);
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
    // この関数は使用されていないため削除
}

void BattleState::handleSpellSelection(int spellChoice) {
    SpellType selectedSpell;
    bool validSelection = false;
    
    switch (spellChoice) {
        case 1:
            if (player->canCastSpell(SpellType::KIZUGAIAERU)) {
                selectedSpell = SpellType::KIZUGAIAERU;
                validSelection = true;
            }
            break;
        case 2:
            if (player->canCastSpell(SpellType::ATSUIATSUI)) {
                selectedSpell = SpellType::ATSUIATSUI;
                validSelection = true;
            }
            break;
        case 3:
            if (player->canCastSpell(SpellType::BIRIBIRIDOKKAN)) {
                selectedSpell = SpellType::BIRIBIRIDOKKAN;
                validSelection = true;
            }
            break;
        case 4:
            if (player->canCastSpell(SpellType::DARKNESSIMPACT)) {
                selectedSpell = SpellType::DARKNESSIMPACT;
                validSelection = true;
            }
            break;
        case 5:
            if (player->canCastSpell(SpellType::ICHIKABACHIKA)) {
                selectedSpell = SpellType::ICHIKABACHIKA;
                validSelection = true;
            }
            break;
        case 6:
            if (player->canCastSpell(SpellType::TSUGICHOTTOTSUYOI)) {
                selectedSpell = SpellType::TSUGICHOTTOTSUYOI;
                validSelection = true;
            }
            break;
        case 7:
            if (player->canCastSpell(SpellType::TSUGIMECHATSUYOI)) {
                selectedSpell = SpellType::TSUGIMECHATSUYOI;
                validSelection = true;
            }
            break;
        case 8:
            if (player->canCastSpell(SpellType::WANCHANTAOSERU)) {
                selectedSpell = SpellType::WANCHANTAOSERU;
                validSelection = true;
            }
            break;
    }
    
    if (validSelection) {
        int result = player->castSpell(selectedSpell, enemy.get());
        
        switch (selectedSpell) {
            case SpellType::KIZUGAIAERU:
                {
                    std::string healMessage = player->getName() + "はキズガイエールを唱えた！\nHP" + std::to_string(result) + "回復！";
                    addBattleLog(healMessage);
                }
                break;
            case SpellType::ATSUIATSUI:
                {
                    // 攻撃ボーナス効果のメッセージを表示
                    if (player->hasNextTurnBonusActive()) {
                        addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                        player->processNextTurnBonus(); // ボーナス効果を消費
                    }
                    
                    if (result > 0) {
                        std::string damageMessage = player->getName() + "はアツイアツーイを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                        addBattleLog(damageMessage);
                    }
                }
                break;
            case SpellType::BIRIBIRIDOKKAN:
                {
                    // 攻撃ボーナス効果のメッセージを表示
                    if (player->hasNextTurnBonusActive()) {
                        addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                        player->processNextTurnBonus(); // ボーナス効果を消費
                    }
                    
                    if (result > 0) {
                        std::string damageMessage = player->getName() + "はビリビリドッカーンを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                        addBattleLog(damageMessage);
                    }
                }
                break;
            case SpellType::DARKNESSIMPACT:
                {
                    // 攻撃ボーナス効果のメッセージを表示
                    if (player->hasNextTurnBonusActive()) {
                        addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                        player->processNextTurnBonus(); // ボーナス効果を消費
                    }
                    
                    if (result > 0) {
                        std::string damageMessage = player->getName() + "はダークネスインパクトを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                        addBattleLog(damageMessage);
                    }
                }
                break;
            case SpellType::ICHIKABACHIKA:
                {
                    if (result > 0) {
                        std::string successMessage = player->getName() + "はイチカバチーカを唱えた！\nカウンター効果が発動した！";
                        addBattleLog(successMessage);
                    } else {
                        std::string failMessage = player->getName() + "はイチカバチーカを唱えた！\nしかし効果が発動しなかった...";
                        addBattleLog(failMessage);
                    }
                }
                break;
            case SpellType::TSUGICHOTTOTSUYOI:
                {
                    if (result > 0) {
                        std::string successMessage = player->getName() + "はツギチョットツヨーイを唱えた！\n次のターンの攻撃が2.5倍になる！";
                        addBattleLog(successMessage);
                    } else {
                        std::string failMessage = player->getName() + "はツギチョットツヨーイを唱えた！\nしかし効果が発動しなかった...";
                        addBattleLog(failMessage);
                    }
                }
                break;
            case SpellType::TSUGIMECHATSUYOI:
                {
                    if (result > 0) {
                        std::string successMessage = player->getName() + "はツギメッチャツヨーイを唱えた！\n次のターンの攻撃が4倍になる！";
                        addBattleLog(successMessage);
                    } else {
                        std::string failMessage = player->getName() + "はツギメッチャツヨーイを唱えた！\nしかし効果が発動しなかった...";
                        addBattleLog(failMessage);
                    }
                }
                break;
            case SpellType::WANCHANTAOSERU:
                {
                    if (result >= enemy->getHp()) {
                        std::string instantKillMessage = player->getName() + "はワンチャンタオセールを唱えた！\n" + enemy->getTypeName() + "を即死させた！";
                        addBattleLog(instantKillMessage);
                    } else if (result > 0) {
                        std::string damageMessage = player->getName() + "はワンチャンタオセールを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                        addBattleLog(damageMessage);
                    } else {
                        std::string failMessage = player->getName() + "はワンチャンタオセールを唱えた！\nしかし効果が発動しなかった...";
                        addBattleLog(failMessage);
                    }
                }
                break;
        }
        
        // 呪文実行後、敵が倒れたかチェック
        updateStatus();
        if (enemy->getIsAlive()) {
            currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
            phaseTimer = 0;
        } else {
            // 敵が倒れた場合は即座に結果をチェック
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
    
    addBattleLog(itemMenu);
}

void BattleState::handleItemSelection(int itemChoice) {
    if (itemChoice == 1 && player->getInventory().getItemCount("やくそう") > 0) {
        int itemSlot = player->getInventory().findItem("やくそう");
        if (itemSlot != -1) {
            int oldHp = player->getHp();
            player->useItem(itemSlot + 1);
            int healAmount = player->getHp() - oldHp;
            addBattleLog(player->getName() + "はやくそうを使った！HP" + std::to_string(healAmount) + "回復！");
            
            // アイテム使用後、1秒待ってから敵のターンへ
            updateStatus();
            if (enemy->getIsAlive()) {
                currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
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
    
    // 選択肢表示をリセット
    isShowingOptions = false;
    hideMessage();
    
    switch (action) {
        case 1: // 攻撃
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> criticalDis(1, 100);
                
                int baseDamage = player->calculateDamageWithBonus(*enemy);
                int damage = baseDamage;
                bool isCritical = false;
                
                // 攻撃ボーナス効果のメッセージを表示（ダメージ計算は既に完了）
                if (player->hasNextTurnBonusActive()) {
                    addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                    player->processNextTurnBonus(); // ボーナス効果を消費
                }
                
                // 会心の一撃判定（8%の確率）
                if (criticalDis(gen) <= 8) {
                    damage = damage * 2; // 攻撃ボーナスが適用されたdamageを使用
                    isCritical = true;
                    std::string damageMessage = player->getName() + "の攻撃！【会心の一撃！】\n" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                } else {
                    std::string damageMessage = player->getName() + "の攻撃！\n" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                }
                
                enemy->takeDamage(damage);
                
                // プレイヤーの攻撃後、1秒待ってから敵のターンへ
                currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
                phaseTimer = 0;
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
                    
                    // アイテム使用後、1秒待ってから敵のターンへ
                    currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
                    phaseTimer = 0;
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
            
            // 防御後、1秒待ってから敵のターンへ
            currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
            phaseTimer = 0;
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
                
                // 逃げ失敗後、1秒待ってから敵のターンへ
                currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
                phaseTimer = 0;
            }
            break;
    }
    
    updateStatus();
}

void BattleState::executeEnemyTurn() {
    if (!enemy->getIsAlive()) return;
    
    int baseDamage = enemy->performAction(*player);
    int actualDamage = baseDamage;
    
    // カウンター効果が発動している場合はダメージを無効化し、敵にダメージを与える
    if (player->hasCounterEffectActive()) {
        actualDamage = 0;
        int counterDamage = player->getAttack();
        enemy->takeDamage(counterDamage);
        addBattleLog(player->getName() + "のカウンター効果が発動！\n" + enemy->getTypeName() + "の攻撃を防いで" + std::to_string(counterDamage) + "のダメージを与えた！");
        player->setCounterEffect(false); // カウンター効果を消費
    } else {
        // 防御中はダメージを半減
        if (playerDefending) {
            actualDamage = std::max(1, baseDamage / 2); // 最低1ダメージ
            std::string attackMessage = enemy->getTypeName() + "の攻撃！" + player->getName() + "は身を守った！" + std::to_string(actualDamage) + "のダメージ！";
            addBattleLog(attackMessage);
            playerDefending = false; // 防御状態をリセット
        } else {
            std::string attackMessage = enemy->getTypeName() + "の攻撃！\n" + player->getName() + "は" + std::to_string(actualDamage) + "のダメージを受けた！";
            addBattleLog(attackMessage);
        }
    }
    
    // 実際のダメージを適用
    player->takeDamage(actualDamage);
    
    updateStatus();
}

void BattleState::checkBattleEnd() {
    if (!player->getIsAlive()) {
        lastResult = BattleResult::PLAYER_DEFEAT;
        currentPhase = BattlePhase::RESULT;
        addBattleLog("戦闘に敗北しました。勇者が倒れました。");
    } else if (!enemy->getIsAlive()) {
        lastResult = BattleResult::PLAYER_VICTORY;
        
        // 魔王を倒した場合の特別な処理
        if (enemy->getType() == EnemyType::DEMON_LORD) {
            addBattleLog("魔王を倒した！");
            
            // 魔王戦勝利の特別な報酬
            player->gainExp(2000); // 通常の2倍
            player->gainGold(10000); // 通常の2倍
            
            currentPhase = BattlePhase::VICTORY_DISPLAY;
            phaseTimer = 0;
        } else {
            // 通常の敵の場合
            int expGained = enemy->getExpReward();
            int goldGained = enemy->getGoldReward();
            player->gainExp(expGained);
            player->gainGold(goldGained);
            
            // 王様からの信頼度を3上昇
            player->changeKingTrust(3);
            
            std::string victoryMessage = enemy->getTypeName() + "は倒れた...\n" + std::to_string(expGained) + "の経験値を得た！\n王様からの信頼度が3上昇した！";
            addBattleLog(victoryMessage);
            
            // 勝利メッセージを先に表示
            currentPhase = BattlePhase::VICTORY_DISPLAY;
            phaseTimer = 0;
        }
    } else if (currentPhase == BattlePhase::ENEMY_TURN_DISPLAY) {
        currentPhase = BattlePhase::PLAYER_TURN;
        isShowingOptions = false; // 選択肢表示をリセット
    }
    
    phaseTimer = 0;
}

void BattleState::showResult() {
    // 結果表示は既にaddBattleLogで行われている
}

void BattleState::endBattle() {
    if (stateManager) {
        if (lastResult == BattleResult::PLAYER_DEFEAT) {
            // 敗北時はGameOverStateを経由してリスタート（HP復元はGameOverStateで行う）
            stateManager->changeState(std::make_unique<GameOverState>(player, "戦闘に敗北しました。"));
        } else {
            // 魔王戦の場合はエンディング画面に移行
            if (enemy->getType() == EnemyType::DEMON_LORD) {
                stateManager->changeState(std::make_unique<EndingState>(player));
            } else {
                // 通常の敵の場合はフィールドに戻る
                stateManager->changeState(std::make_unique<FieldState>(player));
            }
        }
    }
} 

void BattleState::handleOptionSelection(const InputManager& input) {
    // 呪文選択の場合は横4×縦2のグリッド移動
    if (currentPhase == BattlePhase::SPELL_SELECTION) {
        if (input.isKeyJustPressed(InputKey::LEFT) || input.isKeyJustPressed(InputKey::A)) {
            // 左に移動（-1）
            if (selectedOption > 0) {
                selectedOption--;
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::RIGHT) || input.isKeyJustPressed(InputKey::D)) {
            // 右に移動（+1）
            if (selectedOption < currentOptions.size() - 1) {
                selectedOption++;
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            // 上に移動（-4）
            if (selectedOption >= 4 && selectedOption < 8) {
                selectedOption -= 4;
            } else if (selectedOption == currentOptions.size() - 1) {
                // 「やめる」ボタンから2行目の呪文に移動（位置4に移動）
                selectedOption = 4;
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            // 下に移動（+4）
            if (selectedOption < 4) {
                if (selectedOption + 4 < currentOptions.size()) {
                    selectedOption += 4;
                }
            } else if (selectedOption >= 4 && selectedOption < 8) {
                // 2行目から「やめる」ボタンに移動
                selectedOption = currentOptions.size() - 1;
            }
            updateOptionDisplay();
        }
    } else {
        // その他の場合は従来の縦並び移動
        if (input.isKeyJustPressed(InputKey::LEFT) || input.isKeyJustPressed(InputKey::A)) {
            selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::RIGHT) || input.isKeyJustPressed(InputKey::D)) {
            selectedOption = (selectedOption + 1) % currentOptions.size();
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            selectedOption = (selectedOption + 1) % currentOptions.size();
            updateOptionDisplay();
        }
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
            if (currentPhase == BattlePhase::SPELL_SELECTION) {
                // 呪文選択時の横4×縦2グリッド移動
                if (stickY < -DEADZONE) {
                    // 上に移動（-4）
                    if (selectedOption >= 4 && selectedOption < 8) {
                        selectedOption -= 4;
                    } else if (selectedOption == currentOptions.size() - 1) {
                        // 「やめる」ボタンから2行目の呪文に移動（位置4に移動）
                        selectedOption = 4;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickY > DEADZONE) {
                    // 下に移動（+4）
                    if (selectedOption < 4) {
                        if (selectedOption + 4 < currentOptions.size()) {
                            selectedOption += 4;
                        }
                    } else if (selectedOption >= 4 && selectedOption < 8) {
                        // 2行目から「やめる」ボタンに移動
                        selectedOption = currentOptions.size() - 1;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX < -DEADZONE) {
                    // 左に移動（-1）
                    if (selectedOption > 0) {
                        selectedOption--;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX > DEADZONE) {
                    // 右に移動（+1）
                    if (selectedOption < currentOptions.size() - 1) {
                        selectedOption++;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                }
            } else {
                // その他の場合は従来の移動
                if (stickY < -DEADZONE) {
                    selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickY > DEADZONE) {
                    selectedOption = (selectedOption + 1) % currentOptions.size();
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX < -DEADZONE) {
                    selectedOption = (selectedOption - 2 + currentOptions.size()) % currentOptions.size();
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX > DEADZONE) {
                    selectedOption = (selectedOption + 2) % currentOptions.size();
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                }
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
        currentOptions.push_back("攻撃");
        currentOptions.push_back("呪文");
        // currentOptions.push_back("アイテム");
        // currentOptions.push_back("防御");
        currentOptions.push_back("逃げる");
        selectedOption = 0; // 最初の選択肢（戦う）を選択
        isShowingOptions = true;
        updateOptionDisplay();
    }
}

void BattleState::showSpellOptions() {
    if (!isShowingOptions) {
        currentOptions.clear();
        
        // 横4×縦2のグリッドで呪文を配置
        // 1行目
        if (player->canCastSpell(SpellType::KIZUGAIAERU)) {
            currentOptions.push_back("キズガイエール");
        } else {
            currentOptions.push_back("レベル3で解禁");
        }
        if (player->canCastSpell(SpellType::ATSUIATSUI)) {
            currentOptions.push_back("アツイアツーイ");
        } else {
            currentOptions.push_back("レベル3で解禁");
        }
        if (player->canCastSpell(SpellType::BIRIBIRIDOKKAN)) {
            currentOptions.push_back("ビリビリドッカーン");
        } else {
            currentOptions.push_back("レベル10で解禁");
        }
        if (player->canCastSpell(SpellType::ICHIKABACHIKA)) {
            currentOptions.push_back("イチカバチーカ");
        } else {
            currentOptions.push_back("レベル10で解禁");
        }
        // 2行目
        if (player->canCastSpell(SpellType::DARKNESSIMPACT)) {
            currentOptions.push_back("ダークネスインパクト");
        } else {
            currentOptions.push_back("レベル30で解禁");
        }
        if (player->canCastSpell(SpellType::TSUGICHOTTOTSUYOI)) {
            currentOptions.push_back("ツギチョットツヨーイ");
        } else {
            currentOptions.push_back("レベル30で解禁");
        }
        if (player->canCastSpell(SpellType::TSUGIMECHATSUYOI)) {
            currentOptions.push_back("ツギメッチャツヨーイ");
        } else {
            currentOptions.push_back("レベル60で解禁");
        }
        if (player->canCastSpell(SpellType::WANCHANTAOSERU)) {
            currentOptions.push_back("ワンチャンタオセール");
        } else {
            currentOptions.push_back("レベル60で解禁");
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
    std::string displayText = "";
    std::string selectedName = "---";
    
    // 呪文選択の場合は横4×縦2のグリッド表示
    if (currentPhase == BattlePhase::SPELL_SELECTION) {
        // 1行目
        for (int col = 0; col < 4; col++) {
            int index = col;
            if (index < currentOptions.size() - 1) { // "やめる"以外
                if (index == selectedOption) {
                    displayText += "▶ " + currentOptions[index] + " ";
                    selectedName = currentOptions[index];
                } else {
                    displayText += "   " + currentOptions[index] + " ";
                }
            }
        }
        displayText += "\n";
        
        // 2行目
        for (int col = 0; col < 4; col++) {
            int index = col + 4;
            if (index < currentOptions.size() - 1) { // "やめる"以外
                if (index == selectedOption) {
                    displayText += "▶ " + currentOptions[index] + " ";
                    selectedName = currentOptions[index];
                } else {
                    displayText += "   " + currentOptions[index] + " ";
                }
            }
        }
        displayText += "\n";
        
        // "やめる"ボタンを最後に表示
        int cancelIndex = currentOptions.size() - 1;
        if (cancelIndex == selectedOption) {
            displayText += "▶ " + currentOptions[cancelIndex];
            selectedName = "やめる";
        } else {
            displayText += "   " + currentOptions[cancelIndex];
        }
        
        // 呪文の説明を表示
        displayText += "\n\n\n";
        if (selectedOption < currentOptions.size() - 1 && selectedName.find("レベル") == std::string::npos) {
            displayText += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            displayText += getSpellDescription(selectedOption);
            displayText += "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        } else {
            displayText += "";
        }
    } else {
        // その他の場合は従来の縦並び表示
        for (size_t i = 0; i < currentOptions.size(); i++) {
            if (i == selectedOption) {
                displayText += "▶ " + currentOptions[i] + "\n";
            } else {
                displayText += "   " + currentOptions[i] + "\n";
            }
        }
    }
    
    // 選択肢をバトルログに表示
    addBattleLog(displayText);
}

void BattleState::executeSelectedOption() {
    if (currentOptions.empty()) return;
    
    std::string selected = currentOptions[selectedOption];
    isShowingOptions = false;
    
    if (currentPhase == BattlePhase::PLAYER_TURN) {
        if (selected == "攻撃") {
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
            if (player->canCastSpell(SpellType::KIZUGAIAERU)) {
                if (selected.find("キズガイエール") != std::string::npos) {
                    handleSpellSelection(1);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::ATSUIATSUI)) {
                if (selected.find("アツイアツーイ") != std::string::npos) {
                    handleSpellSelection(2);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::BIRIBIRIDOKKAN)) {
                if (selected.find("ビリビリドッカーン") != std::string::npos) {
                    handleSpellSelection(3);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::DARKNESSIMPACT)) {
                if (selected.find("ダークネスインパクト") != std::string::npos) {
                    handleSpellSelection(4);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::ICHIKABACHIKA)) {
                if (selected.find("イチカバチーカ") != std::string::npos) {
                    handleSpellSelection(5);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::TSUGICHOTTOTSUYOI)) {
                if (selected.find("ツギチョットツヨーイ") != std::string::npos) {
                    handleSpellSelection(6);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::TSUGIMECHATSUYOI)) {
                if (selected.find("ツギメッチャツヨーイ") != std::string::npos) {
                    handleSpellSelection(7);
                    return;
                }
                spellIndex++;
            }
            if (player->canCastSpell(SpellType::WANCHANTAOSERU)) {
                if (selected.find("ワンチャンタオセール") != std::string::npos) {
                    handleSpellSelection(8);
                    return;
                }
                spellIndex++;
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

std::string BattleState::getSpellDescription(int spellIndex) {
    // 呪文のインデックスに基づいて説明を返す
    switch (spellIndex) {
        case 0: // キズガイエール
            return "体力を80%回復する。\nMP消費: 10";
        case 1: // アツイアツーイ
            return "攻撃力の1.25倍のダメージを与える。\nMP消費: 4";
        case 2: // ビリビリドッカーン
            return "攻撃力の1.5倍のダメージを与える。\nMP消費: 8";
        case 3: // ダークネスインパクト
            return "攻撃力の2倍のダメージを与える。\nMP消費: 12";
        case 4: // イチカバチーカ
            return "50%の確率でカウンターする。\n相手からの攻撃を防いで反撃する。\nMP消費: 4";
        case 5: // ツギチョットツヨーイ
            return "80%の確率で次のターンの攻撃が2.5倍になる。\nMP消費: 2";
        case 6: // ツギメッチャツヨーイ
            return "50%の確率で次のターンの攻撃が4倍になる。\nMP消費: 4";
        case 7: // ワンチャンタオセール
            return "10%の確率で敵を即死させる。\n失敗時は何も起こらない。\nMP消費: 8";
        default:
            return "説明なし";
    }
} 