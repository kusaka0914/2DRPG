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
      nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      currentSelectingTurn(0), isFirstCommandSelection(true),
      currentJudgingTurn(0), currentExecutingTurn(0), executeDelayTimer(0.0f),
      damageAppliedInAnimation(false),
      judgeSubPhase(JudgeSubPhase::SHOW_PLAYER_COMMAND), judgeDisplayTimer(0.0f), currentJudgingTurnIndex(0) {
    
    battle = std::make_unique<Battle>(player.get(), this->enemy.get());
    
    battleLogic = std::make_unique<BattleLogic>(player, this->enemy.get());
    animationController = std::make_unique<BattleAnimationController>();
    effectManager = std::make_unique<BattleEffectManager>();
    phaseManager = std::make_unique<BattlePhaseManager>(battleLogic.get(), player.get(), this->enemy.get());
    
    oldLevel = player->getLevel();
    oldMaxHp = player->getMaxHp();
    oldMaxMp = player->getMaxMp();
    oldAttack = player->getAttack();
    oldDefense = player->getDefense();
}

void BattleState::enter() {
    loadBattleImages();
    
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
    
    effectManager->updateScreenShake(deltaTime);
    effectManager->updateHitEffects(deltaTime);
    if (currentPhase == BattlePhase::COMMAND_SELECT || 
        currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) {
        animationController->updateCommandSelectAnimation(deltaTime);
    } else {
        animationController->resetCommandSelectAnimation();
    }
    
    if (nightTimerActive) {
        nightTimer -= deltaTime;
        TownState::s_nightTimerActive = true;
        TownState::s_nightTimer = nightTimer;
        
        if (player->getLevel() >= TownState::s_targetLevel && !TownState::s_levelGoalAchieved) {
            TownState::s_levelGoalAchieved = true;
        }
        
        if (nightTimer <= 0.0f) {
            nightTimerActive = false;
            TownState::s_nightTimerActive = false;
            TownState::s_nightTimer = 0.0f;
            
            if (!TownState::s_levelGoalAchieved) {
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
        TownState::s_nightTimerActive = false;
        TownState::s_nightTimer = 0.0f;
    }
    
    phaseTimer += deltaTime;
    static float stickTimer = 0.0f;
    if (stickTimer > 0.0f) {
        stickTimer -= deltaTime;
    }
    
    BattlePhaseManager::PhaseUpdateContext context;
    context.currentPhase = currentPhase;
    context.phaseTimer = phaseTimer;
    context.currentSelectingTurn = currentSelectingTurn;
    context.currentJudgingTurnIndex = currentJudgingTurnIndex;
    context.currentExecutingTurn = currentExecutingTurn;
    context.judgeSubPhase = judgeSubPhase;
    context.judgeDisplayTimer = judgeDisplayTimer;
    context.executeDelayTimer = executeDelayTimer;
    context.isFirstCommandSelection = isFirstCommandSelection;
    context.isShowingOptions = isShowingOptions;
    context.damageAppliedInAnimation = damageAppliedInAnimation;
    context.pendingDamagesSize = pendingDamages.size();
    
    BattlePhaseManager::PhaseTransitionResult transitionResult = phaseManager->updatePhase(context, deltaTime);
    
    if (transitionResult.shouldTransition) {
        currentPhase = transitionResult.nextPhase;
        if (transitionResult.resetTimer) {
            phaseTimer = 0;
        }
        
        switch (transitionResult.nextPhase) {
            case BattlePhase::COMMAND_SELECT:
                if (transitionResult.nextPhase == BattlePhase::COMMAND_SELECT) {
                    battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                    battleLogic->setDesperateMode(false);
                    isFirstCommandSelection = true;
                    initializeCommandSelection();
                }
                break;
            case BattlePhase::JUDGE:
            case BattlePhase::DESPERATE_JUDGE:
                prepareJudgeResults();
                currentJudgingTurn = 0;
                currentJudgingTurnIndex = 0;
                judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                judgeDisplayTimer = 0.0f;
                break;
            default:
                break;
        }
    }
    
    switch (currentPhase) {
        case BattlePhase::INTRO:
            break;
            
        case BattlePhase::COMMAND_SELECT:
            if (isFirstCommandSelection && checkDesperateModeCondition()) {
                isFirstCommandSelection = false;
                currentPhase = BattlePhase::DESPERATE_MODE_PROMPT;
                showDesperateModePrompt();
                break;
            }
            isFirstCommandSelection = false;
            
            if (currentSelectingTurn < battleLogic->getCommandTurnCount()) {
                if (!isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
            } else {
                battleLogic->generateEnemyCommands();
            }
            break;
            
        case BattlePhase::JUDGE:
            updateJudgePhase(deltaTime, false);
            break;
            
        case BattlePhase::JUDGE_RESULT: {
            updateJudgeResultPhase(deltaTime, false);
            break;
        }
            
        case BattlePhase::EXECUTE: {
            updateExecutePhase(deltaTime, false);
            break;
        }
            
        case BattlePhase::DESPERATE_MODE_PROMPT:
            break;
            
        case BattlePhase::DESPERATE_COMMAND_SELECT:
            if (currentSelectingTurn < battleLogic->getCommandTurnCount()) {
                if (!isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
            } else {
                battleLogic->generateEnemyCommands();
            }
            break;
            
        case BattlePhase::DESPERATE_JUDGE:
            updateJudgePhase(deltaTime, true);
            break;
            
        case BattlePhase::DESPERATE_JUDGE_RESULT: {
            updateJudgeResultPhase(deltaTime, true);
            break;
        }
            
        case BattlePhase::DESPERATE_EXECUTE: {
            updateExecutePhase(deltaTime, true);
            break;
        }
            
        case BattlePhase::PLAYER_TURN:
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
                player->processStatusEffects();
                enemy->processStatusEffects();
                updateStatus();
                
                checkBattleEnd();
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::VICTORY_DISPLAY:
            if (phaseTimer > 2.0f) {
                if (player->getLevel() > oldLevel) {
                    hasLeveledUp = true;
                    currentPhase = BattlePhase::LEVEL_UP_DISPLAY;
                    
                    int hpGain = player->getMaxHp() - oldMaxHp;
                    int mpGain = player->getMaxMp() - oldMaxMp;
                    int attackGain = player->getAttack() - oldAttack;
                    int defenseGain = player->getDefense() - oldDefense;
                    
                    std::string levelUpMessage = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(player->getLevel()) + "になった！\n";
                    levelUpMessage += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain) + "\n";
                    
                    int levelGained = player->getLevel() - oldLevel;
                    if (levelGained > 1) {
                        levelUpMessage = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(oldLevel) + "からレベル" + std::to_string(player->getLevel()) + "になった！\n";
                        levelUpMessage += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain) + "\n";
                    }
                    
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
    bool uiJustInitialized = false;
    if (!battleLogLabel) {
        setupUI(graphics);
        uiJustInitialized = true;
    }
    
    if (!battleUI) {
        battleUI = std::make_unique<BattleUI>(&graphics, player, enemy.get(), battleLogic.get(), animationController.get());
    }
    
    if (uiJustInitialized && !battleLog.empty() && battleLogLabel) {
        battleLogLabel->setText(battleLog);
    }
    
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.clear();
    
    if (currentPhase == BattlePhase::JUDGE || currentPhase == BattlePhase::DESPERATE_JUDGE) {
        BattleUI::JudgeRenderParams params;
        params.currentJudgingTurnIndex = currentJudgingTurnIndex;
        params.commandTurnCount = battleLogic->getCommandTurnCount();
        params.judgeSubPhase = judgeSubPhase;
        params.judgeDisplayTimer = judgeDisplayTimer;
        battleUI->renderJudgeAnimation(params);
        ui.render(graphics);
        
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
            CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
            CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) && 
        isShowingOptions) {
        BattleUI::CommandSelectRenderParams params;
        params.currentSelectingTurn = currentSelectingTurn;
        params.commandTurnCount = battleLogic->getCommandTurnCount();
        params.isDesperateMode = battleLogic->getIsDesperateMode();
        params.selectedOption = selectedOption;
        params.currentOptions = &currentOptions;
        battleUI->renderCommandSelectionUI(params);
        
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
            CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
            CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    if (currentPhase == BattlePhase::JUDGE_RESULT || 
        currentPhase == BattlePhase::DESPERATE_JUDGE_RESULT) {
        auto stats = battleLogic->getStats();
        BattleUI::ResultAnnouncementRenderParams params;
        params.isVictory = (stats.playerWins > stats.enemyWins);
        params.isDefeat = (stats.enemyWins > stats.playerWins);
        params.isDesperateMode = battleLogic->getIsDesperateMode();
        params.hasThreeWinStreak = stats.hasThreeWinStreak;
        params.playerWins = stats.playerWins;
        params.enemyWins = stats.enemyWins;
        battleUI->renderResultAnnouncement(params);
        ui.render(graphics);
        
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
            CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
            CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
    auto& shakeState = effectManager->getShakeState();
    
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerX = playerBaseX;
    int playerY = playerBaseY;
    
    if (shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        playerX += static_cast<int>(shakeState.shakeOffsetX);
        playerY += static_cast<int>(shakeState.shakeOffsetY);
    }
    
    int playerWidth = 300;
    int playerHeight = 300;
    
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    SDL_Color playerHpColor = {100, 255, 100, 255};
    int playerHpX = playerX - 100;
    int playerHpY = playerY - playerHeight / 2 - 40;
    if (shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        playerHpX += static_cast<int>(shakeState.shakeOffsetX);
        playerHpY += static_cast<int>(shakeState.shakeOffsetY);
    }
    graphics.drawText(playerHpText, playerHpX, playerHpY, "default", playerHpColor);
    
    SDL_Texture* playerTexture = graphics.getTexture("player");
    if (playerTexture) {
        graphics.drawTexture(playerTexture, playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, false);
    }
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyX = enemyBaseX;
    int enemyY = enemyBaseY;
    
    if (!shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        enemyX += static_cast<int>(shakeState.shakeOffsetX);
        enemyY += static_cast<int>(shakeState.shakeOffsetY);
    }
    
    int enemyWidth = 300;
    int enemyHeight = 300;
    
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    int enemyHpX = enemyX - 100;
    int enemyHpY = enemyY - enemyHeight / 2 - 40;
    if (!shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        enemyHpX += static_cast<int>(shakeState.shakeOffsetX);
        enemyHpY += static_cast<int>(shakeState.shakeOffsetY);
    }
    graphics.drawText(enemyHpText, enemyHpX, enemyHpY, "default", enemyHpColor);
    
    SDL_Texture* enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTexture) {
        graphics.drawTexture(enemyTexture, enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, false);
    }
    
    effectManager->renderHitEffects(graphics);
    
    ui.render(graphics);
    
    if (nightTimerActive) {
        CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
        CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    }
    
    graphics.present();
}

void BattleState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (isShowingOptions) {
        handleOptionSelection(input);
        return;
    }
    
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) && 
        !isShowingOptions) {
        selectCommandForTurn(currentSelectingTurn);
    }
    else if (currentPhase == BattlePhase::DESPERATE_MODE_PROMPT && !isShowingOptions) {
        showDesperateModePrompt();
    }
    else if (currentPhase == BattlePhase::PLAYER_TURN && !isShowingOptions) {
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
    
    int battleLogX, battleLogY;
    config.calculatePosition(battleLogX, battleLogY, battleConfig.battleLog.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto battleLogLabelPtr = std::make_unique<Label>(battleLogX, battleLogY, "", "default");
    battleLogLabelPtr->setColor(battleConfig.battleLog.color);
    battleLogLabel = battleLogLabelPtr.get();
    ui.addElement(std::move(battleLogLabelPtr));
    
    int playerStatusX, playerStatusY;
    config.calculatePosition(playerStatusX, playerStatusY, battleConfig.playerStatus.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto playerStatusLabelPtr = std::make_unique<Label>(playerStatusX, playerStatusY, "", "default");
    playerStatusLabelPtr->setColor(battleConfig.playerStatus.color);
    playerStatusLabel = playerStatusLabelPtr.get();
    ui.addElement(std::move(playerStatusLabelPtr));
    
    int enemyStatusX, enemyStatusY;
    config.calculatePosition(enemyStatusX, enemyStatusY, battleConfig.enemyStatus.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto enemyStatusLabelPtr = std::make_unique<Label>(enemyStatusX, enemyStatusY, "", "default");
    enemyStatusLabelPtr->setColor(battleConfig.enemyStatus.color);
    enemyStatusLabel = enemyStatusLabelPtr.get();
    ui.addElement(std::move(enemyStatusLabelPtr));
    
    int messageX, messageY;
    config.calculatePosition(messageX, messageY, battleConfig.message.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto messageLabelPtr = std::make_unique<Label>(messageX, messageY, "", "default");
    messageLabelPtr->setColor(battleConfig.message.color);
    messageLabel = messageLabelPtr.get();
    ui.addElement(std::move(messageLabelPtr));
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
}

void BattleState::showSpellMenu() {
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
                    if (player->hasNextTurnBonusActive()) {
                        addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                        player->processNextTurnBonus();
                    }
                    
                    if (result > 0) {
                        std::string damageMessage = player->getName() + "はアツイアツーイを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                        addBattleLog(damageMessage);
                    }
                }
                break;
            case SpellType::BIRIBIRIDOKKAN:
                {
                    if (player->hasNextTurnBonusActive()) {
                        addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                        player->processNextTurnBonus();
                    }
                    
                    if (result > 0) {
                        std::string damageMessage = player->getName() + "はビリビリドッカーンを唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                        addBattleLog(damageMessage);
                    }
                }
                break;
            case SpellType::DARKNESSIMPACT:
                {
                    if (player->hasNextTurnBonusActive()) {
                        addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                        player->processNextTurnBonus();
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
        
        updateStatus();
        if (enemy->getIsAlive()) {
            currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
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
    
    isShowingOptions = false;
    hideMessage();
    
    switch (action) {
        case 1:
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> criticalDis(1, 100);
                
                int baseDamage = player->calculateDamageWithBonus(*enemy);
                int damage = baseDamage;
                bool isCritical = false;
                
                if (player->hasNextTurnBonusActive()) {
                    addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                    player->processNextTurnBonus();
                }
                
                if (criticalDis(gen) <= 8) { // 会心の一撃判定（8%の確率）
                    damage = damage * 2;
                    isCritical = true;
                    std::string damageMessage = player->getName() + "の攻撃！【会心の一撃！】\n" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                } else {
                    std::string damageMessage = player->getName() + "の攻撃！\n" + enemy->getTypeName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                    addBattleLog(damageMessage);
                }
                
                enemy->takeDamage(damage);
                
                currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
                phaseTimer = 0;
            }
            break;
            
        case 2:
            break;
            
        case 3:
            if (player->getInventory().getItemCount("やくそう") > 0) {
                int itemSlot = player->getInventory().findItem("やくそう");
                if (itemSlot != -1) {
                    int oldHp = player->getHp();
                    player->useItem(itemSlot + 1);
                    int healAmount = player->getHp() - oldHp;
                    addBattleLog(player->getName() + "はやくそうを使った！HP" + std::to_string(healAmount) + "回復！");
                    
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
            
        case 4:
            playerDefending = true;
            addBattleLog(player->getName() + "は身を守っている...");
            
            currentPhase = BattlePhase::PLAYER_ATTACK_DISPLAY;
            phaseTimer = 0;
            break;
            
        case 5:
            if (player->tryToEscape()) {
                lastResult = BattleResult::PLAYER_ESCAPED;
                currentPhase = BattlePhase::RESULT;
                addBattleLog(player->getName() + "は逃げ出した！");
                phaseTimer = 0;
                return;
            } else {
                addBattleLog(player->getName() + "は逃げ出そうとしたが、失敗した！");
                
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
    
    if (player->hasCounterEffectActive()) {
        actualDamage = 0;
        int counterDamage = player->getAttack();
        enemy->takeDamage(counterDamage);
        addBattleLog(player->getName() + "のカウンター効果が発動！\n" + enemy->getTypeName() + "の攻撃を防いで" + std::to_string(counterDamage) + "のダメージを与えた！");
        player->setCounterEffect(false);
    } else {
        if (playerDefending) {
            actualDamage = std::max(1, baseDamage / 2); // 最低1ダメージ
            std::string attackMessage = enemy->getTypeName() + "の攻撃！" + player->getName() + "は身を守った！" + std::to_string(actualDamage) + "のダメージ！";
            addBattleLog(attackMessage);
            playerDefending = false;
        } else {
            std::string attackMessage = enemy->getTypeName() + "の攻撃！\n" + player->getName() + "は" + std::to_string(actualDamage) + "のダメージを受けた！";
            addBattleLog(attackMessage);
        }
    }
    
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
        
        if (enemy->getType() == EnemyType::DEMON_LORD) {
            addBattleLog("魔王を倒した！");
            
            player->gainExp(2000); // 通常の2倍
            player->gainGold(10000); // 通常の2倍
            
            currentPhase = BattlePhase::VICTORY_DISPLAY;
            phaseTimer = 0;
        } else {
            int expGained = enemy->getExpReward();
            int goldGained = enemy->getGoldReward();
            player->gainExp(expGained);
            player->gainGold(goldGained);
            
            player->changeKingTrust(3);
            
            std::string victoryMessage = enemy->getTypeName() + "は倒れた...\n" + std::to_string(expGained) + "の経験値を得た！\n王様からの信頼度が3上昇した！";
            addBattleLog(victoryMessage);
            
            currentPhase = BattlePhase::VICTORY_DISPLAY;
            phaseTimer = 0;
        }
    } else if (currentPhase == BattlePhase::ENEMY_TURN_DISPLAY) {
        currentPhase = BattlePhase::PLAYER_TURN;
        isShowingOptions = false;
    }
    
    phaseTimer = 0;
}

void BattleState::showResult() {
}

void BattleState::endBattle() {
    if (stateManager) {
        if (lastResult == BattleResult::PLAYER_DEFEAT) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "戦闘に敗北しました。"));
        } else {
            if (enemy->getType() == EnemyType::DEMON_LORD) {
                stateManager->changeState(std::make_unique<EndingState>(player));
            } else {
                stateManager->changeState(std::make_unique<FieldState>(player));
            }
        }
    }
} 

void BattleState::handleOptionSelection(const InputManager& input) {
    if (currentPhase == BattlePhase::SPELL_SELECTION) {
        if (input.isKeyJustPressed(InputKey::LEFT) || input.isKeyJustPressed(InputKey::A)) {
            if (selectedOption > 0) {
                selectedOption--;
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::RIGHT) || input.isKeyJustPressed(InputKey::D)) {
            if (selectedOption < currentOptions.size() - 1) {
                selectedOption++;
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            if (selectedOption >= 4 && selectedOption < 8) {
                selectedOption -= 4;
            } else if (selectedOption == currentOptions.size() - 1) {
                selectedOption = 4;
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            if (selectedOption < 4) {
                if (selectedOption + 4 < currentOptions.size()) {
                    selectedOption += 4;
                }
            } else if (selectedOption >= 4 && selectedOption < 8) {
                selectedOption = currentOptions.size() - 1;
            }
            updateOptionDisplay();
        }
    } else {
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
    
    const float DEADZONE = 0.3f;
    float stickX = input.getLeftStickX();
    float stickY = input.getLeftStickY();
    
    static float lastStickX = 0.0f;
    static float lastStickY = 0.0f;
    static float stickTimer = 0.0f;
    const float STICK_DELAY = 0.15f;
    
    if (abs(stickX) > DEADZONE || abs(stickY) > DEADZONE) {
        if (stickTimer <= 0.0f) {
            if (currentPhase == BattlePhase::SPELL_SELECTION) {
                if (stickY < -DEADZONE) {
                    if (selectedOption >= 4 && selectedOption < 8) {
                        selectedOption -= 4;
                    } else if (selectedOption == currentOptions.size() - 1) {
                        selectedOption = 4;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickY > DEADZONE) {
                    if (selectedOption < 4) {
                        if (selectedOption + 4 < currentOptions.size()) {
                            selectedOption += 4;
                        }
                    } else if (selectedOption >= 4 && selectedOption < 8) {
                        selectedOption = currentOptions.size() - 1;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX < -DEADZONE) {
                    if (selectedOption > 0) {
                        selectedOption--;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX > DEADZONE) {
                    if (selectedOption < currentOptions.size() - 1) {
                        selectedOption++;
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                }
            } else {
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
    
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) &&
        isShowingOptions) {
        if (input.isKeyJustPressed(InputKey::Q)) {
            if (currentSelectingTurn > 0) {
                currentSelectingTurn--;
                selectCommandForTurn(currentSelectingTurn);
            }
            return;
        }
    }
    
    if (input.isKeyJustPressed(InputKey::GAMEPAD_A) || input.isKeyJustPressed(InputKey::ENTER)) {
        executeSelectedOption();
    }
    
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
        currentOptions.push_back("逃げる");
        selectedOption = 0;
        isShowingOptions = true;
        updateOptionDisplay();
    }
}

void BattleState::showSpellOptions() {
    if (!isShowingOptions) {
        currentOptions.clear();
        
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
    
    if (currentPhase == BattlePhase::SPELL_SELECTION) {
        for (int col = 0; col < 4; col++) {
            int index = col;
            if (index < currentOptions.size() - 1) {
                if (index == selectedOption) {
                    displayText += "▶ " + currentOptions[index] + " ";
                    selectedName = currentOptions[index];
                } else {
                    displayText += "   " + currentOptions[index] + " ";
                }
            }
        }
        displayText += "\n";
        
        for (int col = 0; col < 4; col++) {
            int index = col + 4;
            if (index < currentOptions.size() - 1) {
                if (index == selectedOption) {
                    displayText += "▶ " + currentOptions[index] + " ";
                    selectedName = currentOptions[index];
                } else {
                    displayText += "   " + currentOptions[index] + " ";
                }
            }
        }
        displayText += "\n";
        
        int cancelIndex = currentOptions.size() - 1;
        if (cancelIndex == selectedOption) {
            displayText += "▶ " + currentOptions[cancelIndex];
            selectedName = "やめる";
        } else {
            displayText += "   " + currentOptions[cancelIndex];
        }
        
        displayText += "\n\n\n";
        if (selectedOption < currentOptions.size() - 1 && selectedName.find("レベル") == std::string::npos) {
            displayText += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            displayText += getSpellDescription(selectedOption);
            displayText += "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        } else {
            displayText += "";
        }
    } else {
        for (size_t i = 0; i < currentOptions.size(); i++) {
            if (i == selectedOption) {
                displayText += "▶ " + currentOptions[i] + "\n";
            } else {
                displayText += "   " + currentOptions[i] + "\n";
            }
        }
    }
    
    addBattleLog(displayText);
}

void BattleState::executeSelectedOption() {
    if (currentOptions.empty()) return;
    
    std::string selected = currentOptions[selectedOption];
    isShowingOptions = false;
    
    if (currentPhase == BattlePhase::DESPERATE_MODE_PROMPT) {
        if (selected == "大勝負に挑む") {
            battleLogic->setCommandTurnCount(BattleConstants::DESPERATE_TURN_COUNT);
            battleLogic->setDesperateMode(true);
            currentPhase = BattlePhase::DESPERATE_COMMAND_SELECT;
            initializeCommandSelection();
        } else if (selected == "通常戦闘を続ける") {
            battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
            battleLogic->setDesperateMode(false);
            currentPhase = BattlePhase::COMMAND_SELECT;
            initializeCommandSelection();
        }
    } else if (currentPhase == BattlePhase::COMMAND_SELECT || 
               currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) {
        int cmd = -1;
        if (selected == "攻撃") {
            cmd = 0;
        } else if (selected == "防御") {
            cmd = 1;
        } else if (selected == "呪文") {
            if (player->getMp() > 0) {
                cmd = 2;
            } else {
                addBattleLog("MPが足りません！");
                return;
            }
        }
        
        if (cmd >= 0 && currentSelectingTurn < battleLogic->getCommandTurnCount()) {
            auto playerCmds = battleLogic->getPlayerCommands();
            if (currentSelectingTurn < playerCmds.size()) {
                playerCmds[currentSelectingTurn] = cmd;
                battleLogic->setPlayerCommands(playerCmds);
            }
            currentSelectingTurn++;
            
            if (currentSelectingTurn < battleLogic->getCommandTurnCount()) {
                selectCommandForTurn(currentSelectingTurn);
            } else {
                isShowingOptions = false;
            }
        }
    } else if (currentPhase == BattlePhase::PLAYER_TURN) {
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
    switch (spellIndex) {
        case 0:
            return "体力を80%回復する。\nMP消費: 10";
        case 1:
            return "攻撃力の1.25倍のダメージを与える。\nMP消費: 4";
        case 2:
            return "攻撃力の1.5倍のダメージを与える。\nMP消費: 8";
        case 3:
            return "攻撃力の2倍のダメージを与える。\nMP消費: 12";
        case 4:
            return "50%の確率でカウンターする。\n相手からの攻撃を防いで反撃する。\nMP消費: 4";
        case 5:
            return "80%の確率で次のターンの攻撃が2.5倍になる。\nMP消費: 2";
        case 6:
            return "50%の確率で次のターンの攻撃が4倍になる。\nMP消費: 4";
        case 7:
            return "10%の確率で敵を即死させる。\n失敗時は何も起こらない。\nMP消費: 8";
        default:
            return "説明なし";
    }
} 

void BattleState::initializeCommandSelection() {
    int turnCount = battleLogic->getCommandTurnCount();
    std::vector<int> playerCmds(turnCount, -1);
    std::vector<int> enemyCmds(turnCount, -1);
    battleLogic->setPlayerCommands(playerCmds);
    battleLogic->setEnemyCommands(enemyCmds);
    currentSelectingTurn = 0;
    isShowingOptions = false;
}

void BattleState::selectCommandForTurn(int turnIndex) {
    currentOptions.clear();
    currentOptions.push_back("攻撃");
    currentOptions.push_back("防御");
    
    if (player->getMp() > 0) {
        currentOptions.push_back("呪文");
    }
    
    selectedOption = 0;
    isShowingOptions = true;
    
    animationController->resetCommandSelectAnimation();
}

void BattleState::prepareJudgeResults() {
    turnResults.clear();
    
    auto judgeResults = battleLogic->judgeAllRounds();
    auto stats = battleLogic->calculateBattleStats();
    
    for (size_t i = 0; i < judgeResults.size(); i++) {
        const auto& result = judgeResults[i];
        std::string playerCmd = BattleLogic::getCommandName(result.playerCommand);
        std::string enemyCmd = BattleLogic::getCommandName(result.enemyCommand);
        
        std::string turnText = "ターン" + std::to_string(i + 1) + ": ";
        turnText += "自分[" + playerCmd + "] vs 敵[" + enemyCmd + "] → ";
        
        if (result.result == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
            turnText += "自分の勝ち ✅";
        } else if (result.result == BattleConstants::JUDGE_RESULT_ENEMY_WIN) {
            turnText += "敵の勝ち ❌";
        } else {
            turnText += "引き分け";
        }
        
        turnResults.push_back(turnText);
    }
    
    if (!turnResults.empty()) {
        showTurnResult(0);
    }
}

void BattleState::showTurnResult(int turnIndex) {
    if (turnIndex < 0 || turnIndex >= turnResults.size()) return;
}

void BattleState::showFinalResult() {
    animationController->resetResultAnimation();
    animationController->resetAll();
    damageAppliedInAnimation = false;
    
    auto stats = battleLogic->getStats();
    std::string resultText = "\n【結果発表】\n";
    resultText += "自分 " + std::to_string(stats.playerWins) + "勝";
    resultText += " " + std::to_string(stats.enemyWins) + "敗\n\n";
    
    if (stats.playerWins > stats.enemyWins) {
        if (stats.hasThreeWinStreak) {
            resultText += "🔥 3連勝！ダメージ1.5倍！ 🔥\n";
        } else if (battleLogic->getIsDesperateMode()) {
            resultText += "🎉 一発逆転成功！ 🎉\n";
        } else {
            resultText += "🎯 勝利！\n";
        }
        resultText += "自分が" + std::to_string(stats.playerWins) + "ターン分の攻撃を実行！";
    } else if (stats.enemyWins > stats.playerWins) {
        if (battleLogic->getIsDesperateMode()) {
            resultText += "💀 大敗北... 💀\n";
        } else {
            resultText += "❌ 敗北...\n";
        }
        resultText += "敵が" + std::to_string(stats.enemyWins) + "ターン分の攻撃を実行！";
    } else {
        resultText += "⚖️ 引き分け\n";
        resultText += "両方がダメージを受ける";
    }
    
    addBattleLog(resultText);
}

void BattleState::judgeBattle() {
    prepareJudgeResults();
}

void BattleState::prepareDamageList(float damageMultiplier) {
    pendingDamages = battleLogic->prepareDamageList(damageMultiplier);
}

void BattleState::executeWinningTurns(float damageMultiplier) {
    auto stats = battleLogic->getStats();
    auto playerCmds = battleLogic->getPlayerCommands();
    auto enemyCmds = battleLogic->getEnemyCommands();
    
    if (stats.playerWins > stats.enemyWins) {
        for (int i = 0; i < battleLogic->getCommandTurnCount(); i++) {
            if (battleLogic->judgeRound(playerCmds[i], enemyCmds[i]) == 1) {
                int cmd = playerCmds[i];
                if (cmd == 0) {
                    int baseDamage = player->calculateDamageWithBonus(*enemy);
                    int damage = static_cast<int>(baseDamage * damageMultiplier);
                    enemy->takeDamage(damage);
                    int enemyX = BattleConstants::ENEMY_POSITION_X;
                    int enemyY = BattleConstants::ENEMY_POSITION_Y;
                    effectManager->triggerHitEffect(damage, enemyX, enemyY, false);
                    float shakeIntensity = stats.hasThreeWinStreak ? 30.0f : 20.0f;
                    effectManager->triggerScreenShake(shakeIntensity, 0.6f, true, false);
                    std::string msg = player->getName() + "の攻撃！";
                    if (stats.hasThreeWinStreak) {
                        msg += "【3連勝ボーナス！】";
                    }
                    msg += "\n" + enemy->getTypeName() + "は" + 
                           std::to_string(damage) + "のダメージを受けた！";
                    if (damageMultiplier > 1.0f) {
                        msg += "（" + std::to_string(static_cast<int>(damageMultiplier * 100)) + "%）";
                    }
                    addBattleLog(msg);
                } else if (cmd == 1) {
                    addBattleLog(player->getName() + "は防御に成功した！");
                } else if (cmd == 2) {
                    if (player->getMp() > 0) {
                        player->useMp(1);
                        int baseDamage = player->calculateDamageWithBonus(*enemy);
                        int damage = static_cast<int>(baseDamage * 1.5f * damageMultiplier); // 呪文は1.5倍ダメージ
                        enemy->takeDamage(damage);
                        int enemyX = BattleConstants::ENEMY_POSITION_X;
                        int enemyY = BattleConstants::ENEMY_POSITION_Y;
                        effectManager->triggerHitEffect(damage, enemyX, enemyY, false);
                        float shakeIntensity = stats.hasThreeWinStreak ? 35.0f : 25.0f;
                        effectManager->triggerScreenShake(shakeIntensity, 0.7f, true, false);
                        std::string msg = player->getName() + "の呪文！";
                        if (stats.hasThreeWinStreak) {
                            msg += "【3連勝ボーナス！】";
                        }
                        msg += "\n" + enemy->getTypeName() + "は" + 
                               std::to_string(damage) + "のダメージを受けた！";
                        if (damageMultiplier > 1.0f) {
                            msg += "（" + std::to_string(static_cast<int>(damageMultiplier * 100)) + "%）";
                        }
                        addBattleLog(msg);
                    } else {
                        int baseDamage = player->calculateDamageWithBonus(*enemy);
                        int damage = static_cast<int>(baseDamage * damageMultiplier);
                        enemy->takeDamage(damage);
                        addBattleLog(player->getName() + "の攻撃！\n" + 
                                    enemy->getTypeName() + "は" + 
                                    std::to_string(damage) + "のダメージを受けた！");
                    }
                }
            }
        }
    } else if (stats.enemyWins > stats.playerWins) {
        int enemyAttackCount = 0;
        for (int i = 0; i < battleLogic->getCommandTurnCount(); i++) {
            if (battleLogic->judgeRound(playerCmds[i], enemyCmds[i]) == -1) {
                enemyAttackCount++;
                int damage = enemy->getAttack() - player->getDefense();
                if (damage < 0) damage = 0;
                player->takeDamage(damage);
                int playerX = BattleConstants::PLAYER_POSITION_X;
                int playerY = BattleConstants::PLAYER_POSITION_Y;
                effectManager->triggerHitEffect(damage, playerX, playerY, true);
                effectManager->triggerScreenShake(15.0f, 0.5f, false, true);
                std::string msg = enemy->getTypeName() + "の攻撃！\n" + 
                                 player->getName() + "は" + 
                                 std::to_string(damage) + "のダメージを受けた！";
                addBattleLog(msg);
            }
        }
    } else {
        bool hasDraw = false;
        for (int i = 0; i < battleLogic->getCommandTurnCount(); i++) {
            if (battleLogic->judgeRound(playerCmds[i], enemyCmds[i]) == 0) {
                if (!hasDraw) {
                    hasDraw = true;
                    addBattleLog("相打ち！両方がダメージを受けた！");
                }
                int playerDamage = enemy->getAttack() / 2;
                int enemyDamage = player->calculateDamageWithBonus(*enemy) / 2;
                if (playerDamage > 0) {
                    player->takeDamage(playerDamage);
                    int playerX = BattleConstants::PLAYER_POSITION_X;
                    int playerY = BattleConstants::PLAYER_POSITION_Y;
                    effectManager->triggerHitEffect(playerDamage, playerX, playerY, true);
                    effectManager->triggerScreenShake(10.0f, 0.3f, false, true);
                }
                if (enemyDamage > 0) {
                    enemy->takeDamage(enemyDamage);
                    int enemyX = BattleConstants::ENEMY_POSITION_X;
                    int enemyY = BattleConstants::ENEMY_POSITION_Y;
                    effectManager->triggerHitEffect(enemyDamage, enemyX, enemyY, false);
                }
            }
        }
    }
    
    updateStatus();
}

bool BattleState::checkDesperateModeCondition() {
    return battleLogic->checkDesperateModeCondition();
}

void BattleState::showDesperateModePrompt() {
    currentOptions.clear();
    currentOptions.push_back("大勝負に挑む（6ターン）");
    currentOptions.push_back("通常戦闘を続ける（3ターン）");
    selectedOption = 0;
    isShowingOptions = true;
    
    std::string message = "⚠️ 窮地モード発動！ ⚠️\n\n";
    message += "HPが30%以下になりました\n";
    message += "大勝負に挑みますか？\n";
    message += "（6ターン賭けるとダメージ1.5倍）";
    showMessage(message);
    updateOptionDisplay();
}

void BattleState::updateJudgePhase(float deltaTime, bool isDesperateMode) {
    if (currentJudgingTurnIndex < battleLogic->getCommandTurnCount()) {
        judgeDisplayTimer += deltaTime;
        
        switch (judgeSubPhase) {
            case JudgeSubPhase::SHOW_PLAYER_COMMAND:
                if (judgeDisplayTimer >= BattleConstants::JUDGE_PLAYER_COMMAND_DISPLAY_TIME) {
                    judgeSubPhase = JudgeSubPhase::SHOW_ENEMY_COMMAND;
                    judgeDisplayTimer = 0.0f;
                }
                break;
                
            case JudgeSubPhase::SHOW_ENEMY_COMMAND:
                if (judgeDisplayTimer >= BattleConstants::JUDGE_ENEMY_COMMAND_DISPLAY_TIME) {
                    judgeSubPhase = JudgeSubPhase::SHOW_RESULT;
                    judgeDisplayTimer = 0.0f;
                }
                break;
                
            case JudgeSubPhase::SHOW_RESULT:
                if (judgeDisplayTimer >= BattleConstants::JUDGE_RESULT_DISPLAY_TIME) {
                    currentJudgingTurnIndex++;
                    judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                    judgeDisplayTimer = 0.0f;
                    
                    if (currentJudgingTurnIndex >= battleLogic->getCommandTurnCount()) {
                        currentPhase = isDesperateMode ? BattlePhase::DESPERATE_JUDGE_RESULT : BattlePhase::JUDGE_RESULT;
                        showFinalResult();
                        phaseTimer = 0;
                    }
                }
                break;
        }
    }
}

void BattleState::updateJudgeResultPhase(float deltaTime, bool isDesperateMode) {
    animationController->updateResultAnimation(deltaTime, isDesperateMode);
    auto& resultState = animationController->getResultState();
    
    {
        auto stats = battleLogic->getStats();
        bool isVictory = (stats.playerWins > stats.enemyWins);
        bool isDefeat = (stats.enemyWins > stats.playerWins);
        animationController->updateResultCharacterAnimation(
            deltaTime, isVictory, isDefeat, resultState.resultAnimationTimer,
            damageAppliedInAnimation, stats.hasThreeWinStreak);
    }
    
    float shakeDuration = isDesperateMode ? 0.5f : 0.3f;
    if (resultState.resultAnimationTimer < shakeDuration && !resultState.resultShakeActive) {
        resultState.resultShakeActive = true;
        auto stats = battleLogic->getStats();
        if (stats.playerWins > stats.enemyWins) {
            float intensity = isDesperateMode ? BattleConstants::DESPERATE_VICTORY_SHAKE_INTENSITY : BattleConstants::VICTORY_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, true, false);
        } else if (stats.enemyWins > stats.playerWins) {
            float intensity = isDesperateMode ? BattleConstants::DESPERATE_DEFEAT_SHAKE_INTENSITY : BattleConstants::DEFEAT_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, false, true);
        } else {
            float intensity = isDesperateMode ? BattleConstants::DESPERATE_DRAW_SHAKE_INTENSITY : BattleConstants::DRAW_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
        }
    }
    
    if (pendingDamages.empty()) {
        auto stats = battleLogic->getStats();
        float multiplier = isDesperateMode ? 1.5f : (stats.hasThreeWinStreak ? BattleConstants::THREE_WIN_STREAK_MULTIPLIER : 1.0f);
        prepareDamageList(multiplier);
        damageAppliedInAnimation = false;
    }
    
    if (phaseTimer > BattleConstants::JUDGE_RESULT_ANNOUNCEMENT_DURATION) {
        currentPhase = isDesperateMode ? BattlePhase::DESPERATE_EXECUTE : BattlePhase::EXECUTE;
        phaseTimer = 0;
        animationController->resetResultAnimation();
        currentExecutingTurn = damageAppliedInAnimation ? 1 : 0;
        executeDelayTimer = 0.0f;
    }
}

void BattleState::updateExecutePhase(float deltaTime, bool isDesperateMode) {
    executeDelayTimer += deltaTime;
    
    if (executeDelayTimer >= BattleConstants::EXECUTE_DAMAGE_DELAY && currentExecutingTurn < pendingDamages.size()) {
        auto& damageInfo = pendingDamages[currentExecutingTurn];
        int damage = damageInfo.damage;
        bool isPlayerHit = damageInfo.isPlayerHit;
        
        if (isPlayerHit) {
            player->takeDamage(damage);
            int playerX = BattleConstants::PLAYER_POSITION_X;
            int playerY = BattleConstants::PLAYER_POSITION_Y;
            effectManager->triggerHitEffect(damage, playerX, playerY, true);
            effectManager->triggerScreenShake(BattleConstants::DAMAGE_SHAKE_INTENSITY, BattleConstants::DAMAGE_SHAKE_DURATION, false, true);
        } else {
            enemy->takeDamage(damage);
            int enemyX = BattleConstants::ENEMY_POSITION_X;
            int enemyY = BattleConstants::ENEMY_POSITION_Y;
            effectManager->triggerHitEffect(damage, enemyX, enemyY, false);
            auto stats = battleLogic->getStats();
            float shakeIntensity = isDesperateMode ? 
                (stats.hasThreeWinStreak ? BattleConstants::DESPERATE_ATTACK_SHAKE_INTENSITY_STREAK : BattleConstants::DESPERATE_ATTACK_SHAKE_INTENSITY_NORMAL) :
                (stats.hasThreeWinStreak ? BattleConstants::ATTACK_SHAKE_INTENSITY_STREAK : BattleConstants::ATTACK_SHAKE_INTENSITY_NORMAL);
            float shakeDuration = isDesperateMode ? BattleConstants::DESPERATE_ATTACK_SHAKE_DURATION : BattleConstants::ATTACK_SHAKE_DURATION;
            effectManager->triggerScreenShake(shakeIntensity, shakeDuration, true, false);
        }
        
        currentExecutingTurn++;
        executeDelayTimer = 0.0f;
    }
    
    if (currentExecutingTurn >= pendingDamages.size() && executeDelayTimer >= BattleConstants::EXECUTE_DAMAGE_DELAY) {
        updateStatus();
        
        if (enemy->getIsAlive() && player->getIsAlive()) {
            if (isDesperateMode) {
                battleLogic->setDesperateMode(false);
                battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
            }
            currentPhase = BattlePhase::COMMAND_SELECT;
            initializeCommandSelection();
        } else {
            checkBattleEnd();
        }
        phaseTimer = 0;
    }
} 