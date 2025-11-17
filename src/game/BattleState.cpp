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
#include <chrono>
#include <cmath> // abs関数のために追加
#include <iostream> // デバッグ情報のために追加
#include <nlohmann/json.hpp>

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
      introScale(0.0f), introTimer(0.0f),
      currentResidentPlayerCommand(-1), currentResidentCommand(-1),
      cachedResidentBehaviorHint(""), cachedResidentCommand(-1),
      residentTurnCount(0),
      residentAttackFailed(false),
      showGameExplanation(false), explanationStep(0),
      explanationMessageBoard(nullptr) {
    
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
                phaseTimer = 0;
                introScale = 1.0f;
                introTextScale = 1.0f;
                
                // 住民との戦闘の場合は1ターンずつ、通常の戦闘は3ターンずつ
                currentPhase = BattlePhase::COMMAND_SELECT;
                if (enemy->isResident()) {
                    battleLogic->setCommandTurnCount(1);  // 住民戦は1ターンずつ
                    residentTurnCount = 1;  // 住民戦のターン数を初期化
                } else {
                    battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                }
                battleLogic->setDesperateMode(false);
                isFirstCommandSelection = true;
                initializeCommandSelection();
            }
            break;
        }
            
        case BattlePhase::COMMAND_SELECT:
            // 住民戦の場合は既にJUDGEフェーズに遷移しているので、ここでは処理しない
            if (enemy->isResident()) {
                break;
            }
            
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
            
            
        case BattlePhase::RESIDENT_TURN_RESULT:
            // 住民との戦闘用：ターン結果処理（processResidentTurn内で処理）
            break;
            
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
                // 住民との戦闘の場合はレベルアップをスキップ
                if (enemy->isResident()) {
                    endBattle();
                } else if (player->getLevel() > oldLevel) {
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
    
    // 初回の戦闘時のみ説明を開始（UI初期化後、住民戦以外）
    // ただし、COMMAND_SELECTフェーズで設定するので、ここでは設定しない
    
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
        SDL_Texture* bgTexture = getBattleBackgroundTexture(graphics);
        if (bgTexture) {
            // 背景画像を画面全体に描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
    }
    
    // INTROフェーズの特別な描画
    if (currentPhase == BattlePhase::INTRO) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        
        // 背景画像を描画
        SDL_Texture* bgTexture = getBattleBackgroundTexture(graphics);
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
        
        // 中央に敵を配置
        int enemyX = screenWidth / 2;
        int enemyY = screenHeight / 2;
        constexpr int BASE_ENEMY_SIZE = 300;
    
    // 住民の場合は住民の画像を使用、それ以外は通常の敵画像を使用
    SDL_Texture* enemyTexture = nullptr;
    if (enemy->isResident()) {
        int textureIndex = enemy->getResidentTextureIndex();
        std::string textureName = "resident_" + std::to_string(textureIndex + 1);
        enemyTexture = graphics.getTexture(textureName);
    } else {
        enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    }
    
    if (enemyTexture) {
            // 元の画像サイズを取得してアスペクト比を保持
            int textureWidth, textureHeight;
            SDL_QueryTexture(enemyTexture, nullptr, nullptr, &textureWidth, &textureHeight);
            
            // 基準サイズをアスペクト比を保持して計算
            float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
            int enemyWidth, enemyHeight;
            if (textureWidth > textureHeight) {
                // 横長の画像
                enemyWidth = static_cast<int>(BASE_ENEMY_SIZE * introScale);
                enemyHeight = static_cast<int>((BASE_ENEMY_SIZE * introScale) / aspectRatio);
            } else {
                // 縦長または正方形の画像
                enemyHeight = static_cast<int>(BASE_ENEMY_SIZE * introScale);
                enemyWidth = static_cast<int>((BASE_ENEMY_SIZE * introScale) * aspectRatio);
            }
            
            graphics.drawTexture(enemyTexture, enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
            int fallbackWidth = static_cast<int>(BASE_ENEMY_SIZE * introScale);
            int fallbackHeight = static_cast<int>(BASE_ENEMY_SIZE * introScale);
            graphics.setDrawColor(255, 100, 100, 255);
            graphics.drawRect(enemyX - fallbackWidth / 2, enemyY - fallbackHeight / 2, fallbackWidth, fallbackHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(enemyX - fallbackWidth / 2, enemyY - fallbackHeight / 2, fallbackWidth, fallbackHeight, false);
        }
        
        // 敵の下にテキストを表示（スケールアニメーション付き）
        // 住民の場合は住民の名前を使用、それ以外は通常の敵名を使用
        std::string enemyName = enemy->isResident() ? enemy->getName() : enemy->getTypeName();
        std::string appearText = enemyName + "があらわれた！";
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
        
        // 住民戦の場合はコマンド名を直接指定
        if (enemy->isResident()) {
            params.playerCommandName = getPlayerCommandNameForResident(currentResidentPlayerCommand);
            params.enemyCommandName = getResidentCommandName(currentResidentCommand);
            params.judgeResult = judgeResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
            params.residentBehaviorHint = getResidentBehaviorHint();
            params.residentTurnCount = residentTurnCount;  // 住民戦の現在のターン数
        } else {
            params.playerCommandName = "";
            params.enemyCommandName = "";
            params.judgeResult = -999; // 未設定（battleLogicから取得）
            params.residentBehaviorHint = "";
            params.residentTurnCount = 0;  // 通常戦の場合は0
        }
        
        battleUI->renderJudgeAnimation(params);
        
        // 勝敗UIを表示（ジャッジフェーズ中は常に更新）
        renderWinLossUI(graphics, false);
        
        // if (nightTimerActive) {
        //     CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        //     CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
        //     CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        // }
        
        // Rock-Paper-Scissors画像を表示（住民戦以外）
        renderRockPaperScissorsImage(graphics);
        
        graphics.present();
        return;
    }
    
    // 住民戦でも通常のJUDGEとJUDGE_RESULTフェーズを使用するため、特別な描画処理は不要
    
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) && 
        isShowingOptions) {
        // 説明が設定されていない場合は設定する（初回の戦闘時のみ、住民戦以外）
        if (showGameExplanation && !enemy->isResident() && explanationMessageBoard && explanationMessageBoard->getText().empty()) {
            if (!gameExplanationTexts.empty() && explanationStep == 0) {
                showExplanationMessage(gameExplanationTexts[0]);
            }
        }
        
        BattleUI::CommandSelectRenderParams params;
        params.currentSelectingTurn = currentSelectingTurn;
        params.commandTurnCount = battleLogic->getCommandTurnCount();
        params.isDesperateMode = battleLogic->getIsDesperateMode();
        params.selectedOption = selectedOption;
        params.currentOptions = &currentOptions;
        // 住民戦の場合は住民の様子を渡す
        if (enemy->isResident()) {
            params.residentBehaviorHint = getResidentBehaviorHint();
            params.residentTurnCount = residentTurnCount;  // 住民戦の現在のターン数
        } else {
            params.residentBehaviorHint = "";
            params.residentTurnCount = 0;  // 通常戦の場合は0
        }
        battleUI->renderCommandSelectionUI(params);
        
        // 説明UIを表示（初回の戦闘時のみ、住民戦以外）
        if (showGameExplanation && !enemy->isResident() && explanationMessageBoard && !explanationMessageBoard->getText().empty()) {
            auto& config = UIConfig::UIConfigManager::getInstance();
            auto mbConfig = config.getMessageBoardConfig();
            
            int bgX = 20;  // 左下に配置
            int bgHeight = 60;  // 2行分の高さ
            int bgY = graphics.getScreenHeight() - bgHeight - 20;  // 画面下部から20px上
            int bgWidth = graphics.getScreenWidth() / 2;
            
            graphics.setDrawColor(0, 0, 0, 200); // 半透明の黒色
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight);
        }
        
        // 古いUI要素を非表示にする（テキストを空にする）
        if (battleLogLabel) {
            battleLogLabel->setText("");
        }
        if (playerStatusLabel) {
            playerStatusLabel->setText("");
        }
        if (enemyStatusLabel) {
            enemyStatusLabel->setText("");
        }
        if (messageLabel) {
            messageLabel->setText("");
        }
        
        // UIを描画（説明メッセージボードを含む）
        ui.render(graphics);
        
        // if (nightTimerActive) {
        //     CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        //     CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
        //     CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        // }
        
        // Rock-Paper-Scissors画像を表示（住民戦以外）
        renderRockPaperScissorsImage(graphics);
        
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
        SDL_Texture* bgTexture = getBattleBackgroundTexture(graphics);
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
        
        // プレイヤーと敵のキャラクター描画（renderResultAnnouncementと同じ構成）
        auto& charState = animationController->getCharacterState();
        int playerBaseX = screenWidth / 4;
        int playerBaseY = screenHeight / 2;
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        SDL_Texture* playerTex = graphics.getTexture("player");
        
        // 元の画像サイズを取得してアスペクト比を保持（HP表示の位置計算用）
        int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
        if (playerTex) {
            int textureWidth, textureHeight;
            SDL_QueryTexture(playerTex, nullptr, nullptr, &textureWidth, &textureHeight);
            float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
            if (textureWidth <= textureHeight) {
                playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
            } else {
                playerHeight = static_cast<int>(BattleConstants::BATTLE_CHARACTER_SIZE / aspectRatio);
            }
        }
        
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
        if (playerTex) {
            graphics.drawTextureAspectRatio(playerTex, playerX, playerY, BattleConstants::BATTLE_CHARACTER_SIZE);
        } else {
            graphics.setDrawColor(100, 200, 255, 255);
            graphics.drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, false);
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
        SDL_Texture* bgTexture = getBattleBackgroundTexture(graphics);
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
        
        // プレイヤーと敵のキャラクター描画（renderResultAnnouncementと同じ構成）
        auto& charState = animationController->getCharacterState();
        int playerBaseX = screenWidth / 4;
        int playerBaseY = screenHeight / 2;
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        SDL_Texture* playerTex = graphics.getTexture("player");
        
        // 元の画像サイズを取得してアスペクト比を保持（HP表示の位置計算用）
        int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
        if (playerTex) {
            int textureWidth, textureHeight;
            SDL_QueryTexture(playerTex, nullptr, nullptr, &textureWidth, &textureHeight);
            float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
            if (textureWidth <= textureHeight) {
                playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
            } else {
                playerHeight = static_cast<int>(BattleConstants::BATTLE_CHARACTER_SIZE / aspectRatio);
            }
        }
        
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
        if (playerTex) {
            graphics.drawTextureAspectRatio(playerTex, playerX, playerY, BattleConstants::BATTLE_CHARACTER_SIZE);
        } else {
            graphics.setDrawColor(100, 200, 255, 255);
            graphics.drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, false);
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
        
        // 新しいシステム：魔法は初期から覚えているため、魔法習得メッセージは表示しない
        
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
        // 結果フェーズでもコマンド画像を表示するため、renderJudgeAnimationを呼ぶ
        BattleUI::JudgeRenderParams judgeParams;
        judgeParams.currentJudgingTurnIndex = currentJudgingTurnIndex;
        judgeParams.commandTurnCount = battleLogic->getCommandTurnCount();
        judgeParams.judgeSubPhase = JudgeSubPhase::SHOW_RESULT;  // 結果フェーズ
        judgeParams.judgeDisplayTimer = 0.0f;  // タイマーは使用しない
        
        // 住民戦の場合はコマンド名を直接指定
        if (enemy->isResident()) {
            judgeParams.playerCommandName = getPlayerCommandNameForResident(currentResidentPlayerCommand);
            judgeParams.enemyCommandName = getResidentCommandName(currentResidentCommand);
            judgeParams.judgeResult = judgeResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
            judgeParams.residentBehaviorHint = getResidentBehaviorHint();
            judgeParams.residentTurnCount = residentTurnCount;
        } else {
            judgeParams.playerCommandName = "";
            judgeParams.enemyCommandName = "";
            judgeParams.judgeResult = -999; // 未設定（battleLogicから取得）
            judgeParams.residentBehaviorHint = "";
            judgeParams.residentTurnCount = 0;
        }
        
        battleUI->renderJudgeAnimation(judgeParams);
        
        auto stats = battleLogic->getStats();
        BattleUI::ResultAnnouncementRenderParams params;
        
        // 住民戦の場合は、calculateCurrentWinLoss()の結果を使用
        if (enemy->isResident()) {
            auto winLoss = calculateCurrentWinLoss();
            params.playerWins = winLoss.first;
            params.enemyWins = winLoss.second;
            params.isVictory = (params.playerWins > params.enemyWins);
            params.isDefeat = (params.enemyWins > params.playerWins);
            params.isDesperateMode = false;
            params.hasThreeWinStreak = false;
            params.residentBehaviorHint = getResidentBehaviorHint();  // 住民戦の場合は住民の様子を渡す
        } else {
            params.isVictory = (stats.playerWins > stats.enemyWins);
            params.isDefeat = (stats.enemyWins > stats.playerWins);
            params.isDesperateMode = battleLogic->getIsDesperateMode();
            params.hasThreeWinStreak = stats.hasThreeWinStreak;
            params.playerWins = stats.playerWins;
            params.enemyWins = stats.enemyWins;
            params.residentBehaviorHint = "";  // 通常戦の場合は空文字列
        }
        
        battleUI->renderResultAnnouncement(params);
        
        // 勝敗UIを表示（結果フェーズでは「自分が〜ターン攻撃します」も表示）
        renderWinLossUI(graphics, true);
        
        // ヒットエフェクトを描画
        effectManager->renderHitEffects(graphics);
        
        // Rock-Paper-Scissors画像を表示（住民戦以外）
        renderRockPaperScissorsImage(graphics);
        
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
    
    SDL_Texture* playerTexture = graphics.getTexture("player");
    
    // 元の画像サイズを取得してアスペクト比を保持（HP表示の位置計算用）
    int playerHeight = 300;
    if (playerTexture) {
        int textureWidth, textureHeight;
        SDL_QueryTexture(playerTexture, nullptr, nullptr, &textureWidth, &textureHeight);
        float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
        if (textureWidth <= textureHeight) {
            playerHeight = 300;
        } else {
            playerHeight = static_cast<int>(300 / aspectRatio);
        }
    }
    
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
    
    if (playerTexture) {
        graphics.drawTextureAspectRatio(playerTexture, playerAnimX, playerAnimY, 300);
    } else {
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerAnimX - 300 / 2, playerAnimY - 300 / 2, 300, 300, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(playerAnimX - 300 / 2, playerAnimY - 300 / 2, 300, 300, false);
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
    
    // 住民の場合は住民の画像を使用、それ以外は通常の敵画像を使用
    SDL_Texture* enemyTexture = nullptr;
    if (enemy->isResident()) {
        int textureIndex = enemy->getResidentTextureIndex();
        std::string textureName = "resident_" + std::to_string(textureIndex + 1);
        enemyTexture = graphics.getTexture(textureName);
    } else {
        enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    }
    
    // 元の画像サイズを取得してアスペクト比を保持（HP表示の位置計算用）
    int enemyHeight = 300;
    if (enemyTexture) {
        int textureWidth, textureHeight;
        SDL_QueryTexture(enemyTexture, nullptr, nullptr, &textureWidth, &textureHeight);
        float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
        if (textureWidth <= textureHeight) {
            enemyHeight = 300;
        } else {
            enemyHeight = static_cast<int>(300 / aspectRatio);
        }
    }
    
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    int enemyHpX = enemyX - 100;
    int enemyHpY = enemyY - enemyHeight / 2 - 40;
    if (!shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        enemyHpX += static_cast<int>(shakeState.shakeOffsetX);
        enemyHpY += static_cast<int>(shakeState.shakeOffsetY);
    }
    graphics.drawText(enemyHpText, enemyHpX, enemyHpY, "default", enemyHpColor);
    
    if (enemyTexture) {
        graphics.drawTextureAspectRatio(enemyTexture, enemyAnimX, enemyAnimY, 300);
    } else {
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyAnimX - 300 / 2, enemyAnimY - 300 / 2, 300, 300, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(enemyAnimX - 300 / 2, enemyAnimY - 300 / 2, 300, 300, false);
    }
    
    effectManager->renderHitEffects(graphics);
    
    // Rock-Paper-Scissors画像を表示（住民戦以外）
    renderRockPaperScissorsImage(graphics);
    
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
    
    // 説明表示中の処理
    if (showGameExplanation && (currentPhase == BattlePhase::COMMAND_SELECT || 
                                currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT)) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            explanationStep++;
            
            if (explanationStep >= gameExplanationTexts.size()) {
                showGameExplanation = false;
                explanationStep = 0;
                clearExplanationMessage();
                
                // 説明UIが完全に終わったことを記録
                player->hasSeenBattleExplanation = true;
            } else {
                showExplanationMessage(gameExplanationTexts[explanationStep]);
            }
        }
        return; // 説明中は他の操作を無効化
    }
    
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
    
    // 説明用メッセージボード（左下に配置）
    auto mbConfig = config.getMessageBoardConfig();
    int explanationX = 30;  // 左下に配置（背景の内側に配置）
    int bgHeight = 60;  // 2行分の高さ
    int explanationY = graphics.getScreenHeight() - bgHeight - 10;  // 画面下部から背景の内側に配置
    auto explanationLabelPtr = std::make_unique<Label>(explanationX, explanationY, "", "default");
    explanationLabelPtr->setColor(mbConfig.text.color);
    explanationLabelPtr->setText("");
    explanationMessageBoard = explanationLabelPtr.get();
    ui.addElement(std::move(explanationLabelPtr));
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

SDL_Texture* BattleState::getBattleBackgroundTexture(Graphics& graphics) const {
    // 住民の場合は夜の背景、それ以外は通常の戦闘背景を使用
    SDL_Texture* bgTexture = nullptr;
    if (enemy->isResident()) {
        bgTexture = graphics.getTexture("night_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/night_bg.png", "night_bg");
        }
    } else {
        bgTexture = graphics.getTexture("battle_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
    }
    return bgTexture;
}

void BattleState::showSpellMenu() {
}

int BattleState::generateResidentCommand() {
    // 住民のコマンドを生成（怯える70%、助けを呼ぶ30%）
    // より確実な乱数生成のため、時間ベースのシードも使用
    std::random_device rd;
    unsigned int seed = rd() ^ static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dis(1, 100);
    
    int roll = dis(gen);
    // 確率分布：1-70 = 怯える（70%）、71-100 = 助けを呼ぶ（30%）
    if (roll <= 70) {
        return BattleConstants::RESIDENT_COMMAND_AFRAID;  // 怯える
    } else {
        // roll > 70 の場合は助けを呼ぶ（71-100の範囲、30%の確率）
        return BattleConstants::RESIDENT_COMMAND_CALL_HELP;  // 助けを呼ぶ
    }
}

void BattleState::processResidentTurn(int playerCommand, int residentCommand) {
    // 住民との戦闘の特殊ルールを処理
    if (playerCommand == BattleConstants::COMMAND_ATTACK) {
        // プレイヤーが攻撃を選択
        if (residentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID) {
            // 住民が怯える + プレイヤーが攻撃 → メンタルに基づく成功率で判定
            int mental = player->getMental();
            int successRate = 80; // 基本成功率80%
            
            // メンタルが低いほど成功率が下がる
            if (mental < 30) {
                successRate = 40;
            } else if (mental < 50) {
                successRate = 60;
            } else if (mental > 80) {
                successRate = 90; // メンタルが高いと成功率が上がる
            }
            
            // ランダム判定
            std::random_device rd;
            unsigned int seed = rd() ^ static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::mt19937 gen(seed);
            std::uniform_int_distribution<> dis(1, 100);
            int roll = dis(gen);
            
            if (roll <= successRate) {
                // 攻撃成功
            int baseAttack = player->getTotalAttack();
            int damage = std::max(1, baseAttack - enemy->getEffectiveDefense());
            
            BattleLogic::DamageInfo damageInfo;
            damageInfo.damage = damage;
            damageInfo.isPlayerHit = false;
            damageInfo.isDraw = false;
            damageInfo.playerDamage = 0;
            damageInfo.enemyDamage = damage;
            damageInfo.commandType = BattleConstants::COMMAND_ATTACK;
            damageInfo.isCounterRush = false;
            damageInfo.skipAnimation = false;
            
            pendingDamages.push_back(damageInfo);
            damageAppliedInAnimation = false;
            currentExecutingTurn = 0;
            executeDelayTimer = 0.0f;
            
            // アニメーションを開始
            animationController->resetResultAnimation();
            } else {
                // 攻撃失敗（メンタルが低くてためらった）
                addBattleLog("住民を倒すのをためらいました。");
                residentAttackFailed = true;
                // 結果フェーズに遷移して「ためらいました」を表示
                currentPhase = BattlePhase::JUDGE_RESULT;
                phaseTimer = 0.0f;
                // 次のターンへ（pendingDamagesは空のまま）
                residentTurnCount++;
                // 10ターン経過したらゲームオーバー
                if (residentTurnCount > 10) {
                    if (stateManager) {
                        stateManager->changeState(std::make_unique<GameOverState>(
                            player, "10ターン以内に倒せませんでした。衛兵に見つかりました。", enemy->getType(), enemy->getLevel(),
                            true, enemy->getName(), enemy->getResidentX(), enemy->getResidentY(), enemy->getResidentTextureIndex()));
                    }
                    return;
                }
            }
        } else if (residentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP) {
            // 住民が助けを呼ぶ + プレイヤーが攻撃 → ゲームオーバー（再戦可能）
            if (stateManager) {
                stateManager->changeState(std::make_unique<GameOverState>(
                    player, "衛兵に見つかりました。", enemy->getType(), enemy->getLevel(),
                    true, enemy->getName(), enemy->getResidentX(), enemy->getResidentY(), enemy->getResidentTextureIndex()));
            }
            return;
        }
    } else if (playerCommand == BattleConstants::PLAYER_COMMAND_HIDE) {
        // プレイヤーが身を隠すを選択 → 引き分け（何も起こらない）
        if (residentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID) {
            // 住民が怯える + プレイヤーが身を隠す → 何も起こらない
            addBattleLog("住民が怯えている。何も起こらなかった。");
        } else if (residentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP) {
            // 住民が助けを呼ぶ + プレイヤーが身を隠す → 戦闘続行
            addBattleLog("住民が助けを呼んだが、身を隠して見つからなかった。");
        }
        
        // 引き分けの場合は次のターンへ（pendingDamagesは空のまま）
        residentTurnCount++;
        // 10ターン経過したらゲームオーバー
        if (residentTurnCount > 10) {
            if (stateManager) {
                stateManager->changeState(std::make_unique<GameOverState>(
                    player, "10ターン以内に倒せませんでした。衛兵に見つかりました。", enemy->getType(), enemy->getLevel(),
                    true, enemy->getName(), enemy->getResidentX(), enemy->getResidentY(), enemy->getResidentTextureIndex()));
            }
            return;
        }
        currentPhase = BattlePhase::COMMAND_SELECT;
        battleLogic->setCommandTurnCount(1);
        initializeCommandSelection();
    }
}

void BattleState::handleSpellSelection(int spellChoice) {
    // 新しいシステムでは使用しない（自動選択されるため）
    // 互換性のために空の実装を残す
    (void)spellChoice;
}

std::string BattleState::getResidentCommandName(int command) {
    switch (command) {
        case BattleConstants::RESIDENT_COMMAND_AFRAID:
            return "怯える";
        case BattleConstants::RESIDENT_COMMAND_CALL_HELP:
            return "助けを呼ぶ";
        default:
            return "不明";
    }
}

std::string BattleState::getPlayerCommandNameForResident(int command) {
    switch (command) {
        case BattleConstants::COMMAND_ATTACK:
            return "攻撃";
        case BattleConstants::PLAYER_COMMAND_HIDE:
            return "身を隠す";
        default:
            return "不明";
    }
}

int BattleState::judgeResidentTurn(int playerCommand, int residentCommand) const {
    // 住民との戦闘の特殊ルールに基づく判定
    if (playerCommand == BattleConstants::COMMAND_ATTACK) {
        if (residentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID) {
            return BattleConstants::JUDGE_RESULT_PLAYER_WIN;  // 無条件で攻撃成功
        } else if (residentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP) {
            return BattleConstants::JUDGE_RESULT_ENEMY_WIN;  // ゲームオーバー
        }
    } else if (playerCommand == BattleConstants::PLAYER_COMMAND_HIDE) {
        if (residentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID) {
            return BattleConstants::JUDGE_RESULT_DRAW;  // 何も起こらない
        } else if (residentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP) {
            return BattleConstants::JUDGE_RESULT_PLAYER_WIN;  // 戦闘続行（プレイヤー有利）
        }
    }
    return BattleConstants::JUDGE_RESULT_DRAW;
}

std::string BattleState::getResidentBehaviorHint() const {
    // キャッシュされた住民の様子を返す（同じターン中は同じメッセージを表示）
    if (!cachedResidentBehaviorHint.empty()) {
        return cachedResidentBehaviorHint;
    }
    
    // キャッシュがない場合（念のため）は確率的に予測
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    int roll = dis(gen);
    if (roll <= 70) {
        // 怯えるの可能性が高い → 震えが止まらないようだ
        return "震えが止まらないようだ";
    } else {
        // 助けを呼ぶの可能性が高い → 辺りを見渡している
        return "辺りを見渡している";
    }
}

// 住民戦でも通常のupdateJudgePhaseとupdateJudgeResultPhaseを使用するため、これらの関数は不要

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
        } else if (enemy->isResident()) {
            // 住民との戦闘の場合、経験値・ゴールド・レベルアップをスキップ
            lastResult = BattleResult::PLAYER_VICTORY;
            addBattleLog("住民は倒れた...");
            // レベルアップをスキップして直接終了
            endBattle();
            return;
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
            // 敵の情報を渡してGameOverStateを作成（再戦用）
            stateManager->changeState(std::make_unique<GameOverState>(
                player, "戦闘に敗北しました。", enemy->getType(), enemy->getLevel()));
        } else if (enemy->isResident()) {
            // 住民との戦闘の場合、NightStateに戻り、住民を倒した処理を実行
            int residentX = enemy->getResidentX();
            int residentY = enemy->getResidentY();
            
            // 住民の位置情報をPlayerに保存（NightState::enter()で処理を実行）
            player->addKilledResident(residentX, residentY);
            
            // NightStateに戻る（enter()で住民を倒した処理を実行）
            stateManager->changeState(std::make_unique<NightState>(player));
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
        // 住民戦の場合は特別な処理
        if (enemy->isResident()) {
            int playerCmd = -1;
            if (selected == "攻撃") {
                playerCmd = BattleConstants::COMMAND_ATTACK;
            } else if (selected == "身を隠す") {
                playerCmd = BattleConstants::PLAYER_COMMAND_HIDE;
            }
            
            if (playerCmd >= 0) {
                // 住民のコマンドは事前に生成済み（initializeCommandSelectionで生成）
                // キャッシュされたコマンドを使用することで、予測表示と実際のコマンドが一致する
                int residentCmd = cachedResidentCommand;
                
                // キャッシュが無効な場合（念のため）は新しく生成
                if (residentCmd < 0) {
                    residentCmd = generateResidentCommand();
                }
                
                // 通常のJUDGEフェーズに遷移（住民戦でも同じUIを使用）
                isShowingOptions = false;
                currentPhase = BattlePhase::JUDGE;
                judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                judgeDisplayTimer = 0.0f;
                currentJudgingTurnIndex = 0;
                // 現在のコマンドを保存（processResidentTurnで使用）
                currentResidentPlayerCommand = playerCmd;
                currentResidentCommand = residentCmd;
            }
        } else {
            // 通常の戦闘
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
    
    // 初回の戦闘時のみ説明を開始（住民戦以外、静的変数で管理）
    // また、説明UIを既に見た場合は表示しない
    static bool s_firstBattle = true;
    if (s_firstBattle && !enemy->isResident() && !player->hasSeenBattleExplanation) {
        setupGameExplanation();
        showGameExplanation = true;
        explanationStep = 0;
        s_firstBattle = false;
        // explanationMessageBoardはsetupUI()で初期化されるので、render()で設定する
    } else if (player->hasSeenBattleExplanation) {
        // 既に見た場合は、静的変数も更新
        s_firstBattle = false;
    }
    
    // 住民戦の場合は住民のコマンドを事前に生成して、それに基づいて様子を表示
    // これにより、予測表示と実際のコマンドが一致する
    // ただし、50%の確率で「様子が伺えない」と表示し、どちらのコマンドを使うかわからないようにする
    if (enemy->isResident()) {
        // 実際に使われるコマンドを事前に生成
        cachedResidentCommand = generateResidentCommand();
        
        // 30%の確率で「様子が伺えない」と表示（どちらのコマンドを使うかわからない）
        std::random_device rd;
        unsigned int seed = rd() ^ static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> dis(1, 100);
        
        int behaviorRoll = dis(gen);
        if (behaviorRoll <= 50) {
            // 30%の確率で様子が伺えない
            cachedResidentBehaviorHint = "様子が伺えない";
        } else {
            // 70%の確率で、生成したコマンドに基づいて住民の様子を設定
            if (cachedResidentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID) {
                // 怯える → 震えが止まらないようだ
                cachedResidentBehaviorHint = "震えが止まらないようだ";
            } else if (cachedResidentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP) {
                // 助けを呼ぶ → 辺りを見渡している
                cachedResidentBehaviorHint = "辺りを見渡している";
            } else {
                cachedResidentBehaviorHint = "";
            }
        }
    }
}

void BattleState::selectCommandForTurn(int turnIndex) {
    currentOptions.clear();
    
    // 住民戦の場合は「攻撃」「身を隠す」のみ
    if (enemy->isResident()) {
        currentOptions.push_back("攻撃");
        currentOptions.push_back("身を隠す");
    } else {
        // 通常の戦闘：攻撃、防御、呪文
        currentOptions.push_back("攻撃");
        currentOptions.push_back("防御");
        // 新しいシステム：常に魔法を選択可能（取得済みかのチェック不要）
        currentOptions.push_back("呪文");
    }
    
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



std::pair<int, int> BattleState::calculateCurrentWinLoss() const {
    int playerWins = 0;
    int enemyWins = 0;
    
    // 住民戦の場合は、judgeResidentTurnの結果を使用
    if (enemy->isResident()) {
        // currentResidentPlayerCommandとcurrentResidentCommandが-1の場合は未設定なので、引き分けを返す
        if (currentResidentPlayerCommand == -1 || currentResidentCommand == -1) {
            return std::make_pair(0, 0);
        }
        int result = judgeResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
        if (result == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
            playerWins = 1;
        } else if (result == BattleConstants::JUDGE_RESULT_ENEMY_WIN) {
            enemyWins = 1;
        }
        return std::make_pair(playerWins, enemyWins);
    }
    
    auto playerCmds = battleLogic->getPlayerCommands();
    auto enemyCmds = battleLogic->getEnemyCommands();
    
    // ジャッジフェーズ中は、現在判定中のターンまで計算
    // 結果フェーズでは全てのターンを計算
    int maxTurn = (currentPhase == BattlePhase::JUDGE || currentPhase == BattlePhase::DESPERATE_JUDGE) 
                  ? currentJudgingTurnIndex 
                  : battleLogic->getCommandTurnCount();
    
    for (int i = 0; i < maxTurn; i++) {
        int result = battleLogic->judgeRound(playerCmds[i], enemyCmds[i]);
        if (result == BattleConstants::JUDGE_RESULT_PLAYER_WIN) {
            playerWins++;
        } else if (result == BattleConstants::JUDGE_RESULT_ENEMY_WIN) {
            enemyWins++;
        }
    }
    
    return std::make_pair(playerWins, enemyWins);
}

void BattleState::setupGameExplanation() {
    gameExplanationTexts.clear();
    std::string enemyName = enemy->getTypeName();
    gameExplanationTexts.push_back(enemyName + "との戦闘が始まりました！");
    gameExplanationTexts.push_back("戦闘は全てコマンドで行います。中央を見てください！\nこのコマンドには攻撃、防御、呪文の3種類があります。");
    gameExplanationTexts.push_back("3すくみの関係になっていて、上部の図のように攻撃は呪文に強い、\n呪文は防御に強い、防御は攻撃に強いという特徴を持っています。");
    gameExplanationTexts.push_back("そして敵には行動パターンが存在しています。\n攻撃型、防御型、呪文型の3種類で、どれをよく使うかというものです。");
    gameExplanationTexts.push_back("つまり攻撃型の敵に対しては\n防御コマンドを使うと効果的だということですね！");
    gameExplanationTexts.push_back("では相手の型はどこに書いているかというと・・・");
    gameExplanationTexts.push_back("実は戦闘開始時は書いていないんですね！");
    gameExplanationTexts.push_back("その代わり、敵の名前の下に〜型ではないようだと書いてますね！\nこれで敵の型が2種類に絞れるというわけです。");
    gameExplanationTexts.push_back("実際の相手の行動を見て、どちらの型なのかを見極めてください！");
    gameExplanationTexts.push_back("でも型を1つに絞って表示してくれないのは少し意地悪ですね。\n実は自分の体力が7割を切った際に相手の型が表示されます。");
    gameExplanationTexts.push_back("それまでは予測して戦ってみてくださいね。");
    gameExplanationTexts.push_back("最後に左上のターンについて説明します。");
    gameExplanationTexts.push_back("本作の戦闘は最初に3ターン分の行動を選択するようになっています。\nそして勝った回数が多い方が一方的にそのターン数だけ攻撃できます！");
    gameExplanationTexts.push_back("なので勝ちまくれば無傷で戦闘を終了させることもできるんですね。\nでも逆に一発逆転されるってことも・・・？賭けみたいですね！");
    gameExplanationTexts.push_back("長くなってしまいましたが一旦実際に戦ってみましょう！");
}

void BattleState::showExplanationMessage(const std::string& message) {
    if (explanationMessageBoard) {
        explanationMessageBoard->setText(message);
    }
}

void BattleState::clearExplanationMessage() {
    if (explanationMessageBoard) {
        explanationMessageBoard->setText("");
    }
}

void BattleState::renderRockPaperScissorsImage(Graphics& graphics) {
    // 住民戦ではない場合のみ表示
    if (enemy->isResident()) {
        return;
    }
    
    SDL_Texture* rpsTexture = graphics.getTexture("rock_paper_scissors");
    if (rpsTexture) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        
        // 画像のサイズを取得
        int textureWidth, textureHeight;
        SDL_QueryTexture(rpsTexture, nullptr, nullptr, &textureWidth, &textureHeight);
        
        // 中央上部に配置（画像の幅を適切なサイズに調整）
        int displayWidth = 200; // 表示幅を200pxに設定（必要に応じて調整可能）
        int displayHeight = static_cast<int>(textureHeight * (static_cast<float>(displayWidth) / textureWidth));
        int posX = (screenWidth - displayWidth) / 2; // 中央
        int posY = 20; // 上部から20px下
        
        graphics.drawTexture(rpsTexture, posX, posY, displayWidth, displayHeight);
    }
}

void BattleState::renderWinLossUI(Graphics& graphics, bool isResultPhase) {
    auto winLoss = calculateCurrentWinLoss();
    int playerWins = winLoss.first;
    int enemyWins = winLoss.second;
    
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
    // 自分と敵の真ん中（x座標はVSがあるところ = screenWidth / 2）
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // VSの位置を計算
    // VSの最大スケール時の高さを計算（JUDGE_VS_BASE_SCALE = 3.0f）
    // VSテキストの高さを仮定（約30px * 3.0 = 90px）
    std::string vsText = "VS";
    SDL_Color vsTextColor = {255, 255, 255, 255};
    SDL_Texture* vsTexture = graphics.createTextTexture(vsText, "default", vsTextColor);
    int vsTextWidth = 0, vsTextHeight = 0;
    if (vsTexture) {
        SDL_QueryTexture(vsTexture, nullptr, nullptr, &vsTextWidth, &vsTextHeight);
        SDL_DestroyTexture(vsTexture);
    }
    int vsScaledHeight = static_cast<int>(vsTextHeight * BattleConstants::JUDGE_VS_BASE_SCALE);
    int vsPadding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_LARGE;
    
    // VSの背景の下端 = centerY + JUDGE_COMMAND_Y_OFFSET + (scaledHeight / 2) + padding
    int vsBottomY = centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET + (vsScaledHeight / 2) + vsPadding;
    
    // 勝敗UIはVSの真下に配置
    int winLossY = vsBottomY + 20; // VSの下に適切な間隔を空ける
    
    // 勝敗テキスト
    std::string winLossText = "自分 " + std::to_string(playerWins) + "勝  " + 
                              "敵 " + std::to_string(enemyWins) + "勝";
    SDL_Color textColor = {255, 255, 255, 255}; // 白
    
    SDL_Texture* textTexture = graphics.createTextTexture(winLossText, "default", textColor);
    if (textTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
        
        int padding = 8;
        int bgX = centerX - textWidth / 2 - padding;
        int bgY = winLossY - padding;
        
        // 背景黒
        graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
        
        // テキスト白
        graphics.drawText(winLossText, centerX - textWidth / 2, winLossY, "default", textColor);
        
        SDL_DestroyTexture(textTexture);
        
        // 結果フェーズのみ、ターン数表示と現在実行中のターンに応じたメッセージを表示
        if (isResultPhase) {
            // 1. 「〜ターン分の攻撃を実行」を常に表示（勝敗UIの下）
            std::string totalAttackText;
            // 住民戦で攻撃失敗時は「ためらいました」を表示
            if (enemy->isResident() && residentAttackFailed) {
                totalAttackText = player->getName() + "はメンタルの影響で攻撃をためらいました。";
            } else if (playerWins > enemyWins) {
                totalAttackText = player->getName() + "が" + std::to_string(playerWins) + "ターン分の攻撃を実行！";
            } else if (enemyWins > playerWins) {
                totalAttackText = "敵が" + std::to_string(enemyWins) + "ターン分の攻撃を実行！";
            } else {
                totalAttackText = "両方がダメージを受ける";
            }
            
            SDL_Texture* totalAttackTexture = graphics.createTextTexture(totalAttackText, "default", textColor);
            if (totalAttackTexture) {
                int totalAttackTextWidth, totalAttackTextHeight;
                SDL_QueryTexture(totalAttackTexture, nullptr, nullptr, &totalAttackTextWidth, &totalAttackTextHeight);
                
                // 勝敗UIの下に配置
                int totalAttackY = winLossY + textHeight + 20; // 勝敗UIの下に適切な間隔を空ける
                int totalAttackBgX = centerX - totalAttackTextWidth / 2 - padding;
                int totalAttackBgY = totalAttackY - padding;
                
                // 背景黒
                graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                graphics.drawRect(totalAttackBgX, totalAttackBgY, totalAttackTextWidth + padding * 2, totalAttackTextHeight + padding * 2, true);
                
                // テキスト白
                graphics.drawText(totalAttackText, centerX - totalAttackTextWidth / 2, totalAttackY, "default", textColor);
                
                SDL_DestroyTexture(totalAttackTexture);
                
                // 2. 現在実行中のターンに応じたメッセージを表示（「〜ターン分の攻撃を実行」の下）
                if (playerWins > enemyWins) {
                    std::string attackText;
                    
                    // 現在実行中のターンに対応するメッセージを生成
                    if (currentExecutingTurn < pendingDamages.size()) {
                        const auto& damageInfo = pendingDamages[currentExecutingTurn];
                        
                        if (damageInfo.commandType == BattleConstants::COMMAND_ATTACK) {
                            attackText = player->getName() + "のアタック！";
                        } else if (damageInfo.commandType == BattleConstants::COMMAND_DEFEND) {
                            attackText = player->getName() + "のラッシュアタック！";
                        } else if (damageInfo.commandType == BattleConstants::COMMAND_SPELL) {
                            // 呪文の種類を取得
                            auto it = executedSpellsByDamageIndex.find(currentExecutingTurn);
                            if (it != executedSpellsByDamageIndex.end()) {
                                SpellType spellType = it->second;
                                switch (spellType) {
                                    case SpellType::STATUS_UP:
                                        attackText = player->getName() + "がステータスアップ呪文発動！";
                                        break;
                                    case SpellType::HEAL:
                                        attackText = player->getName() + "が回復呪文発動！";
                                        break;
                                    case SpellType::ATTACK:
                                        attackText = player->getName() + "が攻撃呪文発動！";
                                        break;
                                    default:
                                        attackText = player->getName() + "が呪文発動！";
                                        break;
                                }
        } else {
                                attackText = player->getName() + "が呪文発動！";
        }
    } else {
                            // デフォルトメッセージ
                            attackText = player->getName() + "の攻撃！";
                        }
                    }
                    
                    if (!attackText.empty()) {
                        SDL_Texture* attackTexture = graphics.createTextTexture(attackText, "default", textColor);
                        if (attackTexture) {
                            int attackTextWidth, attackTextHeight;
                            SDL_QueryTexture(attackTexture, nullptr, nullptr, &attackTextWidth, &attackTextHeight);
                            
                            // 「〜ターン分の攻撃を実行」の下に配置
                            int attackY = totalAttackY + totalAttackTextHeight + 20; // 上記UIの下に適切な間隔を空ける
                            int attackBgX = centerX - attackTextWidth / 2 - padding;
                            int attackBgY = attackY - padding;
                            
                            // 背景黒
                            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                            graphics.drawRect(attackBgX, attackBgY, attackTextWidth + padding * 2, attackTextHeight + padding * 2, true);
                            
                            // テキスト白
                            graphics.drawText(attackText, centerX - attackTextWidth / 2, attackY, "default", textColor);
                            
                            SDL_DestroyTexture(attackTexture);
                        }
                    }
                }
            }
        }
    }
}

void BattleState::judgeBattle() {
    prepareJudgeResults();
}

void BattleState::prepareDamageList(float damageMultiplier) {
    // 呪文で勝利したターンを記録
    spellWinTurns.clear();
    executedSpellsByDamageIndex.clear();  // 呪文の記録をクリア
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
                    // 住民戦の場合は1ターンずつなので、結果フェーズに遷移
                    // ただし、引き分け（身を隠す+怯える）または身を隠す+助けを呼ぶの場合は結果フェーズをスキップ
                    if (enemy->isResident()) {
                        // 引き分けかどうかを判定
                        int judgeResult = judgeResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
                        bool isDraw = (judgeResult == BattleConstants::JUDGE_RESULT_DRAW);
                        // 身を隠す + 助けを呼ぶの場合も結果フェーズをスキップ（判定では勝利だが、実際は引き分け処理）
                        bool isHideAndCallHelp = (currentResidentPlayerCommand == BattleConstants::PLAYER_COMMAND_HIDE &&
                                                   currentResidentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP);
                        
                        if (isDraw || isHideAndCallHelp) {
                            // 引き分けまたは身を隠す+助けを呼ぶの場合は結果フェーズをスキップして直接コマンド選択に遷移
                            processResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
                            // processResidentTurn内で既にコマンド選択フェーズに遷移しているので、ここでは何もしない
                            judgeDisplayTimer = 0.0f;
                            phaseTimer = 0.0f;
                        } else {
                            // それ以外の場合は結果フェーズに遷移
                        currentPhase = BattlePhase::JUDGE_RESULT;
                        judgeDisplayTimer = 0.0f;
                        phaseTimer = 0.0f;  // 住民戦の場合もphaseTimerをリセット
                        }
                    } else {
                        currentJudgingTurnIndex++;
                        judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                        judgeDisplayTimer = 0.0f;
                        
                        if (currentJudgingTurnIndex >= battleLogic->getCommandTurnCount()) {
                            currentPhase = isDesperateMode ? BattlePhase::DESPERATE_JUDGE_RESULT : BattlePhase::JUDGE_RESULT;
                            phaseTimer = 0;
                        }
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
    
    // 住民戦の場合は特別な処理（最初の1回だけprocessResidentTurnを呼ぶ）
    if (enemy->isResident()) {
        // 攻撃失敗時は既にJUDGE_RESULTフェーズに遷移しているので、2秒間表示してから次のターンに進む
        if (residentAttackFailed && pendingDamages.empty()) {
            phaseTimer += deltaTime;
            // 2秒経過したら次のターンに進む
            if (phaseTimer >= 2.0f) {
                residentAttackFailed = false; // フラグをリセット
                currentPhase = BattlePhase::COMMAND_SELECT;
                battleLogic->setCommandTurnCount(1);
                initializeCommandSelection();
                phaseTimer = 0.0f;
                return;
            } else {
                return;  // 2秒経過するまではreturn
            }
        }
        
        if (pendingDamages.empty() && phaseTimer < 2.0f && !residentAttackFailed) {
            phaseTimer += deltaTime;
            // 2秒後に実際の処理に移行
            if (phaseTimer >= 2.0f) {
                processResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
                // processResidentTurnでpendingDamagesに追加された後、すぐにアニメーション処理に進む
                // phaseTimerをリセットして、アニメーション処理を確実に開始する
                phaseTimer = 0.0f;
                // returnしない（通常の戦闘と同じ処理に進む）
            } else {
                return;  // 2秒経過するまではreturn
            }
        }
        // pendingDamagesが追加された後は通常の戦闘と同じ処理
    }
    
    auto stats = battleLogic->getStats();
    bool isVictory = (stats.playerWins > stats.enemyWins);
    bool isDefeat = (stats.enemyWins > stats.playerWins);
    
    // 住民戦の場合は、pendingDamagesが空でない場合のみ勝利とみなす
    if (enemy->isResident()) {
        isVictory = !pendingDamages.empty();
        isDefeat = false;
    }
    
    // ダメージリストの準備（初回のみ）
    // 住民戦の場合はprocessResidentTurnでpendingDamagesに追加するので、prepareDamageListは呼ばない
    if (pendingDamages.empty() && !enemy->isResident()) {
        float multiplier = isDesperateMode ? 1.5f : (stats.hasThreeWinStreak ? BattleConstants::THREE_WIN_STREAK_MULTIPLIER : 1.0f);
        prepareDamageList(multiplier);
        damageAppliedInAnimation = false;
        currentExecutingTurn = 0;
        executeDelayTimer = 0.0f;
        
        // 呪文で勝利したターンがある場合、自動で呪文を選択して処理（住民戦では不要）
        while (!spellWinTurns.empty() && !waitingForSpellSelection && !enemy->isResident()) {
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
                    int damageIndex = pendingDamages.size();
                    pendingDamages.push_back(spellDamage);
                    // 攻撃呪文を記録
                    executedSpellsByDamageIndex[damageIndex] = SpellType::ATTACK;
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
                int damageIndex = pendingDamages.size();
                pendingDamages.push_back(spellDamage);
                // 呪文の種類を記録
                executedSpellsByDamageIndex[damageIndex] = selectedSpell;
                
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
        // 住民戦の場合は不要（processResidentTurnでpendingDamagesに追加される）
        if (pendingDamages.empty() && spellWinTurns.empty() && isVictory && !enemy->isResident()) {
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
    } else if (enemy->isResident() && !pendingDamages.empty()) {
        // 住民戦の場合、processResidentTurnでpendingDamagesに追加された後、
        // すぐにアニメーション処理に進む（通常の戦闘のprepareDamageList後の処理は不要）
        // processResidentTurnで既にresetResultAnimation()を呼んでいるが、
        // 念のため初期化状態を確認して、必要に応じてリセットする
        if (currentExecutingTurn == 0 && !damageAppliedInAnimation) {
            // 既にprocessResidentTurnでresetResultAnimation()を呼んでいるので、
            // ここでは初期化のみ行う（重複してresetResultAnimation()を呼ばない）
            damageAppliedInAnimation = false;
            currentExecutingTurn = 0;
            executeDelayTimer = 0.0f;
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
        bool hasThreeWinStreak = enemy->isResident() ? false : stats.hasThreeWinStreak;
        animationController->updateResultCharacterAnimation(
            deltaTime, isVictory, isDefeat, resultState.resultAnimationTimer,
            damageAppliedInAnimation, hasThreeWinStreak,
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
        if (isVictory) {
            float intensity = isDesperateMode ? BattleConstants::DESPERATE_VICTORY_SHAKE_INTENSITY : BattleConstants::VICTORY_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, true, false);
        } else if (isDefeat) {
            float intensity = isDesperateMode ? BattleConstants::DESPERATE_DEFEAT_SHAKE_INTENSITY : BattleConstants::DEFEAT_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, false, true);
        } else {
            float intensity = isDesperateMode ? BattleConstants::DESPERATE_DRAW_SHAKE_INTENSITY : BattleConstants::DRAW_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
        }
    }
    
    // 全ダメージ適用完了後、戦闘終了判定
    if (currentExecutingTurn >= pendingDamages.size()) {
        // 住民戦の場合は短い待機時間、通常の戦闘は長い待機時間
        float finalWaitDuration;
        float maxWaitTime;
        if (enemy->isResident()) {
            // 住民戦：アニメーション完了後、1秒待機
            finalWaitDuration = 0.5f + 1.0f; // 1.5秒
            maxWaitTime = 2.0f;
        } else {
            // 通常の戦闘：アニメーション完了後、さらに1.5秒待機（合計3秒）
            constexpr float animTotalDuration = 0.5f + 1.0f; // 1.5秒
            finalWaitDuration = animTotalDuration * 2.0f; // 3.0秒
            maxWaitTime = 4.0f;
        }
        
        if (resultState.resultAnimationTimer >= finalWaitDuration || phaseTimer >= maxWaitTime) {
            updateStatus();
            
            if (enemy->getIsAlive() && player->getIsAlive()) {
                if (enemy->isResident()) {
                    // 住民戦の場合は次のターンへ
                    residentTurnCount++;
                    // 10ターン経過したらゲームオーバー
                    if (residentTurnCount > 10) {
                        if (stateManager) {
                            stateManager->changeState(std::make_unique<GameOverState>(
                                player, "10ターン以内に倒せませんでした。衛兵に見つかりました。", enemy->getType(), enemy->getLevel(),
                                true, enemy->getName(), enemy->getResidentX(), enemy->getResidentY(), enemy->getResidentTextureIndex()));
                        }
                        return;
                    }
                    currentPhase = BattlePhase::COMMAND_SELECT;
                    battleLogic->setCommandTurnCount(1);
                    initializeCommandSelection();
                } else {
                    if (isDesperateMode) {
                        battleLogic->setDesperateMode(false);
                        battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                    }
                    currentPhase = BattlePhase::COMMAND_SELECT;
                    initializeCommandSelection();
                }
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
            if (enemy->isResident()) {
                // 住民戦の場合は次のターンへ
                residentTurnCount++;
                // 10ターン経過したらゲームオーバー
                if (residentTurnCount > 10) {
                    if (stateManager) {
                        stateManager->changeState(std::make_unique<GameOverState>(
                            player, "10ターン以内に倒せませんでした。衛兵に見つかりました。", enemy->getType(), enemy->getLevel(),
                            true, enemy->getName(), enemy->getResidentX(), enemy->getResidentY(), enemy->getResidentTextureIndex()));
                    }
                    return;
                }
                currentPhase = BattlePhase::COMMAND_SELECT;
                battleLogic->setCommandTurnCount(1);
                initializeCommandSelection();
            } else {
                if (isDesperateMode) {
                    battleLogic->setDesperateMode(false);
                    battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                }
                currentPhase = BattlePhase::COMMAND_SELECT;
                initializeCommandSelection();
            }
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
