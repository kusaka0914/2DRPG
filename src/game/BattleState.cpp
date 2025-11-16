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
      victoryExpGained(0), victoryGoldGained(0), victoryEnemyName(""),
      nightTimerActive(TownState::s_nightTimerActive), nightTimer(TownState::s_nightTimer),
      currentSelectingTurn(0), isFirstCommandSelection(true),
      currentJudgingTurn(0), currentExecutingTurn(0), executeDelayTimer(0.0f),
      damageAppliedInAnimation(false), waitingForSpellSelection(false),
      judgeSubPhase(JudgeSubPhase::SHOW_PLAYER_COMMAND), judgeDisplayTimer(0.0f), currentJudgingTurnIndex(0),
      introScale(0.0f), introTimer(0.0f) {
    
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
    
    currentPhase = BattlePhase::INTRO;
    phaseTimer = 0;
    introScale = 0.0f;
    introTextScale = 0.0f;
    introTimer = 0.0f;
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
    
    // プレイヤーのHPが7割を切った時に敵の型を確定
    if (!battleLogic->isBehaviorTypeDetermined()) {
        float hpRatio = static_cast<float>(player->getHp()) / static_cast<float>(player->getMaxHp());
        if (hpRatio < 0.7f) {
            battleLogic->confirmBehaviorType();
        }
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
    
    // INTROフェーズの場合はBattlePhaseManagerをスキップ（独自のタイマー管理のため）
    if (currentPhase != BattlePhase::INTRO) {
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
                    // フェーズ遷移時に必ず敵コマンドを生成する（確実に生成されるように）
                    battleLogic->generateEnemyCommands();
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
    }
    
    switch (currentPhase) {
        case BattlePhase::INTRO: {
            introTimer += deltaTime;
            
            constexpr float SCALE_ANIMATION_DURATION = 0.3f;
            constexpr float TEXT_DELAY = 0.1f; // テキストは敵より0.1秒遅れて登場
            constexpr float TEXT_SCALE_DURATION = 0.25f; // テキストのスケールアニメーション時間
            
            // 敵のスケールアニメーション
            if (introTimer < SCALE_ANIMATION_DURATION) {
                // スケールアニメーション（0.0から1.0へ、イージング付き）
                float progress = introTimer / SCALE_ANIMATION_DURATION;
                float eased = progress * progress * (3.0f - 2.0f * progress); // smoothstep
                introScale = eased;
            } else {
                // スケールアニメーション完了後、パルス効果（大きい小さいを繰り返す）
                float pulseTime = introTimer - SCALE_ANIMATION_DURATION;
                float pulse = std::sin(pulseTime * 3.14159f * 4.0f) * 0.08f; // ±8%のパルス
                introScale = 1.0f + pulse;
            }
            
            // テキストのスケールアニメーション（敵より少し遅れて登場）
            float textTimer = introTimer - TEXT_DELAY;
            if (textTimer < 0.0f) {
                introTextScale = 0.0f;
            } else if (textTimer < TEXT_SCALE_DURATION) {
                float progress = textTimer / TEXT_SCALE_DURATION;
                float eased = progress * progress * (3.0f - 2.0f * progress); // smoothstep
                introTextScale = eased;
            } else {
                introTextScale = 1.0f; // アニメーション完了後は固定
            }
            
            // INTRO_DURATION秒後にコマンド選択画面へ遷移
            if (introTimer >= BattleConstants::INTRO_DURATION) {
                currentPhase = BattlePhase::COMMAND_SELECT;
                phaseTimer = 0;
                introScale = 1.0f;
                introTextScale = 1.0f;
                battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                battleLogic->setDesperateMode(false);
                isFirstCommandSelection = true;
                initializeCommandSelection();
            }
            break;
        }
            
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
            // フェーズが既に変更されている場合は処理をスキップ
            if (currentPhase != BattlePhase::JUDGE_RESULT) {
                break;
            }
            updateJudgeResultPhase(deltaTime, false);
            // フェーズが変更された場合は処理を終了
            if (currentPhase != BattlePhase::JUDGE_RESULT) {
                break;
            }
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
            // フェーズが既に変更されている場合は処理をスキップ
            if (currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                break;
            }
            updateJudgeResultPhase(deltaTime, true);
            // フェーズが変更された場合は処理を終了
            if (currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                break;
            }
            break;
        }
            
        case BattlePhase::PLAYER_TURN:
            if (!isShowingOptions) {
                showPlayerOptions();
            }
            break;
            
        case BattlePhase::SPELL_SELECTION:
            if (!isShowingOptions) {
                showSpellOptions();
            }
            break;
            
        case BattlePhase::ITEM_SELECTION:
            if (!isShowingOptions) {
                showItemOptions();
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
                    phaseTimer = 0;
                } else {
                    endBattle();
                }
            }
            break;
            
        case BattlePhase::LEVEL_UP_DISPLAY:
            if (phaseTimer > 3.0f) {
                endBattle();
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
    
    // JUDGE_RESULTフェーズではrenderResultAnnouncement()内で背景画像を描画するため、ここでは処理しない
    if (currentPhase == BattlePhase::JUDGE_RESULT || 
        currentPhase == BattlePhase::DESPERATE_JUDGE_RESULT) {
        // renderResultAnnouncement()内で背景画像を描画するため、ここでは何もしない
    } else {
        // 画面をクリア（背景画像で覆う前に）
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.clear();
        
        // 背景画像を描画
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        SDL_Texture* bgTexture = graphics.getTexture("battle_bg");
        if (!bgTexture) {
            // 背景画像が読み込まれていない場合は読み込む
            bgTexture = graphics.loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
        
        if (bgTexture) {
            // 背景画像を画面全体に描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
    }
    
    // INTROフェーズの特別な描画
    if (currentPhase == BattlePhase::INTRO) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        
        // 中央に敵を配置
        int enemyX = screenWidth / 2;
        int enemyY = screenHeight / 2;
        constexpr int BASE_ENEMY_SIZE = 300;
        int enemyWidth = static_cast<int>(BASE_ENEMY_SIZE * introScale);
        int enemyHeight = static_cast<int>(BASE_ENEMY_SIZE * introScale);
    
    SDL_Texture* enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTexture) {
            graphics.drawTexture(enemyTexture, enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
            graphics.setDrawColor(255, 100, 100, 255);
            graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, false);
        }
        
        // 敵の下にテキストを表示（スケールアニメーション付き）
        std::string appearText = enemy->getTypeName() + "があらわれた！";
        SDL_Color textColor = {255, 255, 255, 255};
        
        // テキストをテクスチャとして取得
        SDL_Texture* textTexture = graphics.createTextTexture(appearText, "default", textColor);
        if (textTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            // スケールに応じてサイズを調整
            int scaledWidth = static_cast<int>(textWidth * introTextScale);
            int scaledHeight = static_cast<int>(textHeight * introTextScale);
            
            // テキストの中心位置を計算（位置は固定、敵のスケールに依存しない）
            int textX = enemyX - scaledWidth / 2;
            int textY = enemyY + BASE_ENEMY_SIZE / 2 + 40 - scaledHeight / 2 - 20; // 固定サイズを基準に位置を計算
            
            // 背景を描画（元のサイズに合わせて、パディング付き）
            constexpr int padding = 8;
            int bgX = enemyX - textWidth / 2 - padding;
            int bgY = enemyY + BASE_ENEMY_SIZE / 2 + 40 - textHeight / 2 - 20 - padding;
            int bgWidth = textWidth + padding * 2;
            int bgHeight = textHeight + padding * 2;
            
            graphics.setDrawColor(0, 0, 0, 200);
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, false);
            
            // スケールしたテキストを描画
            graphics.drawTexture(textTexture, textX, textY, scaledWidth, scaledHeight);
            
            SDL_DestroyTexture(textTexture);
        }
        
        // フォールバック：通常のテキスト描画（textTextureがnullの場合）
        if (!textTexture) {
            int textX = enemyX;
            int textY = enemyY + BASE_ENEMY_SIZE / 2 + 40;
            
            // 背景を描画（テキストサイズを取得してから）
            SDL_Texture* fallbackTexture = graphics.createTextTexture(appearText, "default", textColor);
            if (fallbackTexture) {
                int textWidth, textHeight;
                SDL_QueryTexture(fallbackTexture, nullptr, nullptr, &textWidth, &textHeight);
                
                constexpr int padding = 8;
                int bgX = textX - padding;
                int bgY = textY - padding;
                int bgWidth = textWidth + padding * 2;
                int bgHeight = textHeight + padding * 2;
                
                graphics.setDrawColor(0, 0, 0, 200);
                graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
                graphics.setDrawColor(255, 255, 255, 255);
                graphics.drawRect(bgX, bgY, bgWidth, bgHeight, false);
                
                SDL_DestroyTexture(fallbackTexture);
            }
            
            graphics.drawText(appearText, textX, textY, "default", textColor);
        }
        
        graphics.present();
        return;
    }
    
    if (currentPhase == BattlePhase::JUDGE || currentPhase == BattlePhase::DESPERATE_JUDGE) {
        BattleUI::JudgeRenderParams params;
        params.currentJudgingTurnIndex = currentJudgingTurnIndex;
        params.commandTurnCount = battleLogic->getCommandTurnCount();
        params.judgeSubPhase = judgeSubPhase;
        params.judgeDisplayTimer = judgeDisplayTimer;
        battleUI->renderJudgeAnimation(params);
        // ui.render(graphics); // バトルログなどの下部UIを非表示
        
        // if (nightTimerActive) {
        //     CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        //     CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
        //     CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        // }
        
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
        
        // if (nightTimerActive) {
        //     CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        //     CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
        //     CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        // }
        
        graphics.present();
        return;
    }
    
    if (currentPhase == BattlePhase::VICTORY_DISPLAY) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        int centerX = screenWidth / 2;
        int centerY = screenHeight / 2;
        
        // 画面をクリア（背景画像で覆う前に）
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.clear();
        
        // 背景画像を描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
        SDL_Texture* bgTexture = graphics.getTexture("battle_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
        
        // プレイヤーと敵のキャラクター描画（renderResultAnnouncementと同じ構成）
        auto& charState = animationController->getCharacterState();
        int playerBaseX = screenWidth / 4;
        int playerBaseY = screenHeight / 2;
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        int playerWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
        int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
        
        // HP表示（プレイヤーのみ表示）
        SDL_Color whiteColor = {255, 255, 255, 255};
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        
        // プレイヤーの名前とレベル（HPの上に表示）
        std::string playerNameText = player->getName() + " Lv." + std::to_string(player->getLevel());
        SDL_Texture* playerNameTexture = graphics.createTextTexture(playerNameText, "default", whiteColor);
        if (playerNameTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerNameTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = playerX - 100 - padding;
            int bgY = playerY - playerHeight / 2 - 80 - padding;
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(playerNameTexture);
        }
        graphics.drawText(playerNameText, playerX - 100, playerY - playerHeight / 2 - 80, "default", whiteColor);
        
        std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
        SDL_Texture* playerHpTexture = graphics.createTextTexture(playerHpText, "default", whiteColor);
        if (playerHpTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerHpTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = playerX - 100 - padding;
            int bgY = playerY - playerHeight / 2 - 40 - padding;
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(playerHpTexture);
        }
        graphics.drawText(playerHpText, playerX - 100, playerY - playerHeight / 2 - 40, "default", whiteColor);
        
        // プレイヤーのみ描画（敵は描画しない）
        SDL_Texture* playerTex = graphics.getTexture("player");
        if (playerTex) {
            graphics.drawTexture(playerTex, playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight);
        } else {
            graphics.setDrawColor(100, 200, 255, 255);
            graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, false);
        }
        
        // 勝利メッセージを中央に表示（renderResultAnnouncementの勝敗テキストの位置）
        std::string victoryText = victoryEnemyName + "を倒した。経験値が" + std::to_string(victoryExpGained);
        SDL_Color textColor = {255, 255, 255, 255};
        
        SDL_Texture* textTexture = graphics.createTextTexture(victoryText, "default", textColor);
        if (textTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            int textX = centerX - textWidth / 2;
            int textY = centerY - textHeight / 2 - 100;
            int padding = 12;
            
            // 背景を描画
            graphics.setDrawColor(0, 0, 0, 200);
            graphics.drawRect(textX - padding, textY - padding,
                             textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(textX - padding, textY - padding,
                             textWidth + padding * 2, textHeight + padding * 2, false);
            
            // テキストを描画
            graphics.drawText(victoryText, textX, textY, "default", textColor);
            
            SDL_DestroyTexture(textTexture);
        }
        
        graphics.present();
        return;
    }
    
    if (currentPhase == BattlePhase::LEVEL_UP_DISPLAY) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        int centerX = screenWidth / 2;
        int centerY = screenHeight / 2;
        
        // 画面をクリア（背景画像で覆う前に）
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.clear();
        
        // 背景画像を描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
        SDL_Texture* bgTexture = graphics.getTexture("battle_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
        
        // プレイヤーと敵のキャラクター描画（renderResultAnnouncementと同じ構成）
        auto& charState = animationController->getCharacterState();
        int playerBaseX = screenWidth / 4;
        int playerBaseY = screenHeight / 2;
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        int playerWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
        int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
        
        // HP表示（プレイヤーのみ表示）
        SDL_Color whiteColor = {255, 255, 255, 255};
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        
        // プレイヤーの名前とレベル（HPの上に表示）
        std::string playerNameText = player->getName() + " Lv." + std::to_string(player->getLevel());
        SDL_Texture* playerNameTexture = graphics.createTextTexture(playerNameText, "default", whiteColor);
        if (playerNameTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerNameTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = playerX - 100 - padding;
            int bgY = playerY - playerHeight / 2 - 80 - padding;
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(playerNameTexture);
        }
        graphics.drawText(playerNameText, playerX - 100, playerY - playerHeight / 2 - 80, "default", whiteColor);
        
        std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
        SDL_Texture* playerHpTexture = graphics.createTextTexture(playerHpText, "default", whiteColor);
        if (playerHpTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerHpTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = playerX - 100 - padding;
            int bgY = playerY - playerHeight / 2 - 40 - padding;
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(playerHpTexture);
        }
        graphics.drawText(playerHpText, playerX - 100, playerY - playerHeight / 2 - 40, "default", whiteColor);
        
        // プレイヤーのみ描画（敵は描画しない）
        SDL_Texture* playerTex = graphics.getTexture("player");
        if (playerTex) {
            graphics.drawTexture(playerTex, playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight);
        } else {
            graphics.setDrawColor(100, 200, 255, 255);
            graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, false);
        }
        
        // レベルアップメッセージを中央に表示（renderResultAnnouncementの勝敗テキストの位置）
        int hpGain = player->getMaxHp() - oldMaxHp;
        int mpGain = player->getMaxMp() - oldMaxMp;
        int attackGain = player->getAttack() - oldAttack;
        int defenseGain = player->getDefense() - oldDefense;
        
        std::string levelUpText = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(player->getLevel()) + "になった！\n";
        levelUpText += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain);
        
        int levelGained = player->getLevel() - oldLevel;
        if (levelGained > 1) {
            levelUpText = "★レベルアップ！★\n" + player->getName() + "はレベル" + std::to_string(oldLevel) + "からレベル" + std::to_string(player->getLevel()) + "になった！\n";
            levelUpText += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain);
        }
        
        std::vector<SpellType> learnedSpells;
        for (int level = oldLevel + 1; level <= player->getLevel(); level++) {
            auto spellsAtLevel = Player::getSpellsLearnedAtLevel(level);
            learnedSpells.insert(learnedSpells.end(), spellsAtLevel.begin(), spellsAtLevel.end());
        }
        
        if (!learnedSpells.empty()) {
            levelUpText += "\n";
            for (const auto& spell : learnedSpells) {
                levelUpText += Player::getSpellName(spell) + " ";
            }
            levelUpText += "を覚えた！";
        }
        
        SDL_Color textColor = {255, 255, 255, 255};
        
        SDL_Texture* textTexture = graphics.createTextTexture(levelUpText, "default", textColor);
        if (textTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            int textX = centerX - textWidth / 2;
            int textY = centerY - textHeight / 2 - 100;
            int padding = 12;
            
            // 背景を描画
            graphics.setDrawColor(0, 0, 0, 200);
            graphics.drawRect(textX - padding, textY - padding,
                             textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(textX - padding, textY - padding,
                             textWidth + padding * 2, textHeight + padding * 2, false);
            
            // テキストを描画
            graphics.drawText(levelUpText, textX, textY, "default", textColor);
            
            SDL_DestroyTexture(textTexture);
        }
        
        graphics.present();
        return;
    }
    
    // JUDGE_RESULTフェーズの描画（VICTORY_DISPLAYやLEVEL_UP_DISPLAYに遷移していない場合のみ）
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
        
        // ヒットエフェクトを描画
        effectManager->renderHitEffects(graphics);
        
        graphics.present();
        return;
    }
    
    auto& shakeState = effectManager->getShakeState();
    
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
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
    
    // ステータス上昇呪文の状態を表示（HPの下）
    if (player->hasNextTurnBonusActive()) {
        float multiplier = player->getNextTurnMultiplier();
        int turns = player->getNextTurnBonusTurns();
        // 倍率を文字列に変換（小数点以下1桁まで表示）
        int multiplierInt = static_cast<int>(multiplier * 10);
        std::string multiplierStr = std::to_string(multiplierInt / 10) + "." + std::to_string(multiplierInt % 10);
        std::string statusText = "攻撃倍率: " + multiplierStr + "倍 (残り" + std::to_string(turns) + "ターン)";
        SDL_Color statusColor = {255, 255, 100, 255}; // 黄色
        int padding = 8;
        SDL_Texture* statusTexture = graphics.createTextTexture(statusText, "default", statusColor);
        if (statusTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(statusTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = playerHpX - padding;
            int bgY = playerHpY + 40;
            if (shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
                bgX += static_cast<int>(shakeState.shakeOffsetX);
                bgY += static_cast<int>(shakeState.shakeOffsetY);
            }
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics.setDrawColor(255, 255, 100, 255);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(statusTexture);
        }
        int statusTextX = playerHpX;
        int statusTextY = playerHpY + 40;
        if (shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
            statusTextX += static_cast<int>(shakeState.shakeOffsetX);
            statusTextY += static_cast<int>(shakeState.shakeOffsetY);
        }
        graphics.drawText(statusText, statusTextX, statusTextY, "default", statusColor);
    }
    
    // アニメーションのオフセットを適用
    auto& charState = animationController->getCharacterState();
    int playerAnimX = playerX + static_cast<int>(charState.playerAttackOffsetX + charState.playerHitOffsetX);
    int playerAnimY = playerY + static_cast<int>(charState.playerAttackOffsetY + charState.playerHitOffsetY);
    
    SDL_Texture* playerTexture = graphics.getTexture("player");
    if (playerTexture) {
        graphics.drawTexture(playerTexture, playerAnimX - playerWidth / 2, playerAnimY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerAnimX - playerWidth / 2, playerAnimY - playerHeight / 2, playerWidth, playerHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(playerAnimX - playerWidth / 2, playerAnimY - playerHeight / 2, playerWidth, playerHeight, false);
    }
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyX = enemyBaseX;
    int enemyY = enemyBaseY;
    
    if (!shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        enemyX += static_cast<int>(shakeState.shakeOffsetX);
        enemyY += static_cast<int>(shakeState.shakeOffsetY);
    }
    
    // アニメーションのオフセットを適用
    int enemyAnimX = enemyX + static_cast<int>(charState.enemyAttackOffsetX + charState.enemyHitOffsetX);
    int enemyAnimY = enemyY + static_cast<int>(charState.enemyAttackOffsetY + charState.enemyHitOffsetY);
    
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
        graphics.drawTexture(enemyTexture, enemyAnimX - enemyWidth / 2, enemyAnimY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyAnimX - enemyWidth / 2, enemyAnimY - enemyHeight / 2, enemyWidth, enemyHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(enemyAnimX - enemyWidth / 2, enemyAnimY - enemyHeight / 2, enemyWidth, enemyHeight, false);
    }
    
    effectManager->renderHitEffects(graphics);
    
    // ui.render(graphics); // バトルログなどの下部UIを非表示
    
    // if (nightTimerActive) {
    //     CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
    //     CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
    //     CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
    // }
    
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
    // 背景画像はrender()で必要に応じて読み込む
}

void BattleState::showSpellMenu() {
}

void BattleState::handleSpellSelection(int spellChoice) {
    // 新しいシステムでは使用しない（自動選択されるため）
    // 互換性のために空の実装を残す
    (void)spellChoice;
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
            
            victoryExpGained = 2000;
            victoryGoldGained = 10000;
            victoryEnemyName = enemy->getTypeName();
            
            currentPhase = BattlePhase::VICTORY_DISPLAY;
            phaseTimer = 0;
        } else {
            int expGained = enemy->getExpReward();
            int goldGained = enemy->getGoldReward();
            player->gainExp(expGained);
            player->gainGold(goldGained);
            
            player->changeKingTrust(3);
            
            victoryExpGained = expGained;
            victoryGoldGained = goldGained;
            victoryEnemyName = enemy->getTypeName();
            
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
        
        // 新しい3種類の魔法
        currentOptions.push_back("回復魔法");
        currentOptions.push_back("ステータスアップ魔法");
        currentOptions.push_back("攻撃魔法");
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
            // MP消費の概念を削除：習得済みの呪文があれば使用可能
                cmd = 2;
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
            if (waitingForSpellSelection) {
                // 結果フェーズで呪文選択中の場合は、結果フェーズに戻る
                waitingForSpellSelection = false;
                if (battleLogic->getIsDesperateMode()) {
                    currentPhase = BattlePhase::DESPERATE_JUDGE_RESULT;
                } else {
                    currentPhase = BattlePhase::JUDGE_RESULT;
                }
                isShowingOptions = false;
                return;
            } else {
                returnToPlayerTurn();
            }
        } else {
            // 新しい3種類の魔法の選択
            SpellType selectedSpell;
            if (selected.find("回復魔法") != std::string::npos) {
                selectedSpell = SpellType::HEAL;
            } else if (selected.find("ステータスアップ魔法") != std::string::npos) {
                selectedSpell = SpellType::STATUS_UP;
            } else if (selected.find("攻撃魔法") != std::string::npos) {
                selectedSpell = SpellType::ATTACK;
        } else {
                    return;
                }
            
            // 魔法を実行
            bool isResultPhase = waitingForSpellSelection;
            bool isAttackSpell = (selectedSpell == SpellType::ATTACK);
            
            int result = 0;
            if (isResultPhase && isAttackSpell) {
                // 結果フェーズで攻撃魔法を選択した場合、ダメージを計算するだけ（適用はしない）
                int baseAttack = player->getTotalAttack();
                if (player->hasNextTurnBonusActive()) {
                    baseAttack = static_cast<int>(baseAttack * player->getNextTurnMultiplier());
                }
                int baseDamage = baseAttack;
                result = std::max(1, baseDamage - enemy->getEffectiveDefense());
                
                if (result > 0) {
                    // ダメージエントリを追加
                    auto stats = battleLogic->getStats();
                    float multiplier = battleLogic->getIsDesperateMode() ? 1.5f : (stats.hasThreeWinStreak ? BattleConstants::THREE_WIN_STREAK_MULTIPLIER : 1.0f);
                    BattleLogic::DamageInfo spellDamage;
                    spellDamage.damage = result;
                    spellDamage.isPlayerHit = false;
                    spellDamage.isDraw = false;
                    spellDamage.playerDamage = 0;
                    spellDamage.enemyDamage = 0;
                    spellDamage.commandType = BattleConstants::COMMAND_SPELL;
                    spellDamage.isCounterRush = false;
                    spellDamage.skipAnimation = false;
                    pendingDamages.push_back(spellDamage);
                    
                    // 呪文で勝利したターンを1つ処理済みにする
                    if (!spellWinTurns.empty()) {
                        spellWinTurns.erase(spellWinTurns.begin());
                    }
                    
                    // まだ呪文で勝利したターンがある場合、再度呪文選択フェーズに移行
                    if (!spellWinTurns.empty()) {
                        waitingForSpellSelection = true;
                        currentPhase = BattlePhase::SPELL_SELECTION;
                        isShowingOptions = false;
                        hideMessage();
                    return;
                    } else {
                        // 全ての呪文を処理した場合、結果フェーズに戻る
                        waitingForSpellSelection = false;
                        animationController->resetResultAnimation();
                        damageAppliedInAnimation = false;
                        if (battleLogic->getIsDesperateMode()) {
                            currentPhase = BattlePhase::DESPERATE_JUDGE_RESULT;
                        } else {
                            currentPhase = BattlePhase::JUDGE_RESULT;
                        }
                        hideMessage();
                    return;
                }
                }
            } else if (isResultPhase && !isAttackSpell) {
                // 結果フェーズで攻撃以外の魔法を選択した場合、効果を適用して結果フェーズに戻る
                result = player->castSpell(selectedSpell, enemy.get());
                switch (selectedSpell) {
                    case SpellType::HEAL:
                        {
                            std::string healMessage = player->getName() + "は回復魔法を唱えた！\nHP" + std::to_string(result) + "回復！";
                            addBattleLog(healMessage);
                        }
                        break;
                    case SpellType::STATUS_UP:
                        {
                            if (result > 0) {
                                std::string successMessage = player->getName() + "はステータスアップ魔法を唱えた！\n次の攻撃が2.5倍になる！";
                                addBattleLog(successMessage);
                            }
                        }
                        break;
                    default:
                        break;
                }
                
                // 呪文で勝利したターンを1つ処理済みにする
                if (!spellWinTurns.empty()) {
                    spellWinTurns.erase(spellWinTurns.begin());
                }
                
                // まだ呪文で勝利したターンがある場合、再度呪文選択フェーズに移行
                if (!spellWinTurns.empty()) {
                    waitingForSpellSelection = true;
                    currentPhase = BattlePhase::SPELL_SELECTION;
                    isShowingOptions = false;
                    hideMessage();
                    return;
                } else {
                    // 全ての呪文を処理した場合、結果フェーズに戻る
                    waitingForSpellSelection = false;
                    animationController->resetResultAnimation();
                    damageAppliedInAnimation = false;
                    if (battleLogic->getIsDesperateMode()) {
                        currentPhase = BattlePhase::DESPERATE_JUDGE_RESULT;
                    } else {
                        currentPhase = BattlePhase::JUDGE_RESULT;
                    }
                    hideMessage();
                    return;
                }
            } else if (!isResultPhase) {
                // 通常のフェーズ（結果フェーズ以外）で魔法を選択した場合
                result = player->castSpell(selectedSpell, enemy.get());
                switch (selectedSpell) {
                    case SpellType::HEAL:
                        {
                            std::string healMessage = player->getName() + "は回復魔法を唱えた！\nHP" + std::to_string(result) + "回復！";
                            addBattleLog(healMessage);
                        }
                        break;
                    case SpellType::STATUS_UP:
                        {
                            if (result > 0) {
                                std::string successMessage = player->getName() + "はステータスアップ魔法を唱えた！\n次の攻撃が2.5倍になる！";
                                addBattleLog(successMessage);
                            }
                        }
                        break;
                    case SpellType::ATTACK:
                        {
                            if (player->hasNextTurnBonusActive()) {
                                addBattleLog(player->getName() + "の攻撃ボーナス効果が発動した！");
                                player->processNextTurnBonus();
                            }
                            if (result > 0) {
                                std::string damageMessage = player->getName() + "は攻撃魔法を唱えた！\n" + enemy->getTypeName() + "は" + std::to_string(result) + "のダメージを受けた！";
                                addBattleLog(damageMessage);
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
            return "自分のHPが4割回復する。\nMP消費なし";
        case 1:
            return "次の攻撃が2.5倍になる。\nMP消費なし";
        case 2:
            return "通常の攻撃と同じ攻撃力の攻撃。\nMP消費なし";
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
    
    // 新しいシステム：常に魔法を選択可能（取得済みかのチェック不要）
        currentOptions.push_back("呪文");
    
    selectedOption = 0;
    isShowingOptions = true;
    
    animationController->resetCommandSelectAnimation();
}

void BattleState::prepareJudgeResults() {
    turnResults.clear();
    
    auto judgeResults = battleLogic->judgeAllRounds();
    // calculateBattleStats()を呼んでstatsを更新する（メンバ変数のstatsが更新される）
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
    // 呪文で勝利したターンを記録
    spellWinTurns.clear();
    auto playerCmds = battleLogic->getPlayerCommands();
    auto enemyCmds = battleLogic->getEnemyCommands();
    auto stats = battleLogic->getStats();
    
    if (stats.playerWins > stats.enemyWins) {
        for (int i = 0; i < battleLogic->getCommandTurnCount(); i++) {
            if (battleLogic->judgeRound(playerCmds[i], enemyCmds[i]) == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
                if (playerCmds[i] == BattleConstants::COMMAND_SPELL) {
                    spellWinTurns.push_back(i);
                }
            }
        }
    }
    
    pendingDamages = battleLogic->prepareDamageList(damageMultiplier);
    
    // 呪文で勝利したターンのダメージエントリを削除（後でプレイヤーが選択した呪文を実行するため）
    // ただし、攻撃呪文の場合はダメージエントリを残す（既に追加されている）
    // ここでは、呪文で勝利したターンは記録だけして、ダメージエントリは削除しない
    // （攻撃呪文の場合は既にダメージエントリが追加されているため）
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
                    // 攻撃魔法：通常の攻撃と同じ攻撃力（MP消費なし）
                        int baseDamage = player->calculateDamageWithBonus(*enemy);
                    int damage = static_cast<int>(baseDamage * damageMultiplier); // 攻撃魔法は通常の攻撃と同じ
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
    // フェーズが既に変更されている場合は処理をスキップ
    if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
        return;
    }
    
    auto stats = battleLogic->getStats();
    bool isVictory = (stats.playerWins > stats.enemyWins);
    bool isDefeat = (stats.enemyWins > stats.playerWins);
    
    // ダメージリストの準備（初回のみ）
    if (pendingDamages.empty()) {
        float multiplier = isDesperateMode ? 1.5f : (stats.hasThreeWinStreak ? BattleConstants::THREE_WIN_STREAK_MULTIPLIER : 1.0f);
        prepareDamageList(multiplier);
        damageAppliedInAnimation = false;
        currentExecutingTurn = 0;
        executeDelayTimer = 0.0f;
        
        // 呪文で勝利したターンがある場合、自動で呪文を選択して処理
        while (!spellWinTurns.empty() && !waitingForSpellSelection) {
            // HPの状態に応じて自動で呪文を選択
            float hpRatio = static_cast<float>(player->getHp()) / static_cast<float>(player->getMaxHp());
            SpellType selectedSpell = SpellType::ATTACK; // デフォルトは攻撃魔法
            
            // ステータス上昇呪文が既に有効かどうかを確認
            bool hasStatusBuff = player->hasNextTurnBonusActive();
            
            // 新しい3種類の魔法の自動選択ロジック
            if (hpRatio <= 0.3f) {
                // HPが3割以下：回復魔法
                selectedSpell = SpellType::HEAL;
            } else if (hasStatusBuff) {
                // ステータスアップが有効な場合：攻撃魔法
                selectedSpell = SpellType::ATTACK;
                } else {
                // HPが3割以上 && ステータスアップが有効でない：ステータスアップ魔法
                selectedSpell = SpellType::STATUS_UP;
            }
            
            // 選択した魔法を処理
            bool isAttackSpell = (selectedSpell == SpellType::ATTACK);
            
            int result = 0;
            if (isAttackSpell) {
                // 攻撃魔法の場合、通常の攻撃と同じ攻撃力でダメージを計算（適用はしない）
                    int baseAttack = player->getTotalAttack();
                    if (player->hasNextTurnBonusActive()) {
                        baseAttack = static_cast<int>(baseAttack * player->getNextTurnMultiplier());
                    }
                    
                // 通常の攻撃と同じ計算（倍率1.0倍）
                int baseDamage = baseAttack;
                        result = std::max(1, baseDamage - enemy->getEffectiveDefense());
                
                if (result > 0) {
                    // ダメージエントリを追加
                    auto stats = battleLogic->getStats();
                    BattleLogic::DamageInfo spellDamage;
                    spellDamage.damage = result;
                    spellDamage.isPlayerHit = false;
                    spellDamage.isDraw = false;
                    spellDamage.playerDamage = 0;
                    spellDamage.enemyDamage = 0;
                    spellDamage.commandType = BattleConstants::COMMAND_SPELL;
                    spellDamage.isCounterRush = false;
                    spellDamage.skipAnimation = false;  // 攻撃魔法なのでアニメーションを実行
                    pendingDamages.push_back(spellDamage);
                }
            } else {
                // 攻撃以外の魔法の場合、効果を適用
                result = player->castSpell(selectedSpell, enemy.get());
                
                // ステータスアップ魔法や回復魔法を使った場合は、ダミーのダメージエントリを追加（アニメーションはスキップ）
                BattleLogic::DamageInfo spellDamage;
                spellDamage.damage = 0;
                spellDamage.isPlayerHit = false;
                spellDamage.isDraw = false;
                spellDamage.playerDamage = 0;
                spellDamage.enemyDamage = 0;
                spellDamage.commandType = BattleConstants::COMMAND_SPELL;
                spellDamage.isCounterRush = false;
                spellDamage.skipAnimation = true;  // ステータスアップ魔法や回復魔法なのでアニメーションをスキップ
                pendingDamages.push_back(spellDamage);
                
                // ログに記録
                switch (selectedSpell) {
                    case SpellType::HEAL:
                        {
                            std::string healMessage = player->getName() + "は回復魔法を唱えた！\nHP" + std::to_string(result) + "回復！";
                            addBattleLog(healMessage);
                        }
                        break;
                    case SpellType::STATUS_UP:
                        {
                            if (result > 0) {
                                std::string successMessage = player->getName() + "はステータスアップ魔法を唱えた！\n次の攻撃が2.5倍になる！";
                                addBattleLog(successMessage);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            
            // 呪文で勝利したターンを1つ処理済みにする
            if (!spellWinTurns.empty()) {
                spellWinTurns.erase(spellWinTurns.begin());
            }
        }
        
        // 呪文で勝利したターンが全て処理され、かつpendingDamagesが空の場合、
        // ダミーのダメージエントリを追加してアニメーションを実行できるようにする
        // （ただし、ステータス上昇呪文や回復呪文の場合はアニメーションをスキップ）
        if (pendingDamages.empty() && spellWinTurns.empty() && isVictory) {
            BattleLogic::DamageInfo dummyDamage;
            dummyDamage.damage = 0;  // ダメージは適用しない（アニメーションのみ実行）
            dummyDamage.isPlayerHit = false;
            dummyDamage.isDraw = false;
            dummyDamage.playerDamage = 0;
            dummyDamage.enemyDamage = 0;
            dummyDamage.commandType = BattleConstants::COMMAND_SPELL;
            dummyDamage.isCounterRush = false;
            dummyDamage.skipAnimation = true;  // ダミーエントリなのでアニメーションをスキップ
            pendingDamages.push_back(dummyDamage);
        }
        
        // 最初のアニメーションを開始（pendingDamagesに追加された後）
        if (!pendingDamages.empty()) {
            animationController->resetResultAnimation();
        }
    }
    
    // アニメーション更新（pendingDamagesが空でない場合のみ）
    if (!pendingDamages.empty()) {
        animationController->updateResultAnimation(deltaTime, isDesperateMode);
    }
    auto& resultState = animationController->getResultState();
    
    // 現在のダメージに対してアニメーションを実行
    if (currentExecutingTurn < pendingDamages.size()) {
        auto& damageInfo = pendingDamages[currentExecutingTurn];
        
        // アニメーションをスキップする場合は、アニメーション更新をスキップ
        if (!damageInfo.skipAnimation) {
        // カウンターラッシュの場合はアニメーション時間を短縮
        bool isCounterRush = damageInfo.isCounterRush;
        float animStartTime = isCounterRush ? 0.1f : 0.5f;
        float animDuration = isCounterRush ? 0.3f : 1.0f;
        
        // アニメーション更新（現在のダメージ用）
        animationController->updateResultCharacterAnimation(
            deltaTime, isVictory, isDefeat, resultState.resultAnimationTimer,
            damageAppliedInAnimation, stats.hasThreeWinStreak,
            animStartTime, animDuration);
        }
        
        // アニメーションをスキップする場合は、ダメージ適用処理もスキップ
        if (damageInfo.skipAnimation) {
            // アニメーションをスキップする場合、すぐに次のダメージへ進む
            currentExecutingTurn++;
            damageAppliedInAnimation = false;
            if (currentExecutingTurn < pendingDamages.size()) {
                animationController->resetResultAnimation();
            }
        } else {
            // カウンターラッシュの場合はアニメーション時間を短縮
            bool isCounterRush = damageInfo.isCounterRush;
            float animStartTime = isCounterRush ? 0.1f : 0.5f;
            float animDuration = isCounterRush ? 0.3f : 1.0f;
        
        // アニメーションのピーク時（攻撃が当たる瞬間）にダメージを適用
        float animTime = resultState.resultAnimationTimer - animStartTime;
        
        if (animTime >= 0.0f && animTime <= animDuration) {
            float progress = animTime / animDuration;
            // ピーク時（progress = 0.2f付近）にダメージを適用
            if (progress >= BattleConstants::ATTACK_ANIMATION_HIT_MOMENT - BattleConstants::ATTACK_ANIMATION_HIT_WINDOW &&
                progress <= BattleConstants::ATTACK_ANIMATION_HIT_MOMENT + BattleConstants::ATTACK_ANIMATION_HIT_WINDOW &&
                !damageAppliedInAnimation) {
                
                // 現在のダメージを適用
                auto& damageInfo = pendingDamages[currentExecutingTurn];
                
                if (damageInfo.isDraw) {
                    // 引き分けの場合、両方にダメージを適用
                    if (damageInfo.playerDamage > 0) {
                        player->takeDamage(damageInfo.playerDamage);
                        int playerX = BattleConstants::PLAYER_POSITION_X;
                        int playerY = BattleConstants::PLAYER_POSITION_Y;
                        effectManager->triggerHitEffect(damageInfo.playerDamage, playerX, playerY, true);
                    }
                    if (damageInfo.enemyDamage > 0) {
                        enemy->takeDamage(damageInfo.enemyDamage);
                        int enemyX = BattleConstants::ENEMY_POSITION_X;
                        int enemyY = BattleConstants::ENEMY_POSITION_Y;
                        effectManager->triggerHitEffect(damageInfo.enemyDamage, enemyX, enemyY, false);
                        
                        // 引き分けでも通常攻撃または攻撃呪文の場合、ステータスアップ効果を切る
                        // （引き分けの場合はcommandTypeが-1なので、判定をスキップ）
                        // 通常は引き分けでは攻撃呪文は使われないが、念のため
                    }
                    // 引き分け時のスクリーンシェイク
                    float intensity = isDesperateMode ? BattleConstants::DESPERATE_DRAW_SHAKE_INTENSITY : BattleConstants::DRAW_SHAKE_INTENSITY;
                    float shakeDuration = isDesperateMode ? 0.5f : 0.3f;
                    effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                } else {
                    int damage = damageInfo.damage;
                    bool isPlayerHit = damageInfo.isPlayerHit;
                    
                    // ダミーのダメージエントリ（damage == 0）の場合はダメージ適用をスキップ
                    // アニメーションは実行するが、実際のダメージは適用しない
                    if (damage > 0) {
                    if (isPlayerHit) {
                        player->takeDamage(damage);
                        int playerX = BattleConstants::PLAYER_POSITION_X;
                        int playerY = BattleConstants::PLAYER_POSITION_Y;
                        effectManager->triggerHitEffect(damage, playerX, playerY, true);
                        // 引き分けと同じスクリーンシェイク
                        float intensity = isDesperateMode ? BattleConstants::DESPERATE_DRAW_SHAKE_INTENSITY : BattleConstants::DRAW_SHAKE_INTENSITY;
                        float shakeDuration = isDesperateMode ? 0.5f : 0.3f;
                        effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                    } else {
                        enemy->takeDamage(damage);
                        int enemyX = BattleConstants::ENEMY_POSITION_X;
                        int enemyY = BattleConstants::ENEMY_POSITION_Y;
                        effectManager->triggerHitEffect(damage, enemyX, enemyY, false);
                            
                            // 通常攻撃または攻撃呪文の場合、ステータスアップ効果を切る
                            if (damageInfo.commandType == BattleConstants::COMMAND_ATTACK || 
                                damageInfo.commandType == BattleConstants::COMMAND_SPELL) {
                                if (player->hasNextTurnBonusActive()) {
                                    player->clearNextTurnBonus();
                                }
                            }
                            
                        // 引き分けと同じスクリーンシェイク
                        float intensity = isDesperateMode ? BattleConstants::DESPERATE_DRAW_SHAKE_INTENSITY : BattleConstants::DRAW_SHAKE_INTENSITY;
                        float shakeDuration = isDesperateMode ? 0.5f : 0.3f;
                        effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                        }
                    }
                }
                
                damageAppliedInAnimation = true;
            }
        }
        
        // アニメーション完了後、次のダメージへ進む
        float animTotalDuration = animStartTime + animDuration;
        if (resultState.resultAnimationTimer >= animTotalDuration) {
            // 現在のアニメーション完了
            currentExecutingTurn++;
            damageAppliedInAnimation = false;
            // 次のダメージのアニメーションを開始
            if (currentExecutingTurn < pendingDamages.size()) {
                animationController->resetResultAnimation();
            }
            // 最後のダメージ完了時は、タイマーをリセットせず、そのまま継続させる
            // （最後のダメージ完了後、一定時間待ってからフェーズ遷移するため）
            }
        }
    }
    
    // 最初のアニメーション開始時のスクリーンシェイク（勝利/敗北/引き分け）
    float shakeDuration = isDesperateMode ? 0.5f : 0.3f;
    if (resultState.resultAnimationTimer < shakeDuration && !resultState.resultShakeActive && currentExecutingTurn == 0) {
        resultState.resultShakeActive = true;
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
    
    // 全ダメージ適用完了後、戦闘終了判定
    if (currentExecutingTurn >= pendingDamages.size()) {
        constexpr float animTotalDuration = 0.5f + 1.0f; // 1.5秒
        // 最後のダメージ完了時点から、さらに1.5秒待機（合計3秒）
        constexpr float finalWaitDuration = animTotalDuration * 2.0f; // 3.0秒
        // タイマーが一定時間経過したらフェーズ遷移（フォールバック：最大4秒待機）
        constexpr float maxWaitTime = 4.0f;
        if (resultState.resultAnimationTimer >= finalWaitDuration || phaseTimer >= maxWaitTime) {
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
                // checkBattleEnd()でフェーズが変更された場合は、ここで処理を終了（リセット処理は行わない）
                if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                    return;
                }
            }
            
            // リセット（フェーズが変更されていない場合のみ）
            animationController->resetResultAnimation();
            currentExecutingTurn = 0;
            executeDelayTimer = 0.0f;
            damageAppliedInAnimation = false;
            pendingDamages.clear();
            phaseTimer = 0;
        }
    } else if (pendingDamages.empty() && phaseTimer > BattleConstants::JUDGE_RESULT_ANNOUNCEMENT_DURATION) {
        // ダメージが1つもない場合（引き分けなど）は即座に終了
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
            // checkBattleEnd()でフェーズが変更された場合は、ここで処理を終了（リセット処理は行わない）
            if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                return;
            }
        }
        
        // リセット（フェーズが変更されていない場合のみ）
        animationController->resetResultAnimation();
        currentExecutingTurn = 0;
        executeDelayTimer = 0.0f;
        damageAppliedInAnimation = false;
        pendingDamages.clear();
        phaseTimer = 0;
    }
}
