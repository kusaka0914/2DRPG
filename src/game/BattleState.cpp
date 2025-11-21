#include "BattleState.h"
#include "CastleState.h"
#include "NightState.h"
#include "MainMenuState.h"
#include "TownState.h"
#include "FieldState.h"
#include "GameOverState.h"
#include "EndingState.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include "../core/AudioManager.h"
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
      isTargetLevelEnemy(false),
      currentSelectingTurn(0), isFirstCommandSelection(true), hasUsedLastChanceMode(false),
      damageListPrepared(false), enemyAttackStarted(false), enemyAttackTimer(0.0f),
      battleMusicStarted(false), lastChanceIntroMusicStopped(false), adversityMusicStarted(false),
      currentJudgingTurn(0), currentExecutingTurn(0), executeDelayTimer(0.0f),
      damageAppliedInAnimation(false), waitingForSpellSelection(false),
      judgeSubPhase(JudgeSubPhase::SHOW_PLAYER_COMMAND), judgeDisplayTimer(0.0f), currentJudgingTurnIndex(0),
      introScale(0.0f), introTimer(0.0f), lastIntroTimer(-1.0f),
      currentResidentPlayerCommand(-1), currentResidentCommand(-1),
      cachedResidentBehaviorHint(""), cachedResidentCommand(-1),
      residentTurnCount(0),
      residentAttackFailed(false),
      residentHitCount(0),
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
    
    // 目標レベル達成用の敵かどうかを確認（敵のレベルが目標レベルと一致する場合）
    if (this->enemy->getLevel() == TownState::s_targetLevel) {
        // 敵のタイプが目標レベルに到達できる敵タイプであるかを確認
        // createTargetLevelEnemyで生成された敵のレベルが目標レベルと一致する場合、isTargetLevelEnemyをtrueにする
        int targetLevel = TownState::s_targetLevel;
        Enemy testEnemy = Enemy::createTargetLevelEnemy(targetLevel);
        if (testEnemy.getType() == this->enemy->getType() && testEnemy.getLevel() == this->enemy->getLevel()) {
            isTargetLevelEnemy = true;
        }
    }
}

void BattleState::enter() {
    // 特殊技効果をリセット
    player->getPlayerStats().resetEnemySkillEffects();
    loadBattleImages();
    
    // INTROフェーズに入った時にフィールドBGMを停止（音楽はCOMMAND_SELECTフェーズで開始）
    AudioManager::getInstance().stopMusic();
    // 再戦時に音楽が最初から再生されるようにリセット
    battleMusicStarted = false;
    
    std::string enemyAppearMessage = enemy->getTypeName() + "が現れた！";
    battleLog = enemyAppearMessage; // battleLogLabelがまだ初期化されていないので、直接battleLogに保存
    
    currentPhase = BattlePhase::INTRO;
    phaseTimer = 0;
    introScale = 0.0f;
    introTextScale = 0.0f;
    introTimer = 0.0f;
    lastIntroTimer = -1.0f;  // intro.ogg再生判定用にリセット
    updateStatus();
}

void BattleState::exit() {
    ui.clear();
    // BattleUIをクリーンアップ（次の戦闘で再作成される）
    battleUI.reset();
    battleLogLabel = nullptr;
    playerStatusLabel = nullptr;
    enemyStatusLabel = nullptr;
    explanationMessageBoard = nullptr;
}

void BattleState::update(float deltaTime) {
    ui.update(deltaTime);
    
    effectManager->updateScreenShake(deltaTime);
    effectManager->updateHitEffects(deltaTime);
    if (currentPhase == BattlePhase::COMMAND_SELECT || 
        currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT ||
        currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT) {
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
                // 目標レベル未達成の場合、目標レベルの敵と戦闘を開始
                if (stateManager) {
                    int targetLevel = TownState::s_targetLevel;
                    auto targetLevelEnemy = std::make_unique<Enemy>(Enemy::createTargetLevelEnemy(targetLevel));
                    isTargetLevelEnemy = true;
                    // 現在の戦闘を終了して、目標レベルの敵との戦闘を開始
                    // ただし、既に戦闘中の場合（VICTORY_DISPLAYやLEVEL_UP_DISPLAYフェーズなど）は、そのまま続行
                    // 通常の戦闘中の場合のみ、新しい敵と戦闘を開始
                    if (currentPhase == BattlePhase::VICTORY_DISPLAY || 
                        currentPhase == BattlePhase::LEVEL_UP_DISPLAY ||
                        currentPhase == BattlePhase::RESULT) {
                        // 既に戦闘終了フェーズの場合は、そのまま続行（endBattle()で処理される）
                    } else {
                        // 戦闘中の場合、新しい敵と戦闘を開始
                        this->enemy = std::move(targetLevelEnemy);
                        // 戦闘をリセット
                        battle = std::make_unique<Battle>(player.get(), this->enemy.get());
                        battleLogic = std::make_unique<BattleLogic>(player, this->enemy.get());
                        animationController = std::make_unique<BattleAnimationController>();
                        effectManager = std::make_unique<BattleEffectManager>();
                        phaseManager = std::make_unique<BattlePhaseManager>(battleLogic.get(), player.get(), this->enemy.get());
                        currentPhase = BattlePhase::INTRO;
                        phaseTimer = 0;
                        introScale = 0.0f;
                        introTextScale = 0.0f;
                        introTimer = 0.0f;
                        // INTROフェーズに入った瞬間にintro.oggを再生
                        AudioManager::getInstance().playSound("intro", 0);
                        oldLevel = player->getLevel();
                        oldMaxHp = player->getMaxHp();
                        oldMaxMp = player->getMaxMp();
                        oldAttack = player->getAttack();
                        oldDefense = player->getDefense();
                        addBattleLog("目標レベル" + std::to_string(targetLevel) + "の敵が現れた！");
                    }
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
                        // コマンド選択フェーズに入った時にbattle.oggを再生（初回のみ）
                        if (!battleMusicStarted) {
                            AudioManager::getInstance().playMusic("battle", -1);
                            battleMusicStarted = true;
                        }
                    }
                    break;
                case BattlePhase::JUDGE:
                case BattlePhase::DESPERATE_JUDGE:
                case BattlePhase::LAST_CHANCE_JUDGE:
                    // フェーズ遷移時に必ず敵コマンドを生成する（確実に生成されるように）
                    battleLogic->generateEnemyCommands();
                    prepareJudgeResults();
                    currentJudgingTurn = 0;
                    currentJudgingTurnIndex = 0;
                    judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                    judgeDisplayTimer = 0.0f;
                    // 最初のコマンド表示時にcommand.oggを再生
                    AudioManager::getInstance().playSound("command", 0);
                    break;
                default:
                    break;
            }
        }
    }
    
    switch (currentPhase) {
        case BattlePhase::INTRO: {
            // INTROフェーズに入った瞬間（introTimerが0の時）にintro.oggを再生
            if (introTimer == 0.0f && lastIntroTimer < 0.0f) {
                AudioManager::getInstance().playSound("intro", 0);
                lastIntroTimer = 0.0f;
            }
            
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
                    residentHitCount = 0;  // 住民戦の攻撃成功回数を初期化
                } else {
                    battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                }
                battleLogic->setDesperateMode(false);
                isFirstCommandSelection = true;
                initializeCommandSelection();
                // コマンド選択フェーズに入った時にbattle.oggを再生（初回のみ）
                if (!battleMusicStarted) {
                    AudioManager::getInstance().playMusic("battle", -1);
                    battleMusicStarted = true;
                }
            }
            break;
        }
            
        case BattlePhase::COMMAND_SELECT:
            // 住民戦の場合は、コマンド選択UIを表示する必要がある
            if (enemy->isResident()) {
                // 住民戦の場合、初回のみコマンド選択UIを表示（handleInputでも呼ばれるが、念のため）
                // HPがMAXでない場合でも確実に表示されるように、update()でも呼ぶ
                if (currentSelectingTurn < battleLogic->getCommandTurnCount() && !isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
                break;
            }
            
            if (isFirstCommandSelection && checkDesperateModeCondition()) {
                isFirstCommandSelection = false;
                currentPhase = BattlePhase::DESPERATE_MODE_PROMPT;
                showDesperateModePrompt();
                break;
            }
            isFirstCommandSelection = false;
            
            // 初回のみコマンド選択UIを表示（handleInputでも呼ばれるが、念のため）
            // HPがMAXでない場合でも確実に表示されるように、update()でも呼ぶ
            if (currentSelectingTurn < battleLogic->getCommandTurnCount() && !isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
            
            // 全てのコマンドが選択された場合は敵コマンドを生成
            if (currentSelectingTurn >= battleLogic->getCommandTurnCount()) {
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
            // 初回のみコマンド選択UIを表示（handleInputでも呼ばれるが、念のため）
            if (currentSelectingTurn < battleLogic->getCommandTurnCount() && !isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
            
            // 全てのコマンドが選択された場合は敵コマンドを生成
            if (currentSelectingTurn >= battleLogic->getCommandTurnCount()) {
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
        
        case BattlePhase::LAST_CHANCE_COMMAND_SELECT:
            // コマンド選択に入ったら adversity.ogg を再生
            if (!adversityMusicStarted) {
                AudioManager::getInstance().playMusic("adversity", -1);
                adversityMusicStarted = true;
            }
            
            // 通常戦のCOMMAND_SELECTと同じ処理
            if (currentSelectingTurn < battleLogic->getCommandTurnCount() && !isShowingOptions) {
                selectCommandForTurn(currentSelectingTurn);
            }
            
            // 全てのコマンドが選択された場合は敵コマンドを生成
            // フェーズ遷移はBattlePhaseManagerが処理する（COMMAND_SELECTと同じ）
            if (currentSelectingTurn >= battleLogic->getCommandTurnCount()) {
                battleLogic->generateEnemyCommands();
            }
            break;
            
        case BattlePhase::LAST_CHANCE_JUDGE:
            updateJudgePhase(deltaTime, false); // 通常戦と同じ（isDesperateMode=false）
            break;
            
        case BattlePhase::LAST_CHANCE_INTRO: {
            // イントロに入ったら無音にする
            if (!lastChanceIntroMusicStopped) {
                AudioManager::getInstance().stopMusic();
                lastChanceIntroMusicStopped = true;
            }
            
            // タイマーで遷移
            phaseTimer += deltaTime;
            
            // 2秒後にコマンド選択フェーズに遷移
            if (phaseTimer >= 3.0f) {
                currentPhase = BattlePhase::LAST_CHANCE_COMMAND_SELECT;
                battleLogic->setCommandTurnCount(BattleConstants::LAST_CHANCE_TURN_COUNT);
                initializeCommandSelection();
                phaseTimer = 0.0f;
                addBattleLog("最後のチャンス！5ターンで勝負だ！");
                
                // 初回の場合、説明UIを設定
                if (!player->hasSeenLastChanceExplanation) {
                    setupLastChanceExplanation();
                    showGameExplanation = true;
                    explanationStep = 0;
                    // explanationMessageBoardはsetupUI()で初期化されるので、render()で設定する
                }
            }
            break;
        }
            
        case BattlePhase::LAST_CHANCE_JUDGE_RESULT: {
            // フェーズが既に変更されている場合は処理をスキップ
            if (currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
                break;
            }
            updateLastChanceJudgeResultPhase(deltaTime);
            // フェーズが変更された場合は処理を終了
            if (currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
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
    // ホットリロード対応
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    bool uiJustInitialized = false;
    // フォントが読み込まれている場合のみUIをセットアップ
    if (graphics.getFont("default")) {
        if (!battleLogLabel || (!lastReloadState && currentReloadState)) {
            setupUI(graphics);
            uiJustInitialized = true;
        }
    } else {
        // フォントが読み込まれていない場合は警告を出す（初回のみ）
        static bool fontWarningShown = false;
        if (!fontWarningShown) {
            std::cerr << "警告: フォントが読み込まれていません。UIの表示をスキップします。" << std::endl;
            fontWarningShown = true;
        }
    }
    lastReloadState = currentReloadState;
    
    // フォントが読み込まれている場合のみBattleUIを作成
    if (!battleUI && graphics.getFont("default")) {
        battleUI = std::make_unique<BattleUI>(&graphics, player, enemy.get(), battleLogic.get(), animationController.get());
    }
    
    if (uiJustInitialized && !battleLog.empty() && battleLogLabel) {
        battleLogLabel->setText(battleLog);
    }
    
    // 初回の戦闘時のみ説明を開始（UI初期化後、住民戦以外）
    // ただし、COMMAND_SELECTフェーズで設定するので、ここでは設定しない
    
    // JUDGE_RESULTフェーズではrenderResultAnnouncement()内で背景画像を描画するため、ここでは処理しない
    if (currentPhase == BattlePhase::JUDGE_RESULT || 
        currentPhase == BattlePhase::DESPERATE_JUDGE_RESULT ||
        currentPhase == BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
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
    
    // LAST_CHANCE_INTROフェーズの特別な描画
    if (currentPhase == BattlePhase::LAST_CHANCE_INTRO) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();

        // 背景画像を描画
        SDL_Texture* bgTexture = getBattleBackgroundTexture(graphics);
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }

        // 「終焉突破」テキストを画面中央に表示
        std::string text = "終焉解放";
        SDL_Color textColor = {255, 0, 0, 255}; // 赤色

        // テキストをテクスチャとして取得
        SDL_Texture* textTexture = graphics.createTextTexture(text, "default", textColor);
        if (textTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);

            // 2倍サイズに拡大
            constexpr float scale = 2.0f;
            int scaledWidth = static_cast<int>(textWidth * scale);
            int scaledHeight = static_cast<int>(textHeight * scale);

            // 画面中央に配置
            int textX = (screenWidth - scaledWidth) / 2;
            int textY = (screenHeight - scaledHeight) / 2;

            // 背景を描画（パディング付き、スケールに応じて調整）
            constexpr int padding = 20;
            int bgX = textX - padding;
            int bgY = textY - padding;
            int bgWidth = scaledWidth + padding * 2;
            int bgHeight = scaledHeight + padding * 2;
            
            graphics.setDrawColor(0, 0, 0, 200);
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics.setDrawColor(255, 0, 0, 255);
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, false);
            
            // テキストを描画
            graphics.drawTexture(textTexture, textX, textY, scaledWidth, scaledHeight);
            
            SDL_DestroyTexture(textTexture);
        }
        
        graphics.present();
        return;
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
        
        // JSONから敵の位置を取得（INTROフェーズでは中央に配置）
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto battleConfig = config.getBattleConfig();
        
        int enemyX, enemyY;
        config.calculatePosition(enemyX, enemyY, battleConfig.enemyPosition, screenWidth, screenHeight);
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
        
        // 夜のタイマーUIを表示
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    if (currentPhase == BattlePhase::JUDGE || currentPhase == BattlePhase::DESPERATE_JUDGE || currentPhase == BattlePhase::LAST_CHANCE_JUDGE) {
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
            params.residentHitCount = residentHitCount;  // 住民戦で攻撃が成功した回数
        } else {
            params.playerCommandName = "";
            params.enemyCommandName = "";
            params.judgeResult = -999; // 未設定（battleLogicから取得）
            params.residentBehaviorHint = "";
            params.residentTurnCount = 0;  // 通常戦の場合は0
            params.residentHitCount = 0;  // 通常戦の場合は0
        }
        
        battleUI->renderJudgeAnimation(params);
        
        // 勝敗UIを表示（ジャッジフェーズ中は常に更新）
        renderWinLossUI(graphics, false);
        
        // 夜のタイマーUIを表示
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        }
        
        // Rock-Paper-Scissors画像を表示（住民戦以外）
        renderRockPaperScissorsImage(graphics);
        
        graphics.present();
        return;
    }
    
    // 住民戦でも通常のJUDGEとJUDGE_RESULTフェーズを使用するため、特別な描画処理は不要
    
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT ||
         currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT) && 
        isShowingOptions) {
        // 背景画像とキャラクターを描画（説明UIが表示される前に正常な画面を表示）
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        
        // 画面をクリア
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.clear();
        
        // 背景画像を描画
        SDL_Texture* bgTexture = getBattleBackgroundTexture(graphics);
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        }
        
        // プレイヤーと敵のキャラクター描画
        auto& charState = animationController->getCharacterState();
        auto& uiConfigManager = UIConfig::UIConfigManager::getInstance();
        auto battleConfig = uiConfigManager.getBattleConfig();
        
        int playerBaseX, playerBaseY;
        uiConfigManager.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        // 窮地モードではplayer_adversity.pngを使用
        SDL_Texture* playerTex = hasUsedLastChanceMode ? graphics.getTexture("player_adversity") : graphics.getTexture("player");
        if (!playerTex && hasUsedLastChanceMode) {
            playerTex = graphics.getTexture("player"); // フォールバック
        }
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
        
        int enemyBaseX, enemyBaseY;
        uiConfigManager.calculatePosition(enemyBaseX, enemyBaseY, battleConfig.enemyPosition, screenWidth, screenHeight);
        int enemyX = enemyBaseX + (int)charState.enemyAttackOffsetX + (int)charState.enemyHitOffsetX;
        int enemyY = enemyBaseY + (int)charState.enemyAttackOffsetY + (int)charState.enemyHitOffsetY;
        
        SDL_Texture* enemyTexture = nullptr;
        if (enemy->isResident()) {
            int textureIndex = enemy->getResidentTextureIndex();
            std::string textureName = "resident_" + std::to_string(textureIndex + 1);
            enemyTexture = graphics.getTexture(textureName);
        } else {
            enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
        }
        
        int enemyHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
        if (enemyTexture) {
            int textureWidth, textureHeight;
            if (SDL_QueryTexture(enemyTexture, nullptr, nullptr, &textureWidth, &textureHeight) == 0) {
                if (textureWidth > 0 && textureHeight > 0) {
                    float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
                    if (textureWidth <= textureHeight) {
                        enemyHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
                    } else {
                        enemyHeight = static_cast<int>(BattleConstants::BATTLE_CHARACTER_SIZE / aspectRatio);
                    }
                }
            }
        }
        
        // HP表示
        if (battleUI) {
            std::string enemyName = enemy->isResident() ? enemy->getName() : enemy->getTypeName();
            std::string residentBehaviorHint = enemy->isResident() ? getResidentBehaviorHint() : "";
            int hitCount = enemy->isResident() ? residentHitCount : 0;
            try {
                battleUI->renderHP(playerX, playerY, enemyX, enemyY, playerHeight, enemyHeight, residentBehaviorHint, false, hitCount);
            } catch (const std::exception& e) {
                std::cerr << "renderHPエラー: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "renderHPエラー: 不明なエラー" << std::endl;
            }
        }
        
        // キャラクター描画
        if (playerTex) {
            graphics.drawTextureAspectRatio(playerTex, playerX, playerY, BattleConstants::BATTLE_CHARACTER_SIZE);
        } else {
            graphics.setDrawColor(100, 200, 255, 255);
            graphics.drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, false);
        }
        
        if (enemyTexture) {
            graphics.drawTextureAspectRatio(enemyTexture, enemyX, enemyY, BattleConstants::BATTLE_CHARACTER_SIZE);
        } else {
            graphics.setDrawColor(255, 100, 100, 255);
            graphics.drawRect(enemyX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, enemyY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
            graphics.setDrawColor(255, 255, 255, 255);
            graphics.drawRect(enemyX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, enemyY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, false);
        }
        
        // 説明が設定されていない場合は設定する（初回の戦闘時のみ）
        if (showGameExplanation && explanationMessageBoard && explanationMessageBoard->getText().empty()) {
            if (!gameExplanationTexts.empty() && explanationStep == 0) {
                showExplanationMessage(gameExplanationTexts[0]);
            }
        }
        
        // battleUIとbattleLogicが有効な場合のみ描画
        if (!battleUI || !battleLogic) {
            graphics.present();
            return;
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
            params.residentHitCount = residentHitCount;  // 住民戦で攻撃が成功した回数
        } else {
            params.residentBehaviorHint = "";
            params.residentTurnCount = 0;  // 通常戦の場合は0
            params.residentHitCount = 0;  // 通常戦の場合は0
        }
        
        try {
            battleUI->renderCommandSelectionUI(params);
        } catch (const std::exception& e) {
            std::cerr << "renderCommandSelectionUIエラー: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "renderCommandSelectionUIエラー: 不明なエラー" << std::endl;
        }
        
        // 説明UIを表示（初回の戦闘時のみ）
        if (showGameExplanation && explanationMessageBoard && !explanationMessageBoard->getText().empty()) {
            auto& config = UIConfig::UIConfigManager::getInstance();
            auto mbConfig = config.getMessageBoardConfig();
            
            int bgX = 20;  // 左下に配置
            int bgHeight = 60;  // 2行分の高さ
            int bgY = graphics.getScreenHeight() - bgHeight - 20;  // 画面下部から20px上
            int bgWidth = graphics.getScreenWidth() / 2;
            
            SDL_Color bgColor = mbConfig.backgroundColor;
            bgColor.a = 200;  // 半透明
            graphics.setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
            graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics.setDrawColor(mbConfig.borderColor.r, mbConfig.borderColor.g, mbConfig.borderColor.b, mbConfig.borderColor.a);
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
        
        // 夜のタイマーUIを表示
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        }
        
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
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto battleConfig = config.getBattleConfig();
        
        int playerBaseX, playerBaseY;
        config.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        // 窮地モードではplayer_adversity.pngを使用
        SDL_Texture* playerTex = hasUsedLastChanceMode ? graphics.getTexture("player_adversity") : graphics.getTexture("player");
        if (!playerTex && hasUsedLastChanceMode) {
            playerTex = graphics.getTexture("player"); // フォールバック
        }
        
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
        
        // HP表示（コマンド選択フェーズと同じ位置にするためrenderHPを使用）
        // 敵は描画しないので、適当な位置を渡す（敵のHPは表示されない）
        // VICTORY_DISPLAYフェーズでは敵のUIを非表示にする
        int dummyEnemyX = screenWidth;
        int dummyEnemyY = screenHeight;
        battleUI->renderHP(playerX, playerY, dummyEnemyX, dummyEnemyY, playerHeight, BattleConstants::BATTLE_CHARACTER_SIZE, "", true);
        
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
        
        // 夜のタイマーUIを表示
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
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
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto battleConfig = config.getBattleConfig();
        
        int playerBaseX, playerBaseY;
        config.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
        int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
        int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
        
        // 窮地モードではplayer_adversity.pngを使用
        SDL_Texture* playerTex = hasUsedLastChanceMode ? graphics.getTexture("player_adversity") : graphics.getTexture("player");
        if (!playerTex && hasUsedLastChanceMode) {
            playerTex = graphics.getTexture("player"); // フォールバック
        }
        
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
        
        // HP表示（コマンド選択フェーズと同じ位置にするためrenderHPを使用）
        // 敵は描画しないので、適当な位置を渡す（敵のHPは表示されない）
        // LEVEL_UP_DISPLAYフェーズでは敵のUIを非表示にする
        int dummyEnemyX = screenWidth;
        int dummyEnemyY = screenHeight;
        battleUI->renderHP(playerX, playerY, dummyEnemyX, dummyEnemyY, playerHeight, BattleConstants::BATTLE_CHARACTER_SIZE, "", true);
        
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
        
        std::string levelUpText = "レベルアップ！\n" + player->getName() + "はレベル" + std::to_string(player->getLevel()) + "になった！\n";
        levelUpText += "HP+" + std::to_string(hpGain) + " MP+" + std::to_string(mpGain) + " 攻撃力+" + std::to_string(attackGain) + " 防御力+" + std::to_string(defenseGain);
        
        int levelGained = player->getLevel() - oldLevel;
        if (levelGained > 1) {
            levelUpText = "レベルアップ！\n" + player->getName() + "はレベル" + std::to_string(oldLevel) + "からレベル" + std::to_string(player->getLevel()) + "になった！\n";
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
        
        // 夜のタイマーUIを表示
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    // JUDGE_RESULTフェーズの描画（VICTORY_DISPLAYやLEVEL_UP_DISPLAYに遷移していない場合のみ）
    if (currentPhase == BattlePhase::JUDGE_RESULT || 
        currentPhase == BattlePhase::DESPERATE_JUDGE_RESULT ||
        currentPhase == BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
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
            judgeParams.residentHitCount = residentHitCount;  // 住民戦で攻撃が成功した回数
        } else {
            judgeParams.playerCommandName = "";
            judgeParams.enemyCommandName = "";
            judgeParams.judgeResult = -999; // 未設定（battleLogicから取得）
            judgeParams.residentBehaviorHint = "";
            judgeParams.residentTurnCount = 0;
            judgeParams.residentHitCount = 0;  // 通常戦の場合は0
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
            params.residentHitCount = residentHitCount;  // 住民戦で攻撃が成功した回数
        } else {
            params.isVictory = (stats.playerWins > stats.enemyWins);
            params.isDefeat = (stats.enemyWins > stats.playerWins);
            params.isDesperateMode = battleLogic->getIsDesperateMode();
            params.hasThreeWinStreak = stats.hasThreeWinStreak;
            params.playerWins = stats.playerWins;
            params.enemyWins = stats.enemyWins;
            params.residentBehaviorHint = "";  // 通常戦の場合は空文字列
            params.residentHitCount = 0;  // 通常戦の場合は0
        }
        
        battleUI->renderResultAnnouncement(params);
        
        // 勝敗UIを表示（結果フェーズでは「自分が〜ターン攻撃します」も表示）
        renderWinLossUI(graphics, true);
        
        // ヒットエフェクトを描画
        effectManager->renderHitEffects(graphics);
        
        // 夜のタイマーUIを表示
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
        }
        
        // Rock-Paper-Scissors画像を表示（住民戦以外）
        renderRockPaperScissorsImage(graphics);
        
        // LAST_CHANCE_JUDGE_RESULTフェーズでは、キャラクター描画も行う（攻撃アニメーション表示のため）
        if (currentPhase == BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
            // キャラクター描画処理は1127行目以降で行われるため、returnしない
        } else {
            graphics.present();
            return;
        }
    }
    
    auto& shakeState = effectManager->getShakeState();
    
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
    // JSONからプレイヤーと敵の位置を取得
    auto& uiConfigManager = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = uiConfigManager.getBattleConfig();
    
    // デバッグ: 位置情報を確認（毎フレーム表示、変更時のみ）
    static int lastPlayerX = -1, lastPlayerY = -1;
    static float lastAbsoluteX = -1, lastAbsoluteY = -1;
    int playerBaseX, playerBaseY;
    uiConfigManager.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
    
    // JSONの値が変更された場合も検出
    bool jsonChanged = (battleConfig.playerPosition.absoluteX != lastAbsoluteX || 
                       battleConfig.playerPosition.absoluteY != lastAbsoluteY);
    bool positionChanged = (playerBaseX != lastPlayerX || playerBaseY != lastPlayerY);
    
    if (jsonChanged || positionChanged) {
        printf("BattleState: Player position calculated: (%d, %d) from JSON (absoluteX: %.0f, absoluteY: %.0f, useRelative: %s)\n", 
               playerBaseX, playerBaseY,
               battleConfig.playerPosition.absoluteX, 
               battleConfig.playerPosition.absoluteY,
               battleConfig.playerPosition.useRelative ? "true" : "false");
        lastPlayerX = playerBaseX;
        lastPlayerY = playerBaseY;
        lastAbsoluteX = battleConfig.playerPosition.absoluteX;
        lastAbsoluteY = battleConfig.playerPosition.absoluteY;
    }
    int playerX = playerBaseX;
    int playerY = playerBaseY;
    
    if (shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
        playerX += static_cast<int>(shakeState.shakeOffsetX);
        playerY += static_cast<int>(shakeState.shakeOffsetY);
    }
    
    // 窮地モードではplayer_adversity.pngを使用
    SDL_Texture* playerTexture = hasUsedLastChanceMode ? graphics.getTexture("player_adversity") : graphics.getTexture("player");
    if (!playerTexture && hasUsedLastChanceMode) {
        playerTexture = graphics.getTexture("player"); // フォールバック
    }
    
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
        auto& attackMultiplierConfig = battleConfig.attackMultiplier;
        float multiplier = player->getNextTurnMultiplier();
        int turns = player->getNextTurnBonusTurns();
        // 倍率を文字列に変換（小数点以下1桁まで表示）
        int multiplierInt = static_cast<int>(multiplier * 10);
        std::string multiplierStr = std::to_string(multiplierInt / 10) + "." + std::to_string(multiplierInt % 10);
        // プレースホルダーを使わずに直接文字列を組み立て（文字化けを防ぐため）
        std::string statusText = "攻撃倍率: " + multiplierStr + "倍 (残り" + std::to_string(turns) + "ターン)";
        SDL_Color statusColor = attackMultiplierConfig.textColor;
        SDL_Texture* statusTexture = graphics.createTextTexture(statusText, "default", statusColor);
        if (statusTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(statusTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = static_cast<int>(playerHpX + attackMultiplierConfig.offsetX - attackMultiplierConfig.padding);
            int bgY = static_cast<int>(playerHpY + attackMultiplierConfig.offsetY - attackMultiplierConfig.padding);
            if (shakeState.shakeTargetPlayer && shakeState.shakeTimer > 0.0f) {
                bgX += static_cast<int>(shakeState.shakeOffsetX);
                bgY += static_cast<int>(shakeState.shakeOffsetY);
            }
            graphics.setDrawColor(attackMultiplierConfig.bgColor.r, attackMultiplierConfig.bgColor.g, attackMultiplierConfig.bgColor.b, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + attackMultiplierConfig.padding * 2, textHeight + attackMultiplierConfig.padding * 2, true);
            graphics.setDrawColor(attackMultiplierConfig.borderColor.r, attackMultiplierConfig.borderColor.g, attackMultiplierConfig.borderColor.b, attackMultiplierConfig.borderColor.a);
            graphics.drawRect(bgX, bgY, textWidth + attackMultiplierConfig.padding * 2, textHeight + attackMultiplierConfig.padding * 2, false);
            SDL_DestroyTexture(statusTexture);
        }
        int statusTextX = static_cast<int>(playerHpX + attackMultiplierConfig.offsetX);
        int statusTextY = static_cast<int>(playerHpY + attackMultiplierConfig.offsetY);
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
    
    int enemyBaseX, enemyBaseY;
    uiConfigManager.calculatePosition(enemyBaseX, enemyBaseY, battleConfig.enemyPosition, screenWidth, screenHeight);
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
                                currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT ||
                                currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT)) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            explanationStep++;
            
            if (explanationStep >= gameExplanationTexts.size()) {
                showGameExplanation = false;
                explanationStep = 0;
                clearExplanationMessage();
                
                // 説明UIが完全に終わったことを記録
                if (currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT || hasUsedLastChanceMode) {
                    // 終焉解放モードの場合
                    player->hasSeenLastChanceExplanation = true;
                    // 説明が終わったら、コマンド選択フェーズに遷移（まだ遷移していない場合）
                    if (currentPhase != BattlePhase::LAST_CHANCE_COMMAND_SELECT) {
                        currentPhase = BattlePhase::LAST_CHANCE_COMMAND_SELECT;
                        battleLogic->setCommandTurnCount(BattleConstants::LAST_CHANCE_TURN_COUNT);
                        initializeCommandSelection();
                        phaseTimer = 0.0f;
                        addBattleLog("最後のチャンス！5ターンで勝負だ！");
                    }
                } else if (enemy->isResident()) {
                    player->hasSeenResidentBattleExplanation = true;
                } else {
                    player->hasSeenBattleExplanation = true;
                }
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
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT ||
         currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT) && 
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
    
    // 説明用メッセージボード（JSONから取得）
    int explanationX, explanationY;
    config.calculatePosition(explanationX, explanationY, battleConfig.explanationMessageBoard.text.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    auto explanationLabelPtr = std::make_unique<Label>(explanationX, explanationY, "", "default");
    explanationLabelPtr->setColor(battleConfig.explanationMessageBoard.text.color);
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
        
        // メッセージ表示時に効果音を再生
        AudioManager::getInstance().playSound("decide", 0);
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
    // 住民の場合は夜の背景、衛兵・王様の場合は城の背景、魔王の場合は魔王の背景、それ以外は通常の戦闘背景を使用
    SDL_Texture* bgTexture = nullptr;
    if (enemy->isResident()) {
        bgTexture = graphics.getTexture("night_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/night_bg.png", "night_bg");
        }
    } else if (enemy->getType() == EnemyType::GUARD || enemy->getType() == EnemyType::KING) {
        bgTexture = graphics.getTexture("castle_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/castle_bg.png", "castle_bg");
        }
    } else if (enemy->getType() == EnemyType::DEMON_LORD) {
        bgTexture = graphics.getTexture("demon_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/demon_bg.png", "demon_bg");
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
                residentHitCount++;
                
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
        // 最後のチャンスモードをまだ使っていない場合、最後のチャンスモードに遷移
        if (!hasUsedLastChanceMode && 
            currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT &&
            currentPhase != BattlePhase::LAST_CHANCE_COMMAND_SELECT &&
            currentPhase != BattlePhase::LAST_CHANCE_JUDGE) {
            hasUsedLastChanceMode = true;
            // HPを1に回復（0のままだと即座にゲームオーバーになるため）
            player->setHp(1);
            // isAliveを明示的にtrueに設定（setHp()はhp <= 0の場合のみisAlive = falseを設定するため）
            player->setIsAlive(true);
            // BattleUIにも窮地モード開始を通知
            if (battleUI) {
                battleUI->setHasUsedLastChanceMode(true);
            }
            // 最後のチャンスモードのイントロフェーズに遷移（終焉突破UI表示）
            currentPhase = BattlePhase::LAST_CHANCE_INTRO;
            phaseTimer = 0.0f; // タイマーをリセット
            damageListPrepared = false; // ダメージリストを再度準備できるようにリセット
            enemyAttackStarted = false; // 敵の攻撃アニメーションフラグをリセット
            lastChanceIntroMusicStopped = false; // 音楽停止フラグをリセット
            adversityMusicStarted = false; // 音楽開始フラグをリセット
            return;
        }
        
        // 既に最後のチャンスモードを使った場合、または最後のチャンスモードの結果フェーズから呼ばれた場合は通常通りゲームオーバー
        lastResult = BattleResult::PLAYER_DEFEAT;
        addBattleLog("戦闘に敗北しました。勇者が倒れました。");
        // JUDGE_RESULTフェーズの場合は、RESULTフェーズをスキップして直接ゲームオーバーに遷移
        if (currentPhase == BattlePhase::JUDGE_RESULT || 
            currentPhase == BattlePhase::DESPERATE_JUDGE_RESULT ||
            currentPhase == BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
            endBattle();
            return;
        }
        currentPhase = BattlePhase::RESULT;
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
            // 戦闘曲を停止してclear.oggを再生
            AudioManager::getInstance().stopMusic();
            AudioManager::getInstance().playMusic("clear", -1);
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
            
            // 目標レベル達成用の敵の場合、特別な処理
            if (isTargetLevelEnemy) {
                // プレイヤーレベルを目標レベルまで一気に上げる
                int targetLevel = TownState::s_targetLevel;
                int currentLevel = player->getLevel();
                
                if (currentLevel < targetLevel) {
                    // 目標レベルまでレベルアップ
                    while (player->getLevel() < targetLevel) {
                        player->levelUp();
                    }
                    // レベルアップ時のステータス増加を考慮して、oldLevelを更新
                    oldLevel = currentLevel;
                    oldMaxHp = player->getMaxHp();
                    oldMaxMp = player->getMaxMp();
                    oldAttack = player->getAttack();
                    oldDefense = player->getDefense();
                    
                    // 経験値とゴールドも通常通り付与
                    player->gainGold(goldGained);
                    player->changeKingTrust(3);
                    
                    victoryExpGained = expGained;
                    victoryGoldGained = goldGained;
                    victoryEnemyName = enemy->getTypeName();
                    
                    std::string victoryMessage = enemy->getTypeName() + "を倒した！\n目標レベル" + std::to_string(targetLevel) + "に到達した！\n" + std::to_string(goldGained) + "ゴールドを獲得！\n王様からの信頼度が3上昇した！";
                    addBattleLog(victoryMessage);
                    
                    // 目標レベル達成フラグを設定
                    TownState::s_levelGoalAchieved = true;
                } else {
                    // 既に目標レベルに達している場合（通常の処理）
            player->gainExp(expGained);
            player->gainGold(goldGained);
                    player->changeKingTrust(3);
                    
                    victoryExpGained = expGained;
                    victoryGoldGained = goldGained;
                    victoryEnemyName = enemy->getTypeName();
                    
                    std::string victoryMessage = enemy->getTypeName() + "は倒れた...\n" + std::to_string(expGained) + "の経験値を得た！\n王様からの信頼度が3上昇した！";
                    addBattleLog(victoryMessage);
                }
            } else {
                // 通常の敵の場合
                player->gainExp(expGained);
                player->gainGold(goldGained);
            player->changeKingTrust(3);
            
            victoryExpGained = expGained;
            victoryGoldGained = goldGained;
            victoryEnemyName = enemy->getTypeName();
            
            std::string victoryMessage = enemy->getTypeName() + "は倒れた...\n" + std::to_string(expGained) + "の経験値を得た！\n王様からの信頼度が3上昇した！";
            addBattleLog(victoryMessage);
            }
            
            currentPhase = BattlePhase::VICTORY_DISPLAY;
            phaseTimer = 0;
            // 戦闘曲を停止してclear.oggを再生
            AudioManager::getInstance().stopMusic();
            AudioManager::getInstance().playMusic("clear", -1);
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
    // 戦闘終了時にHPを全回復（敗北時を除く）
    if (lastResult != BattleResult::PLAYER_DEFEAT && player) {
        player->setHp(player->getMaxHp());
    }
    
    if (stateManager) {
        if (lastResult == BattleResult::PLAYER_DEFEAT) {
            // 敵の情報を渡してGameOverStateを作成（再戦用）
            stateManager->changeState(std::make_unique<GameOverState>(
                player, "戦闘に敗北しました。", enemy->getType(), enemy->getLevel(),
                false, "", -1, -1, -1, isTargetLevelEnemy));
        } else if (enemy->isResident()) {
            // 住民との戦闘の場合、NightStateに戻り、住民を倒した処理を実行
            int residentX = enemy->getResidentX();
            int residentY = enemy->getResidentY();
            
            // 住民の位置情報をPlayerに保存（NightState::enter()で処理を実行）
            player->addKilledResident(residentX, residentY);
            
            // NightStateに戻る（enter()で住民を倒した処理を実行）
            stateManager->changeState(std::make_unique<NightState>(player));
        } else if (enemy->getType() == EnemyType::GUARD) {
            // 衛兵との戦闘の場合、レベルで判定
            if (enemy->getLevel() == 110) {
                // 城の衛兵（レベル110）の場合、CastleStateに戻り、衛兵を倒した処理を実行
                if (stateManager) {
                    auto castleState = std::make_unique<CastleState>(player, true); // fromNightStateをtrueにする
                    // 保存された状態を復元してから、左衛兵か右衛兵かを判定
                    const nlohmann::json* savedState = player->getSavedGameState();
                    if (savedState && !savedState->is_null() && 
                        savedState->contains("stateType") && 
                        static_cast<StateType>((*savedState)["stateType"]) == StateType::CASTLE) {
                        castleState->fromJson(*savedState);
                    }
                    // まだ倒していない方を倒したとみなす
                    if (!castleState->isGuardLeftDefeated()) {
                        castleState->onGuardDefeated(true); // 左衛兵を倒した
                    } else if (!castleState->isGuardRightDefeated()) {
                        castleState->onGuardDefeated(false); // 右衛兵を倒した
                    }
                    // 状態を保存（倒された後の状態を保存）
                    // saveCurrentState()は使えない（まだstateManagerに追加されていないため）、直接toJson()を呼ぶ
                    nlohmann::json stateJson = castleState->toJson();
                    player->setSavedGameState(stateJson);
                    stateManager->changeState(std::move(castleState));
                }
            } else {
                // 夜の街の衛兵（レベル105）の場合、NightStateに戻る
                stateManager->changeState(std::make_unique<NightState>(player));
            }
        } else if (enemy->getType() == EnemyType::KING) {
            // 王様との戦闘の場合、CastleStateに戻り、王様を倒した処理を実行
            if (stateManager) {
                auto castleState = std::make_unique<CastleState>(player, true); // fromNightStateをtrueにする
                // 保存された状態を復元してから、王様を倒した処理を実行
                const nlohmann::json* savedState = player->getSavedGameState();
                if (savedState && !savedState->is_null() && 
                    savedState->contains("stateType") && 
                    static_cast<StateType>((*savedState)["stateType"]) == StateType::CASTLE) {
                    castleState->fromJson(*savedState);
                }
                castleState->onKingDefeated(); // 王様を倒したことを通知
                // 状態を保存（倒された後の状態を保存）
                // saveCurrentState()は使えない（まだstateManagerに追加されていないため）、直接toJson()を呼ぶ
                nlohmann::json stateJson = castleState->toJson();
                player->setSavedGameState(stateJson);
                stateManager->changeState(std::move(castleState));
            }
        } else {
            if (enemy->getType() == EnemyType::DEMON_LORD) {
                stateManager->changeState(std::make_unique<EndingState>(player));
            } else {
                // 目標レベル達成用の敵に勝利した場合、夜の街に遷移
                if (isTargetLevelEnemy) {
                    // 目標レベル達成フラグは既にcheckBattleEnd()で設定済み
                    // 夜の街に遷移
                    stateManager->changeState(std::make_unique<NightState>(player));
                    player->setCurrentNight(player->getCurrentNight() + 1);
                    TownState::s_nightCount++;
                } else {
                    // 通常の敵の場合
                    // 目標レベルに達した場合は夜の街に遷移
                    // update()で既にs_levelGoalAchievedがtrueになっている可能性があるため、
                    // 目標レベルに達しているかどうかのみをチェック
                    if (player->getLevel() >= TownState::s_targetLevel) {
                        // まだs_levelGoalAchievedがfalseの場合のみ設定
                        if (!TownState::s_levelGoalAchieved) {
                            TownState::s_levelGoalAchieved = true;
                        }
                        // 夜の街に遷移
                        stateManager->changeState(std::make_unique<NightState>(player));
                        player->setCurrentNight(player->getCurrentNight() + 1);
                        TownState::s_nightCount++;
            } else {
                stateManager->changeState(std::make_unique<FieldState>(player));
                    }
                }
            }
        }
    }
} 

void BattleState::handleOptionSelection(const InputManager& input) {
    if (currentPhase == BattlePhase::SPELL_SELECTION) {
        if (input.isKeyJustPressed(InputKey::LEFT) || input.isKeyJustPressed(InputKey::A)) {
            if (selectedOption > 0) {
                selectedOption--;
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::RIGHT) || input.isKeyJustPressed(InputKey::D)) {
            if (selectedOption < currentOptions.size() - 1) {
                selectedOption++;
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            int oldOption = selectedOption;
            if (selectedOption >= 4 && selectedOption < 8) {
                selectedOption -= 4;
            } else if (selectedOption == currentOptions.size() - 1) {
                selectedOption = 4;
            }
            if (oldOption != selectedOption) {
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            int oldOption = selectedOption;
            if (selectedOption < 4) {
                if (selectedOption + 4 < currentOptions.size()) {
                    selectedOption += 4;
                }
            } else if (selectedOption >= 4 && selectedOption < 8) {
                selectedOption = currentOptions.size() - 1;
            }
            if (oldOption != selectedOption) {
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        }
    } else {
        if (input.isKeyJustPressed(InputKey::LEFT) || input.isKeyJustPressed(InputKey::A)) {
            int oldOption = selectedOption;
            selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
            if (oldOption != selectedOption) {
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::RIGHT) || input.isKeyJustPressed(InputKey::D)) {
            int oldOption = selectedOption;
            selectedOption = (selectedOption + 1) % currentOptions.size();
            if (oldOption != selectedOption) {
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            int oldOption = selectedOption;
            selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
            if (oldOption != selectedOption) {
                AudioManager::getInstance().playSound("button", 0);
            }
            updateOptionDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            int oldOption = selectedOption;
            selectedOption = (selectedOption + 1) % currentOptions.size();
            if (oldOption != selectedOption) {
                AudioManager::getInstance().playSound("button", 0);
            }
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
                    int oldOption = selectedOption;
                    if (selectedOption >= 4 && selectedOption < 8) {
                        selectedOption -= 4;
                    } else if (selectedOption == currentOptions.size() - 1) {
                        selectedOption = 4;
                    }
                    if (oldOption != selectedOption) {
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickY > DEADZONE) {
                    int oldOption = selectedOption;
                    if (selectedOption < 4) {
                        if (selectedOption + 4 < currentOptions.size()) {
                            selectedOption += 4;
                        }
                    } else if (selectedOption >= 4 && selectedOption < 8) {
                        selectedOption = currentOptions.size() - 1;
                    }
                    if (oldOption != selectedOption) {
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX < -DEADZONE) {
                    if (selectedOption > 0) {
                        selectedOption--;
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX > DEADZONE) {
                    if (selectedOption < currentOptions.size() - 1) {
                        selectedOption++;
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                }
            } else {
                if (stickY < -DEADZONE) {
                    int oldOption = selectedOption;
                    selectedOption = (selectedOption - 1 + currentOptions.size()) % currentOptions.size();
                    if (oldOption != selectedOption) {
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickY > DEADZONE) {
                    int oldOption = selectedOption;
                    selectedOption = (selectedOption + 1) % currentOptions.size();
                    if (oldOption != selectedOption) {
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX < -DEADZONE) {
                    int oldOption = selectedOption;
                    selectedOption = (selectedOption - 2 + currentOptions.size()) % currentOptions.size();
                    if (oldOption != selectedOption) {
                        AudioManager::getInstance().playSound("button", 0);
                    }
                    updateOptionDisplay();
                    stickTimer = STICK_DELAY;
                } else if (stickX > DEADZONE) {
                    int oldOption = selectedOption;
                    selectedOption = (selectedOption + 2) % currentOptions.size();
                    if (oldOption != selectedOption) {
                        AudioManager::getInstance().playSound("button", 0);
                    }
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
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT ||
         currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT) &&
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
    
    // コマンド選択時にdecide.oggを再生
    AudioManager::getInstance().playSound("decide", 0);
    
    std::string selected = currentOptions[selectedOption];
    isShowingOptions = false;
    
    if (currentPhase == BattlePhase::DESPERATE_MODE_PROMPT) {
        if (selected == "大勝負に挑む") {
            battleLogic->setCommandTurnCount(BattleConstants::DESPERATE_TURN_COUNT);
            battleLogic->setDesperateMode(true);
            currentPhase = BattlePhase::DESPERATE_COMMAND_SELECT;
            initializeCommandSelection();
            // コマンド選択フェーズに入った時にbattle.oggを再生（初回のみ）
            if (!battleMusicStarted) {
                AudioManager::getInstance().playMusic("battle", -1);
                battleMusicStarted = true;
            }
        } else if (selected == "通常戦闘を続ける") {
            battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
            battleLogic->setDesperateMode(false);
            currentPhase = BattlePhase::COMMAND_SELECT;
            initializeCommandSelection();
            // コマンド選択フェーズに入った時にbattle.oggを再生（初回のみ）
            if (!battleMusicStarted) {
                AudioManager::getInstance().playMusic("battle", -1);
                battleMusicStarted = true;
            }
        }
    } else if (currentPhase == BattlePhase::COMMAND_SELECT || 
               currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT ||
               currentPhase == BattlePhase::LAST_CHANCE_COMMAND_SELECT) {
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
                // 最初のコマンド表示時にcommand.oggを再生
                AudioManager::getInstance().playSound("command", 0);
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
    
    // 初回の戦闘時のみ説明を開始（静的変数で管理）
    // また、説明UIを既に見た場合は表示しない
    // 終焉解放モード（LAST_CHANCE_COMMAND_SELECT）の場合は説明UIを設定しない（終焉解放の説明UIが別途設定されるため）
    static bool s_firstBattle = true;
    static bool s_firstResidentBattle = true;
    
    // 終焉解放モードの場合は説明UIを設定しない（終焉解放の説明UIが別途設定されるため）
    if (currentPhase != BattlePhase::LAST_CHANCE_COMMAND_SELECT && !hasUsedLastChanceMode) {
        if (enemy->isResident()) {
            // 住民戦の場合
            if (s_firstResidentBattle && !player->hasSeenResidentBattleExplanation) {
                setupResidentBattleExplanation();
                showGameExplanation = true;
                explanationStep = 0;
                s_firstResidentBattle = false;
                // explanationMessageBoardはsetupUI()で初期化されるので、render()で設定する
            } else if (player->hasSeenResidentBattleExplanation) {
                // 既に見た場合は、静的変数も更新
                s_firstResidentBattle = false;
            }
        } else {
            // 通常の戦闘の場合
            if (s_firstBattle && !player->hasSeenBattleExplanation) {
                setupGameExplanation();
                showGameExplanation = true;
                explanationStep = 0;
                s_firstBattle = false;
                // explanationMessageBoardはsetupUI()で初期化されるので、render()で設定する
            } else if (player->hasSeenBattleExplanation) {
                // 既に見た場合は、静的変数も更新
                s_firstBattle = false;
            }
        }
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
    
    // 前のターンで選択したコマンドの位置にカーソルを初期化
    selectedOption = 0;
    if (turnIndex > 0) {
        // 前のターン（turnIndex - 1）のコマンドを取得
        auto playerCmds = battleLogic->getPlayerCommands();
        if (turnIndex - 1 < static_cast<int>(playerCmds.size()) && playerCmds[turnIndex - 1] >= 0) {
            int previousCmd = playerCmds[turnIndex - 1];
            
            if (enemy->isResident()) {
                // 住民戦の場合：コマンド0（攻撃）→ 0、コマンド20（身を隠す）→ 1
                if (previousCmd == BattleConstants::COMMAND_ATTACK) {
                    selectedOption = 0;
                } else if (previousCmd == BattleConstants::PLAYER_COMMAND_HIDE) {
                    selectedOption = 1;
                }
            } else {
                // 通常の戦闘：コマンド0（攻撃）→ 0、コマンド1（防御）→ 1、コマンド2（呪文）→ 2
                if (previousCmd >= 0 && previousCmd < static_cast<int>(currentOptions.size())) {
                    selectedOption = previousCmd;
                }
            }
        }
    } else if (enemy->isResident() && currentResidentPlayerCommand >= 0) {
        // 住民戦で1ターン目の場合、前回の戦闘のコマンドを確認
        if (currentResidentPlayerCommand == BattleConstants::COMMAND_ATTACK) {
            selectedOption = 0;
        } else if (currentResidentPlayerCommand == BattleConstants::PLAYER_COMMAND_HIDE) {
            selectedOption = 1;
        }
    }
    
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
    int maxTurn = (currentPhase == BattlePhase::JUDGE || currentPhase == BattlePhase::DESPERATE_JUDGE || currentPhase == BattlePhase::LAST_CHANCE_JUDGE) 
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
    gameExplanationTexts.push_back("3すくみの関係になっていて、右上の図のように攻撃は呪文に強い、\n呪文は防御に強い、防御は攻撃に強いという特徴を持っています。");
    gameExplanationTexts.push_back("そして敵には行動パターンが存在しています。\n攻撃型、防御型、呪文型の3種類で、どれをよく使うかというものです。");
    gameExplanationTexts.push_back("つまり攻撃型の敵に対しては\n防御コマンドを使うと効果的だということですね！");
    gameExplanationTexts.push_back("では相手の型はどこに書いているかというと・・・");
    gameExplanationTexts.push_back("実は戦闘開始時は書いていないんですね！");
    gameExplanationTexts.push_back("その代わり、敵の下に〜型ではないようだと書いてますね！\nこれで敵の型が2種類に絞れるというわけです。");
    gameExplanationTexts.push_back("実際の相手の行動を見て、どちらの型なのかを見極めてください！");
    gameExplanationTexts.push_back("でも型を1つに絞って表示してくれないのは少し意地悪ですね。\n実は自分の体力が7割を切った際に相手の型が表示されます。");
    gameExplanationTexts.push_back("それまでは予測して戦ってみてくださいね。");
    gameExplanationTexts.push_back("最後に左上のターンについて説明します。");
    gameExplanationTexts.push_back("本作の戦闘は最初に3ターン分の行動を選択するようになっています。\nそして勝った回数が多い方が一方的にそのターン数だけ攻撃できます！");
    gameExplanationTexts.push_back("なので勝ちまくれば無傷で戦闘を終了させることもできるんですね。\nでも逆に一発逆転されるってことも・・・？賭けみたいですね！");
    gameExplanationTexts.push_back("長くなってしまいましたが一旦実際に戦ってみましょう！");
}

void BattleState::setupResidentBattleExplanation() {
    gameExplanationTexts.clear();
    gameExplanationTexts.push_back("これから住民と戦っていきます！");
    gameExplanationTexts.push_back("住民戦では1ターンずつ進んでいきます。\n10ターン以内に倒さないと、衛兵に見つかりゲームオーバーです！");
    gameExplanationTexts.push_back("そしてまた同じようなコマンドが見えますね！今度は攻撃と隠密です！");
    gameExplanationTexts.push_back("攻撃は住民を攻撃するコマンドです！\n隠密は衛兵に見つからないように身を潜めるコマンドです！");
    gameExplanationTexts.push_back("住民戦ではメンタルの影響で攻撃を躊躇うことがあるので注意です！");
    gameExplanationTexts.push_back("また、住民は「恐怖」と「求援」しか行うことができません。");
    gameExplanationTexts.push_back("自分が攻撃をした際に相手が恐怖だったら攻撃成功、\n求援をしたら衛兵に見つかりゲームオーバーという感じです！");
    gameExplanationTexts.push_back("求援をしそうだと思ったら隠密を選んで身を潜めてくださいね！");
    gameExplanationTexts.push_back("そして、住民の下に書いているのは住民の次の行動予測です！");
    gameExplanationTexts.push_back("予測できない時もあるのですが、震えが止まらない時は恐怖を行い、\n辺りを見渡している時は求援を使います！");
    gameExplanationTexts.push_back("このヒントを上手く使い、衛兵に見つかる前に住民を倒してください！");
    gameExplanationTexts.push_back("さあ、早速やってみましょう！");
}

void BattleState::setupLastChanceExplanation() {
    gameExplanationTexts.clear();
    gameExplanationTexts.push_back("やられたと思いましたかー？");
    gameExplanationTexts.push_back("実はあなたは窮地に立たされた時、\n「終焉解放」をすることができるんです！");
    gameExplanationTexts.push_back("これは死と隣り合わせの状況の時、\nあなたに秘められた力を全て解放するというものです！");
    gameExplanationTexts.push_back("終焉解放時、1度だけ5ターン分の選択ができます！\n5ターンのうちに敵を倒せたら大逆転です！ルールはいつも通りです！");
    gameExplanationTexts.push_back("しかし勝利数が2回以下だったり、敵を倒せなかったりすると・・・");
    gameExplanationTexts.push_back("見逃してはくれませんからゲームオーバーになってしまいますね・・・");
    gameExplanationTexts.push_back("さあ、最後の力を振り絞ってラッシュをかましちゃいましょう！");
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
        
        // JSONから設定を取得（ホットリロード対応のため、毎回取得）
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto battleConfig = config.getBattleConfig();
        
        // 画像のサイズを取得
        int textureWidth, textureHeight;
        SDL_QueryTexture(rpsTexture, nullptr, nullptr, &textureWidth, &textureHeight);
        
        // JSONから設定を取得
        int displayWidth = battleConfig.rockPaperScissors.width;
        int displayHeight = static_cast<int>(textureHeight * (static_cast<float>(displayWidth) / textureWidth));
        
        // 位置を計算（calculatePositionを使用）
        int posX, posY;
        config.calculatePosition(posX, posY, battleConfig.rockPaperScissors.position, screenWidth, screenHeight);
        
        // absoluteXが0の場合は中央に配置（後方互換性のため）
        if (battleConfig.rockPaperScissors.position.absoluteX == 0.0f && !battleConfig.rockPaperScissors.position.useRelative) {
            posX = (screenWidth - displayWidth) / 2;
        }
        
        graphics.drawTexture(rpsTexture, posX, posY, displayWidth, displayHeight);
    }
}

void BattleState::renderWinLossUI(Graphics& graphics, bool isResultPhase) {
    // JSONから設定を取得
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    
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
    
    // 勝敗UIはVSの真下に配置（JSONから設定を取得）
    auto& winLossTextConfig = battleConfig.winLossUI.winLossText;
    int winLossY = vsBottomY + static_cast<int>(winLossTextConfig.position.offsetY);
    
    // ジャッジ結果フェーズでは「自分〜勝」テキストを非表示にする
    int textHeight = 0;  // 結果フェーズでは0、通常フェーズでは実際の高さを設定
    if (!isResultPhase) {
        // 勝敗テキスト（JSONからフォーマットを取得）
        std::string winLossText = winLossTextConfig.format;
        // プレースホルダーを置換（安全な方法：文字列を前後で結合）
        size_t pos = winLossText.find("{playerWins}");
        if (pos != std::string::npos) {
            winLossText = winLossText.substr(0, pos) + std::to_string(playerWins) + winLossText.substr(pos + 12);
        }
        pos = winLossText.find("{enemyWins}");
        if (pos != std::string::npos) {
            winLossText = winLossText.substr(0, pos) + std::to_string(enemyWins) + winLossText.substr(pos + 11);
        }
        SDL_Color textColor = winLossTextConfig.color;
        
        SDL_Texture* textTexture = graphics.createTextTexture(winLossText, "default", textColor);
        if (textTexture) {
            int textWidth;
            SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            int padding = winLossTextConfig.padding;
            int bgX = centerX - textWidth / 2 - padding;
            int bgY = winLossY - padding;
            
            // 背景黒
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            
            // テキスト白
            graphics.drawText(winLossText, centerX - textWidth / 2, winLossY, "default", textColor);
            
            SDL_DestroyTexture(textTexture);
        }
    }
    
    // 結果フェーズのみ、ターン数表示と現在実行中のターンに応じたメッセージを表示
    if (isResultPhase) {
        // 1. 「〜ターン分の攻撃を実行」を常に表示（勝敗UIの下）
        auto& totalAttackTextConfig = battleConfig.winLossUI.totalAttackText;
        std::string totalAttackText;
        // 住民戦で攻撃失敗時は「ためらいました」を表示
        if (enemy->isResident() && residentAttackFailed) {
            totalAttackText = totalAttackTextConfig.hesitateFormat;
            size_t pos = totalAttackText.find("{playerName}");
            if (pos != std::string::npos) {
                totalAttackText = totalAttackText.substr(0, pos) + player->getName() + totalAttackText.substr(pos + 12);
            }
        } else if (playerWins > enemyWins) {
            totalAttackText = totalAttackTextConfig.playerWinFormat;
            size_t pos = totalAttackText.find("{playerName}");
            if (pos != std::string::npos) {
                totalAttackText = totalAttackText.substr(0, pos) + player->getName() + totalAttackText.substr(pos + 12);
            }
            pos = totalAttackText.find("{turns}");
            if (pos != std::string::npos) {
                totalAttackText = totalAttackText.substr(0, pos) + std::to_string(playerWins) + totalAttackText.substr(pos + 7);
            }
        } else if (enemyWins > playerWins) {
            totalAttackText = totalAttackTextConfig.enemyWinFormat;
            size_t pos = totalAttackText.find("{turns}");
            if (pos != std::string::npos) {
                totalAttackText = totalAttackText.substr(0, pos) + std::to_string(enemyWins) + totalAttackText.substr(pos + 7);
            }
        } else {
            totalAttackText = totalAttackTextConfig.drawFormat;
        }
        
        SDL_Texture* totalAttackTexture = graphics.createTextTexture(totalAttackText, "default", totalAttackTextConfig.color);
        if (totalAttackTexture) {
            int totalAttackTextWidth, totalAttackTextHeight;
            SDL_QueryTexture(totalAttackTexture, nullptr, nullptr, &totalAttackTextWidth, &totalAttackTextHeight);
            
            // 勝敗UIの下に配置（JSONから設定を取得）
            // 結果フェーズでは「自分〜勝」テキストを表示しないので、winLossYから直接オフセットを適用
            int totalAttackY = winLossY + textHeight + static_cast<int>(totalAttackTextConfig.position.offsetY);
            int totalAttackPadding = totalAttackTextConfig.padding;
            int totalAttackBgX = centerX - totalAttackTextWidth / 2 - totalAttackPadding;
            int totalAttackBgY = totalAttackY - totalAttackPadding;
            
            // 背景黒
            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics.drawRect(totalAttackBgX, totalAttackBgY, totalAttackTextWidth + totalAttackPadding * 2, totalAttackTextHeight + totalAttackPadding * 2, true);
            
            // テキスト白
            graphics.drawText(totalAttackText, centerX - totalAttackTextWidth / 2, totalAttackY, "default", totalAttackTextConfig.color);
            
            SDL_DestroyTexture(totalAttackTexture);
            
            // 2. 現在実行中のターンに応じたメッセージを表示（「〜ターン分の攻撃を実行」の下）
            if (playerWins > enemyWins) {
                    auto& attackTextConfig = battleConfig.winLossUI.attackText;
                    std::string attackText;
                    
                    // 現在実行中のターンに対応するメッセージを生成
                    // skipAnimationがtrueの場合でも、テキストは表示する必要があるため、
                    // pendingDamagesを全てチェックして、最初に見つかったコマンドを表示する
                    int displayTurnIndex = currentExecutingTurn;
                    // skipAnimationがtrueでcurrentExecutingTurnが既にインクリメントされている場合、
                    // 最初のpendingDamagesエントリを表示する
                    if (displayTurnIndex >= static_cast<int>(pendingDamages.size()) && !pendingDamages.empty()) {
                        displayTurnIndex = 0;
                    }
                    
                    if (displayTurnIndex < static_cast<int>(pendingDamages.size())) {
                        const auto& damageInfo = pendingDamages[displayTurnIndex];
                        
                        if (damageInfo.commandType == BattleConstants::COMMAND_ATTACK) {
                            attackText = attackTextConfig.attackFormat;
                        } else if (damageInfo.commandType == BattleConstants::COMMAND_DEFEND) {
                            attackText = attackTextConfig.rushFormat;
                        } else if (damageInfo.commandType == BattleConstants::COMMAND_SPELL) {
                            // 呪文の種類を取得
                            auto it = executedSpellsByDamageIndex.find(displayTurnIndex);
                            if (it != executedSpellsByDamageIndex.end()) {
                                SpellType spellType = it->second;
                                switch (spellType) {
                                    case SpellType::STATUS_UP:
                                        attackText = attackTextConfig.statusUpSpellFormat;
                                        break;
                                    case SpellType::HEAL:
                                        attackText = attackTextConfig.healSpellFormat;
                                        break;
                                    case SpellType::ATTACK:
                                        attackText = attackTextConfig.attackSpellFormat;
                                        break;
                                    default:
                                        attackText = attackTextConfig.defaultSpellFormat;
                                        break;
                                }
                            } else {
                                attackText = attackTextConfig.defaultSpellFormat;
                            }
                        } else {
                            // デフォルトメッセージ
                            attackText = attackTextConfig.defaultAttackFormat;
                        }
                        
                        // プレースホルダーを置換（安全な方法：文字列を前後で結合）
                        size_t pos = attackText.find("{playerName}");
                        if (pos != std::string::npos) {
                            attackText = attackText.substr(0, pos) + player->getName() + attackText.substr(pos + 12);
                        }
                    }
                    
                    if (!attackText.empty()) {
                        SDL_Texture* attackTexture = graphics.createTextTexture(attackText, "default", attackTextConfig.color);
                        if (attackTexture) {
                            int attackTextWidth, attackTextHeight;
                            SDL_QueryTexture(attackTexture, nullptr, nullptr, &attackTextWidth, &attackTextHeight);
                            
                            // 「〜ターン分の攻撃を実行」の下に配置（JSONから設定を取得）
                            int attackY = totalAttackY + totalAttackTextHeight + static_cast<int>(attackTextConfig.position.offsetY);
                            int attackPadding = attackTextConfig.padding;
                            int attackBgX = centerX - attackTextWidth / 2 - attackPadding;
                            int attackBgY = attackY - attackPadding;
                            
                            // 背景黒
                            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                            graphics.drawRect(attackBgX, attackBgY, attackTextWidth + attackPadding * 2, attackTextHeight + attackPadding * 2, true);
                            
                            // テキスト白
                            graphics.drawText(attackText, centerX - attackTextWidth / 2, attackY, "default", attackTextConfig.color);
                            
                            SDL_DestroyTexture(attackTexture);
                        }
            }
        } else if (enemyWins > playerWins) {
            // 敵が勝った場合の個別攻撃メッセージを表示
            auto& attackTextConfig = battleConfig.winLossUI.attackText;
                    
                    // すべてのpendingDamagesをチェックして、特殊技名と効果メッセージの両方がある最初のターンを見つける
                    std::string skillNameText = "";
                    std::string effectMessageText = "";
                    int foundIndex = -1;
                    
                    // まず、特殊技名と効果メッセージの両方があるターンを探す
                    for (size_t i = 0; i < pendingDamages.size(); i++) {
                        const auto& damageInfo = pendingDamages[i];
                        if (damageInfo.isSpecialSkill && !damageInfo.specialSkillName.empty() && !damageInfo.specialSkillEffectMessage.empty()) {
                            skillNameText = enemy->getTypeName() + "の【" + damageInfo.specialSkillName + "】";
                            effectMessageText = "（" + damageInfo.specialSkillEffectMessage + "）";
                            foundIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    
                    // 見つからなかった場合、現在実行中のターンを使用
                    if (foundIndex == -1) {
                        int displayTurnIndex = currentExecutingTurn;
                        if (displayTurnIndex >= static_cast<int>(pendingDamages.size()) && !pendingDamages.empty()) {
                            displayTurnIndex = 0;
                        }
                        
                        if (displayTurnIndex < static_cast<int>(pendingDamages.size())) {
                            const auto& damageInfo = pendingDamages[displayTurnIndex];
                            
                            if (damageInfo.isSpecialSkill && !damageInfo.specialSkillName.empty()) {
                                // 特殊技の場合は「敵名の【特殊技名】」を表示
                                skillNameText = enemy->getTypeName() + "の【" + damageInfo.specialSkillName + "】";
                                // 効果メッセージがある場合は別行で表示
                                if (!damageInfo.specialSkillEffectMessage.empty()) {
                                    effectMessageText = "（" + damageInfo.specialSkillEffectMessage + "）";
                                }
                            } else {
                                // 通常攻撃
                                skillNameText = enemy->getTypeName() + "の攻撃！";
                            }
                        } else {
                            // デフォルトメッセージ
                            skillNameText = enemy->getTypeName() + "の攻撃！";
                        }
                    }
                    
                    // 特殊技名を表示
                    if (!skillNameText.empty()) {
                        SDL_Texture* skillNameTexture = graphics.createTextTexture(skillNameText, "default", attackTextConfig.color);
                        if (skillNameTexture) {
                            int skillNameTextWidth, skillNameTextHeight;
                            SDL_QueryTexture(skillNameTexture, nullptr, nullptr, &skillNameTextWidth, &skillNameTextHeight);
                            
                            // 「〜ターン分の攻撃を実行」の下に配置
                            int skillNameY = totalAttackY + totalAttackTextHeight + static_cast<int>(attackTextConfig.position.offsetY);
                            
                            int attackPadding = attackTextConfig.padding;
                            int skillNameBgX = centerX - skillNameTextWidth / 2 - attackPadding;
                            int skillNameBgY = skillNameY - attackPadding;
                            
                            // 背景黒
                            graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                            graphics.drawRect(skillNameBgX, skillNameBgY, skillNameTextWidth + attackPadding * 2, skillNameTextHeight + attackPadding * 2, true);
                            
                            // テキスト白
                            graphics.drawText(skillNameText, centerX - skillNameTextWidth / 2, skillNameY, "default", attackTextConfig.color);
                            
                            SDL_DestroyTexture(skillNameTexture);
                            
                            // 効果メッセージがある場合は、特殊技名の下に表示
                            if (!effectMessageText.empty()) {
                                SDL_Texture* effectMessageTexture = graphics.createTextTexture(effectMessageText, "default", attackTextConfig.color);
                                if (effectMessageTexture) {
                                    int effectMessageTextWidth, effectMessageTextHeight;
                                    SDL_QueryTexture(effectMessageTexture, nullptr, nullptr, &effectMessageTextWidth, &effectMessageTextHeight);
                                    
                                    // 特殊技名の下に配置
                                    int effectMessageY = skillNameY + skillNameTextHeight + attackPadding;
                                    
                                    int effectMessageBgX = centerX - effectMessageTextWidth / 2 - attackPadding;
                                    int effectMessageBgY = effectMessageY - attackPadding;
                                    
                                    // 背景黒
                                    graphics.setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                                    graphics.drawRect(effectMessageBgX, effectMessageBgY, effectMessageTextWidth + attackPadding * 2, effectMessageTextHeight + attackPadding * 2, true);
                                    
                                    // テキスト白
                                    graphics.drawText(effectMessageText, centerX - effectMessageTextWidth / 2, effectMessageY, "default", attackTextConfig.color);
                                    
                                    SDL_DestroyTexture(effectMessageTexture);
                                }
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
    
    // 敵が勝った場合、特殊技の効果メッセージを事前に設定（UIで同時に表示するため）
    if (stats.enemyWins > stats.playerWins) {
        for (size_t i = 0; i < pendingDamages.size(); i++) {
            auto& damageInfo = pendingDamages[i];
            if (damageInfo.isSpecialSkill && !damageInfo.specialSkillName.empty() && damageInfo.specialSkillEffectMessage.empty()) {
                // 効果メッセージが空の場合、事前に取得して設定（副作用なし）
                std::string effectMessage = getEnemySpecialSkillEffectMessage(enemy->getType());
                damageInfo.specialSkillEffectMessage = effectMessage;
            }
        }
    }
    
    // 呪文で勝利したターンのダメージエントリを削除（後でプレイヤーが選択した呪文を実行するため）
    // ただし、攻撃呪文の場合はダメージエントリを残す（既に追加されている）
    // ここでは、呪文で勝利したターンは記録だけして、ダメージエントリは削除しない
    // （攻撃呪文の場合は既にダメージエントリが追加されているため）
}

void BattleState::executeWinningTurns(float damageMultiplier) {
    auto stats = battleLogic->getStats();
    auto playerCmds = battleLogic->getPlayerCommands();
    auto enemyCmds = battleLogic->getEnemyCommands();
    
    // JSONからプレイヤーと敵の位置を取得
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    int screenWidth = BattleConstants::SCREEN_WIDTH;
    int screenHeight = BattleConstants::SCREEN_HEIGHT;
    
    int playerX, playerY, enemyX, enemyY;
    config.calculatePosition(playerX, playerY, battleConfig.playerPosition, screenWidth, screenHeight);
    config.calculatePosition(enemyX, enemyY, battleConfig.enemyPosition, screenWidth, screenHeight);
    
    if (stats.playerWins > stats.enemyWins) {
        for (int i = 0; i < battleLogic->getCommandTurnCount(); i++) {
            if (battleLogic->judgeRound(playerCmds[i], enemyCmds[i]) == 1) {
                int cmd = playerCmds[i];
                if (cmd == 0) {
                    int baseDamage = player->calculateDamageWithBonus(*enemy);
                    int damage = static_cast<int>(baseDamage * damageMultiplier);
                    damage = applyPlayerAttackSkillEffects(damage);
                    enemy->takeDamage(damage);
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
                        damage = applyPlayerAttackSkillEffects(damage);
                        enemy->takeDamage(damage);
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
                    effectManager->triggerHitEffect(playerDamage, playerX, playerY, true);
                    effectManager->triggerScreenShake(10.0f, 0.3f, false, true);
                }
                if (enemyDamage > 0) {
                    enemy->takeDamage(enemyDamage);
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
        // サブフェーズが変更された時（judgeDisplayTimerが0の時）に効果音を再生
        static JudgeSubPhase lastJudgeSubPhase = JudgeSubPhase::SHOW_RESULT; // 初期値をSHOW_RESULTに設定（最初のSHOW_PLAYER_COMMANDを検出するため）
        if (judgeDisplayTimer == 0.0f) {
            if (judgeSubPhase == JudgeSubPhase::SHOW_PLAYER_COMMAND || 
                judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
                // コマンドが表示される時にcommand.oggを再生（サブフェーズが変更された時のみ）
                if (lastJudgeSubPhase != judgeSubPhase) {
                    AudioManager::getInstance().playSound("command", 0);
                }
            } else if (judgeSubPhase == JudgeSubPhase::SHOW_RESULT) {
                // 勝敗結果が表示される時にresult.oggを再生（サブフェーズが変更された時のみ）
                if (lastJudgeSubPhase != judgeSubPhase) {
                    AudioManager::getInstance().playSound("result", 0);
                }
            }
            lastJudgeSubPhase = judgeSubPhase;
        }
        
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
                            // ただし、「助けを呼ぶ」の場合は即座にゲームオーバー
                            if (currentResidentCommand == BattleConstants::RESIDENT_COMMAND_CALL_HELP &&
                                currentResidentPlayerCommand == BattleConstants::COMMAND_ATTACK) {
                                // 攻撃 + 助けを呼ぶ → 即座にゲームオーバー
                                if (stateManager) {
                                    stateManager->changeState(std::make_unique<GameOverState>(
                                        player, "衛兵に見つかりました。", enemy->getType(), enemy->getLevel(),
                                        true, enemy->getName(), enemy->getResidentX(), enemy->getResidentY(), enemy->getResidentTextureIndex()));
                                }
                                return;
                            }
                            
                            // 「怯える」かつ「攻撃」の場合は即座にprocessResidentTurnを呼ぶ
                            if (currentResidentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID &&
                                currentResidentPlayerCommand == BattleConstants::COMMAND_ATTACK) {
                                // 結果フェーズに遷移してから即座に処理を開始
                                currentPhase = BattlePhase::JUDGE_RESULT;
                                judgeDisplayTimer = 0.0f;
                                phaseTimer = 0.0f;
                                // 即座にprocessResidentTurnを呼ぶ（2秒待たない）
                                processResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
                                // processResidentTurnでGameOverStateに遷移した場合は、ここで処理を終了
                                if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                                    return;
                                }
                                // アニメーションタイマーをリセット（アニメーションを最初から開始するため）
                                animationController->resetResultAnimation();
                            } else {
                                // その他の場合は通常通り結果フェーズに遷移
                        currentPhase = BattlePhase::JUDGE_RESULT;
                        judgeDisplayTimer = 0.0f;
                        phaseTimer = 0.0f;  // 住民戦の場合もphaseTimerをリセット
                            }
                        }
                    } else {
                        currentJudgingTurnIndex++;
                        judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                        judgeDisplayTimer = 0.0f;
                        // 次のターンのコマンド表示時にcommand.oggを再生
                        AudioManager::getInstance().playSound("command", 0);
                        
                        if (currentJudgingTurnIndex >= battleLogic->getCommandTurnCount()) {
                            // 現在のフェーズに応じて適切な結果フェーズに遷移
                            if (currentPhase == BattlePhase::DESPERATE_JUDGE) {
                                currentPhase = BattlePhase::DESPERATE_JUDGE_RESULT;
                            } else if (currentPhase == BattlePhase::LAST_CHANCE_JUDGE) {
                                currentPhase = BattlePhase::LAST_CHANCE_JUDGE_RESULT;
                            } else {
                                currentPhase = BattlePhase::JUDGE_RESULT;
                            }
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
    
    // JSONからプレイヤーと敵の位置を取得
    auto& uiConfigManager = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = uiConfigManager.getBattleConfig();
    int screenWidth = BattleConstants::SCREEN_WIDTH;
    int screenHeight = BattleConstants::SCREEN_HEIGHT;
    
    int playerX, playerY, enemyX, enemyY;
    uiConfigManager.calculatePosition(playerX, playerY, battleConfig.playerPosition, screenWidth, screenHeight);
    uiConfigManager.calculatePosition(enemyX, enemyY, battleConfig.enemyPosition, screenWidth, screenHeight);
    
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
        
        // 「怯える」かつ「攻撃」の場合は既にジャッジフェーズでprocessResidentTurnが呼ばれているので、
        // ここでは2秒待つ処理をスキップする
        // それ以外の場合（身を隠すなど）でpendingDamagesが空の場合は2秒待つ
        if (pendingDamages.empty() && phaseTimer < 2.0f && !residentAttackFailed) {
            // 「怯える」かつ「攻撃」の場合は既に処理済みなのでスキップ
            if (currentResidentCommand == BattleConstants::RESIDENT_COMMAND_AFRAID &&
                currentResidentPlayerCommand == BattleConstants::COMMAND_ATTACK) {
                // 既にprocessResidentTurnが呼ばれているはずなので、何もしない
            } else {
            phaseTimer += deltaTime;
            // 2秒後に実際の処理に移行
            if (phaseTimer >= 2.0f) {
                processResidentTurn(currentResidentPlayerCommand, currentResidentCommand);
                    // processResidentTurnでGameOverStateに遷移した場合は、ここで処理を終了
                    if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                        return;
                    }
                // processResidentTurnでpendingDamagesに追加された後、すぐにアニメーション処理に進む
                // phaseTimerをリセットして、アニメーション処理を確実に開始する
                phaseTimer = 0.0f;
                    // アニメーションタイマーもリセット（重要：アニメーションを最初から開始するため）
                    animationController->resetResultAnimation();
                // returnしない（通常の戦闘と同じ処理に進む）
            } else {
                return;  // 2秒経過するまではreturn
                }
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
    
    // 住民戦の場合は専用の処理（1ターンしかないため）
    if (enemy->isResident() && !pendingDamages.empty()) {
        // アニメーション更新
        animationController->updateResultAnimation(deltaTime, isDesperateMode);
        auto& resultState = animationController->getResultState();
        
        // アニメーションタイマーが初期化されていない場合（初回のみ）、リセット
        // これはprocessResidentTurnが呼ばれた直後の初回フレームで実行される
        if (resultState.resultAnimationTimer == 0.0f && currentExecutingTurn == 0 && !damageAppliedInAnimation) {
            animationController->resetResultAnimation();
        }
        
        // 住民戦は1ターンしかないので、currentExecutingTurnは常に0
        if (currentExecutingTurn == 0) {
            auto& damageInfo = pendingDamages[0];
            
            // アニメーションをスキップする場合は、ダメージ適用処理もスキップ
            if (!damageInfo.skipAnimation) {
                // カウンターラッシュの場合はアニメーション時間を短縮
                bool isCounterRush = damageInfo.isCounterRush;
                float animStartTime = isCounterRush ? 0.05f : 0.5f;
                float animDuration = isCounterRush ? 0.15f : 1.0f;
                
                // アニメーション更新（現在のダメージ用）
                bool isVictory = !pendingDamages.empty();
                animationController->updateResultCharacterAnimation(
                    deltaTime, isVictory, false, resultState.resultAnimationTimer,
                    damageAppliedInAnimation, false,
                    animStartTime, animDuration);
                
                // アニメーションのピーク時（攻撃が当たる瞬間）にダメージを適用
                float animTime = resultState.resultAnimationTimer - animStartTime;
                
                if (animTime >= 0.0f && animTime <= animDuration) {
                    float progress = animTime / animDuration;
                    // ピーク時（progress = 0.2f付近）にダメージを適用
                    if (progress >= BattleConstants::ATTACK_ANIMATION_HIT_MOMENT - BattleConstants::ATTACK_ANIMATION_HIT_WINDOW &&
                        progress <= BattleConstants::ATTACK_ANIMATION_HIT_MOMENT + BattleConstants::ATTACK_ANIMATION_HIT_WINDOW &&
                        !damageAppliedInAnimation) {
                        
                        int damage = damageInfo.damage;
                        bool isPlayerHit = damageInfo.isPlayerHit;
                        
                        // ダミーのダメージエントリ（damage == 0）の場合はダメージ適用をスキップ
                        if (damage > 0) {
                            // 攻撃アニメーションが当たる瞬間に効果音を再生
                            // 防御で勝った場合（カウンターラッシュ）はrush.ogg、それ以外はattack.ogg
                            if (damageInfo.commandType == BattleConstants::COMMAND_DEFEND || damageInfo.isCounterRush) {
                                AudioManager::getInstance().playSound("attack", 0);
                            } else {
                                AudioManager::getInstance().playSound("attack", 0);
                            }
                            
                            if (isPlayerHit) {
                                // 特殊技の場合は特殊技を適用
                                if (damageInfo.isSpecialSkill && !damageInfo.specialSkillName.empty()) {
                                    std::string effectMessage = "";
                                    damage = applyEnemySpecialSkill(enemy->getType(), damage, effectMessage);
                                    // 効果メッセージをDamageInfoに保存
                                    pendingDamages[currentExecutingTurn].specialSkillEffectMessage = effectMessage;
                                    std::string msg = enemy->getTypeName() + "の【" + damageInfo.specialSkillName + "】！\n";
                                    addBattleLog(msg);
                                } else {
                                    std::string msg = enemy->getTypeName() + "の攻撃！\n";
                                    addBattleLog(msg);
                                }
                                
                                player->takeDamage(damage);
                                effectManager->triggerHitEffect(damage, playerX, playerY, true);
                                float intensity = BattleConstants::DRAW_SHAKE_INTENSITY;
                                float shakeDuration = 0.3f;
                                effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                                
                                std::string damageMsg = player->getName() + "は" + std::to_string(damage) + "のダメージを受けた！";
                                addBattleLog(damageMsg);
                            } else {
                                enemy->takeDamage(damage);
                                effectManager->triggerHitEffect(damage, enemyX, enemyY, false);
                                
                                // 通常攻撃の場合、ステータスアップ効果を切る
                                if (damageInfo.commandType == BattleConstants::COMMAND_ATTACK || 
                                    damageInfo.commandType == BattleConstants::COMMAND_SPELL) {
                                    if (player->hasNextTurnBonusActive()) {
                                        player->clearNextTurnBonus();
                                    }
                                }
                                
                                float intensity = BattleConstants::DRAW_SHAKE_INTENSITY;
                                float shakeDuration = 0.3f;
                                effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                            }
                        }
                        
                        damageAppliedInAnimation = true;
                    }
                }
                
                // アニメーション完了後、次の処理へ進む
                float animTotalDuration = animStartTime + animDuration;
                if (resultState.resultAnimationTimer >= animTotalDuration) {
                    // アニメーション完了
                    currentExecutingTurn = 1; // 完了フラグとして1に設定（pendingDamages.size()と同じ値にする）
                    damageAppliedInAnimation = false;
                }
            } else {
                // アニメーションをスキップする場合、すぐに完了とする
                currentExecutingTurn = 1;
                damageAppliedInAnimation = false;
            }
        }
        
        // 最初のアニメーション開始時のスクリーンシェイク（勝利）
        float shakeDuration = 0.3f;
        if (resultState.resultAnimationTimer < shakeDuration && !resultState.resultShakeActive && currentExecutingTurn == 0) {
            resultState.resultShakeActive = true;
            float intensity = BattleConstants::VICTORY_SHAKE_INTENSITY;
            effectManager->triggerScreenShake(intensity, shakeDuration, true, false);
        }
        
        // 全ダメージ適用完了後、戦闘終了判定
        if (currentExecutingTurn >= static_cast<int>(pendingDamages.size())) {
            // 住民戦：アニメーション完了後、1秒待機
            float finalWaitDuration = 0.5f + 1.0f; // 1.5秒
            float maxWaitTime = 2.0f;
            
            if (resultState.resultAnimationTimer >= finalWaitDuration || phaseTimer >= maxWaitTime) {
                updateStatus();
                
                // 住民戦の場合は、3回攻撃が当たったかどうかで判定
                if (enemy->isResident()) {
                    if (residentHitCount >= 3) {
                        // 3回攻撃が当たったら勝利
                        enemy->setHp(0);
                        checkBattleEnd();
                        if (currentPhase != BattlePhase::JUDGE_RESULT && 
                            currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT &&
                            currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
                            return;
                        }
                    } else if (player->getIsAlive()) {
                        // まだ3回に達していない場合は次のターンへ
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
                } else if (enemy->getIsAlive() && player->getIsAlive()) {
                    // 通常戦の場合は次のターンへ
                    if (isDesperateMode) {
                        battleLogic->setDesperateMode(false);
                        battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                    }
                    currentPhase = BattlePhase::COMMAND_SELECT;
                    initializeCommandSelection();
                } else {
                    checkBattleEnd();
                    if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                        return;
                    }
                }
                
                // リセット
                animationController->resetResultAnimation();
                currentExecutingTurn = 0;
                executeDelayTimer = 0.0f;
                damageAppliedInAnimation = false;
                pendingDamages.clear();
                phaseTimer = 0;
            }
        }
        
        return; // 住民戦の処理が完了したら、通常戦の処理をスキップ
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
        float animStartTime = isCounterRush ? 0.05f : 0.5f;
        float animDuration = isCounterRush ? 0.15f : 1.0f;
        
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
            float animStartTime = isCounterRush ? 0.05f : 0.5f;
            float animDuration = isCounterRush ? 0.15f : 1.0f;
        
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
                    // 攻撃アニメーションが当たる瞬間にattack.oggを再生
                    if (damageInfo.playerDamage > 0 || damageInfo.enemyDamage > 0) {
                        AudioManager::getInstance().playSound("attack", 0);
                    }
                    
                    if (damageInfo.playerDamage > 0) {
                        player->takeDamage(damageInfo.playerDamage);
                        effectManager->triggerHitEffect(damageInfo.playerDamage, playerX, playerY, true);
                    }
                    if (damageInfo.enemyDamage > 0) {
                        enemy->takeDamage(damageInfo.enemyDamage);
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
                        // 攻撃アニメーションが当たる瞬間に効果音を再生
                        // 防御で勝った場合（カウンターラッシュ）はrush.ogg、それ以外はattack.ogg
                        if (damageInfo.commandType == BattleConstants::COMMAND_DEFEND || damageInfo.isCounterRush) {
                            AudioManager::getInstance().playSound("attack", 0);
                        } else {
                            AudioManager::getInstance().playSound("attack", 0);
                        }
                        
                        if (isPlayerHit) {
                            player->takeDamage(damage);
                            effectManager->triggerHitEffect(damage, playerX, playerY, true);
                            // 引き分けと同じスクリーンシェイク
                            float intensity = isDesperateMode ? BattleConstants::DESPERATE_DRAW_SHAKE_INTENSITY : BattleConstants::DRAW_SHAKE_INTENSITY;
                            float shakeDuration = isDesperateMode ? 0.5f : 0.3f;
                            effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                        } else {
                            enemy->takeDamage(damage);
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
            
            // HPが0になった場合、最後のチャンスモードに遷移する可能性があるため、checkBattleEnd()を呼ぶ
            if (!player->getIsAlive() && !hasUsedLastChanceMode) {
                checkBattleEnd();
                // フェーズが変更された場合は処理を終了（リセット処理は行わない）
                if (currentPhase != BattlePhase::JUDGE_RESULT && 
                    currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT &&
                    currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
                    return;
                }
            }
            
            // 住民戦の場合は、3回攻撃が当たったかどうかで判定
            if (enemy->isResident()) {
                if (residentHitCount >= 3) {
                    // 3回攻撃が当たったら勝利
                    enemy->setHp(0);
                    checkBattleEnd();
                    // checkBattleEnd()でフェーズが変更された場合は、ここで処理を終了（リセット処理は行わない）
                    if (currentPhase != BattlePhase::JUDGE_RESULT && currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT) {
                        return;
                    }
                } else if (player->getIsAlive()) {
                    // まだ3回に達していない場合は次のターンへ
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
            } else if (enemy->getIsAlive() && player->getIsAlive()) {
                // 通常戦の場合は次のターンへ
                if (isDesperateMode) {
                    battleLogic->setDesperateMode(false);
                    battleLogic->setCommandTurnCount(BattleConstants::NORMAL_TURN_COUNT);
                }
                currentPhase = BattlePhase::COMMAND_SELECT;
                initializeCommandSelection();
            } else {
                checkBattleEnd();
                // checkBattleEnd()でフェーズが変更された場合は、ここで処理を終了（リセット処理は行わない）
                if (currentPhase != BattlePhase::JUDGE_RESULT && 
                    currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT &&
                    currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
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
            if (currentPhase != BattlePhase::JUDGE_RESULT && 
                currentPhase != BattlePhase::DESPERATE_JUDGE_RESULT &&
                currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
                return;
            }
        }
    }
}

int BattleState::applyEnemySpecialSkill(EnemyType enemyType, int baseDamage, std::string& effectMessage) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> percentDis(0, 99);
    std::uniform_int_distribution<> chaosDis(0, 2);
    
    auto& skillEffects = player->getPlayerStats().getEnemySkillEffects();
    int finalDamage = baseDamage;
    effectMessage = ""; // 初期化
    
    switch (enemyType) {
        case EnemyType::SLIME:
            // 通常攻撃 + 次のプレイヤーの攻撃を50%軽減
            skillEffects.playerAttackReduction = 0.5f;
            effectMessage = "次の攻撃50%軽減";
            addBattleLog("粘液が体を覆った！次の攻撃が弱くなる！");
            break;
            
        case EnemyType::GOBLIN:
            // 通常ダメージの1.2倍
            finalDamage = static_cast<int>(baseDamage * 1.2f);
            effectMessage = "ダメージ1.2倍";
            break;
            
        case EnemyType::ORC:
            // HP50%以下なら通常ダメージの1.5倍、それ以上なら1.2倍
            if (enemy->getHp() <= enemy->getMaxHp() * 0.5f) {
                finalDamage = static_cast<int>(baseDamage * 1.5f);
                effectMessage = "ダメージ1.5倍（HP50%以下）";
                addBattleLog("怒りで力が増した！");
            } else {
                finalDamage = static_cast<int>(baseDamage * 1.2f);
                effectMessage = "ダメージ1.2倍";
            }
            break;
            
        case EnemyType::DRAGON:
            // 通常ダメージ + 火傷状態（3ターン、毎ターンHPの3%を削る）
            skillEffects.burnTurns = 3;
            skillEffects.burnDamageRatio = 0.03f;
            effectMessage = "火傷状態（3ターン、毎ターンHP3%減少）";
            addBattleLog("炎のブレスが体を焼いた！火傷状態になった！");
            break;
            
        case EnemyType::SKELETON:
            // 通常ダメージ + 次攻撃を受けた時にそのダメージの30%をプレイヤーに与える
            skillEffects.hasCounterOnNextAttack = true;
            skillEffects.counterDamageRatio = 0.3f;
            effectMessage = "次攻撃30%カウンター";
            addBattleLog("骨の壁が構築された！次の攻撃が跳ね返される！");
            break;
            
        case EnemyType::GHOST:
            // 通常 + 次のプレイヤー攻撃の失敗率を50%向上
            skillEffects.attackFailureRateIncrease = 0.5f;
            effectMessage = "次の攻撃失敗率50%向上";
            addBattleLog("呪いの触手が体を絡みついた！攻撃が当たりにくくなった！");
            break;
            
        case EnemyType::VAMPIRE:
            // 通常 + 与えたダメージの20%回復
            {
                int healAmount = static_cast<int>(baseDamage * 0.2f);
                enemy->heal(healAmount);
                effectMessage = "与えたダメージの20%回復";
                addBattleLog("血を吸い取られた！" + enemy->getTypeName() + "は" + std::to_string(healAmount) + "回復した！");
            }
            break;
            
        case EnemyType::DEMON_SOLDIER:
            // 通常 + 5%の確率でプレイヤーを即死させる
            if (percentDis(gen) < 5) {
                player->takeDamage(player->getHp());
                effectMessage = "5%確率で即死（発動）";
                addBattleLog("地獄の一撃が直撃した！即死した！");
                return baseDamage; // 既にダメージを適用したので、ここで終了
            }
            effectMessage = "5%確率で即死（発動せず）";
            break;
            
        case EnemyType::WEREWOLF:
            // 通常攻撃 + 裂傷（3ターン、プレイヤーのHPの5%を継続的に減らす）
            skillEffects.bleedTurns = 3;
            skillEffects.bleedDamageRatio = 0.05f;
            effectMessage = "裂傷（3ターン、毎ターンHP5%減少）";
            addBattleLog("猛襲で裂傷を負った！");
            break;
            
        case EnemyType::MINOTAUR:
            // 通常の1.5倍
            finalDamage = static_cast<int>(baseDamage * 1.5f);
            effectMessage = "ダメージ1.5倍";
            break;
            
        case EnemyType::CYCLOPS:
            // 通常の1.75倍
            finalDamage = static_cast<int>(baseDamage * 1.75f);
            effectMessage = "ダメージ1.75倍";
            break;
            
        case EnemyType::GARGOYLE:
            // 通常 + 次に受けるダメージを80%低減
            skillEffects.damageReduction = 0.8f;
            effectMessage = "次に受けるダメージ80%軽減";
            addBattleLog("石の守りが発動した！次に受けるダメージが大幅に減る！");
            break;
            
        case EnemyType::PHANTOM:
            // 通常 + 次の攻撃を80%で回避できる
            skillEffects.hasEvasion = true;
            skillEffects.evasionRate = 0.8f;
            effectMessage = "次の攻撃80%回避";
            addBattleLog("幻影の攻撃で体が透けた！次の攻撃を回避できる！");
            break;
            
        case EnemyType::DARK_KNIGHT:
            // 通常 + 次にプレイヤーに攻撃された時にHPの10%を削る
            skillEffects.hasDarkKnightCounter = true;
            effectMessage = "次に攻撃された時HP10%削減";
            addBattleLog("暗黒の一撃が体を貫いた！次の攻撃が跳ね返される！");
            break;
            
        case EnemyType::ICE_GIANT:
            // 通常 + 20%の確率で氷漬けにして次のターンコマンド選択できず一方的に攻撃される
            if (percentDis(gen) < 20) {
                skillEffects.isFrozen = true;
                effectMessage = "20%確率で氷漬け（発動）";
                addBattleLog("氷結の息吹で体が凍りついた！次のターン動けない！");
            } else {
                effectMessage = "20%確率で氷漬け（発動せず）";
            }
            break;
            
        case EnemyType::FIRE_DEMON:
            // 通常 + 2ターンHPの10%を削る
            skillEffects.burnTurns = 2;
            skillEffects.burnDamageRatio = 0.1f;
            effectMessage = "火傷状態（2ターン、毎ターンHP10%減少）";
            addBattleLog("業火が体を焼いた！継続ダメージを受ける！");
            break;
            
        case EnemyType::SHADOW_LORD:
            // 通常 + 次の攻撃を無効化
            skillEffects.hasShadowLordBlock = true;
            effectMessage = "次の攻撃無効化";
            addBattleLog("影の呪縛で次の攻撃が無効化される！");
            break;
            
        case EnemyType::ANCIENT_DRAGON:
            // 通常 + 1ターンプレイヤーの攻撃力を50%下げる
            skillEffects.attackReduction = 0.5f;
            skillEffects.attackReductionTurns = 1;
            effectMessage = "1ターン攻撃力50%低下";
            addBattleLog("古の咆哮で攻撃力が半減した！");
            break;
            
        case EnemyType::CHAOS_BEAST:
            // 通常 + 1ターン自分の防御力を0にする代わりに攻撃力が999になる
            skillEffects.hasChaosBeastEffect = true;
            skillEffects.chaosBeastDefense = 0;
            skillEffects.chaosBeastAttack = 999;
            skillEffects.chaosBeastTurns = 1;
            effectMessage = "1ターン防御力0、攻撃力999";
            addBattleLog("混沌の力で防御力が0になり、攻撃力が999になった！");
            break;
            
        case EnemyType::ELDER_GOD: {
            // 通常なしでプレイヤーのHPを1にする
            int currentHp = player->getHp();
            if (currentHp > 1) {
                player->takeDamage(currentHp - 1);
                effectMessage = "HPを1にする";
                addBattleLog("神の裁きでHPが1になった！");
            }
            return 0; // 通常ダメージはなし
        }
            
        case EnemyType::GOBLIN_KING:
            // 通常ダメージの1.3倍
            finalDamage = static_cast<int>(baseDamage * 1.3f);
            effectMessage = "ダメージ1.3倍";
            break;
            
        case EnemyType::ORC_LORD:
            // HP50%以下なら通常ダメージの1.8倍、それ以上なら1.3倍
            if (enemy->getHp() <= enemy->getMaxHp() * 0.5f) {
                finalDamage = static_cast<int>(baseDamage * 1.8f);
                effectMessage = "ダメージ1.8倍（HP50%以下）";
            } else {
                finalDamage = static_cast<int>(baseDamage * 1.3f);
                effectMessage = "ダメージ1.3倍";
            }
            break;
            
        case EnemyType::DRAGON_LORD:
            // 通常ダメージ + 火傷状態（5ターン、毎ターンHPの5%を削る）
            skillEffects.burnTurns = 5;
            skillEffects.burnDamageRatio = 0.05f;
            effectMessage = "火傷状態（5ターン、毎ターンHP5%減少）";
            addBattleLog("ドラゴンの炎が体を焼いた！強力な火傷状態になった！");
            break;
            
        case EnemyType::GUARD:
            // 通常ダメージの1.15倍
            finalDamage = static_cast<int>(baseDamage * 1.15f);
            effectMessage = "ダメージ1.15倍";
            break;
            
        case EnemyType::KING:
            // 通常ダメージの1.2倍 + 次のプレイヤーの攻撃を30%軽減
            finalDamage = static_cast<int>(baseDamage * 1.2f);
            skillEffects.playerAttackReduction = 0.3f;
            effectMessage = "ダメージ1.2倍、次の攻撃30%軽減";
            addBattleLog("王の威厳が体を覆った！次の攻撃が弱くなる！");
            break;
            
        case EnemyType::DEMON_LORD:
            // 通常ダメージ（魔王の特殊技は別途実装可能）
            break;
            
        default:
            break;
    }
    
    return finalDamage;
}

std::string BattleState::getEnemySpecialSkillEffectMessage(EnemyType enemyType) {
    // 効果メッセージのみを取得（副作用なし）
    switch (enemyType) {
        case EnemyType::SLIME:
            return "次の攻撃50%軽減";
        case EnemyType::GOBLIN:
            return "ダメージ1.2倍";
        case EnemyType::ORC:
            // HP50%以下かどうかは実際の適用時に判定するため、ここでは一般的なメッセージを返す
            return "ダメージ1.5倍（HP50%以下）または1.2倍";
        case EnemyType::DRAGON:
            return "火傷状態（3ターン、毎ターンHP3%減少）";
        case EnemyType::SKELETON:
            return "次攻撃30%カウンター";
        case EnemyType::GHOST:
            return "次の攻撃失敗率50%向上";
        case EnemyType::VAMPIRE:
            return "与えたダメージの20%回復";
        case EnemyType::DEMON_SOLDIER:
            return "5%確率で即死";
        case EnemyType::WEREWOLF:
            return "裂傷（3ターン、毎ターンHP5%減少）";
        case EnemyType::MINOTAUR:
            return "ダメージ1.5倍";
        case EnemyType::CYCLOPS:
            return "ダメージ1.75倍";
        case EnemyType::GARGOYLE:
            return "次に受けるダメージ80%軽減";
        case EnemyType::PHANTOM:
            return "次の攻撃80%回避";
        case EnemyType::DARK_KNIGHT:
            return "次に攻撃された時HP10%削減";
        case EnemyType::ICE_GIANT:
            return "20%確率で氷漬け";
        case EnemyType::FIRE_DEMON:
            return "火傷状態（2ターン、毎ターンHP10%減少）";
        case EnemyType::SHADOW_LORD:
            return "次の攻撃無効化";
        case EnemyType::ANCIENT_DRAGON:
            return "1ターン攻撃力50%低下";
        case EnemyType::CHAOS_BEAST:
            return "1ターン防御力0、攻撃力999";
        case EnemyType::ELDER_GOD:
            return "HPを1にする";
        case EnemyType::GOBLIN_KING:
            return "ダメージ1.3倍";
        case EnemyType::ORC_LORD:
            // HP50%以下かどうかは実際の適用時に判定するため、ここでは一般的なメッセージを返す
            return "ダメージ1.8倍（HP50%以下）または1.3倍";
        case EnemyType::DRAGON_LORD:
            return "火傷状態（5ターン、毎ターンHP5%減少）";
        case EnemyType::GUARD:
            return "ダメージ1.15倍";
        case EnemyType::KING:
            return "ダメージ1.2倍、次の攻撃30%軽減";
        default:
            return "";
    }
}

void BattleState::processEnemySkillEffects() {
    auto& skillEffects = player->getPlayerStats().getEnemySkillEffects();
    
    // 火傷の処理
    if (skillEffects.burnTurns > 0) {
        int burnDamage = static_cast<int>(player->getMaxHp() * skillEffects.burnDamageRatio);
        if (burnDamage > 0) {
            player->takeDamage(burnDamage);
            addBattleLog("火傷で" + std::to_string(burnDamage) + "のダメージを受けた！");
        }
        skillEffects.burnTurns--;
        if (skillEffects.burnTurns <= 0) {
            skillEffects.burnDamageRatio = 0.0f;
        }
    }
    
    // 裂傷の処理
    if (skillEffects.bleedTurns > 0) {
        int bleedDamage = static_cast<int>(player->getMaxHp() * skillEffects.bleedDamageRatio);
        if (bleedDamage > 0) {
            player->takeDamage(bleedDamage);
            addBattleLog("裂傷で" + std::to_string(bleedDamage) + "のダメージを受けた！");
        }
        skillEffects.bleedTurns--;
        if (skillEffects.bleedTurns <= 0) {
            skillEffects.bleedDamageRatio = 0.0f;
        }
    }
    
    // 攻撃力減少の処理
    if (skillEffects.attackReductionTurns > 0) {
        skillEffects.attackReductionTurns--;
        if (skillEffects.attackReductionTurns <= 0) {
            skillEffects.attackReduction = 0.0f;
            addBattleLog("攻撃力が元に戻った！");
        }
    }
    
    // カオスビースト効果の処理
    if (skillEffects.chaosBeastTurns > 0) {
        skillEffects.chaosBeastTurns--;
        if (skillEffects.chaosBeastTurns <= 0) {
            skillEffects.hasChaosBeastEffect = false;
            skillEffects.chaosBeastDefense = 0;
            skillEffects.chaosBeastAttack = 0;
            addBattleLog("混沌の力が消えた！");
        }
    }
    
    // 氷漬けの処理（次のターンで解除）
    if (skillEffects.isFrozen) {
        skillEffects.isFrozen = false;
    }
    
    // シャドウロードのブロック効果（1回のみ）
    if (skillEffects.hasShadowLordBlock) {
        skillEffects.hasShadowLordBlock = false;
    }
}

int BattleState::applyPlayerAttackSkillEffects(int baseDamage) {
    auto& skillEffects = player->getPlayerStats().getEnemySkillEffects();
    int finalDamage = baseDamage;
    
    // スライムの攻撃ダメージ軽減
    if (skillEffects.playerAttackReduction > 0.0f) {
        finalDamage = static_cast<int>(finalDamage * (1.0f - skillEffects.playerAttackReduction));
        skillEffects.playerAttackReduction = 0.0f; // 1回のみ
    }
    
    // エンシェントドラゴンの攻撃力減少
    if (skillEffects.attackReduction > 0.0f) {
        finalDamage = static_cast<int>(finalDamage * (1.0f - skillEffects.attackReduction));
    }
    
    // カオスビーストの防御力0、攻撃力999
    if (skillEffects.hasChaosBeastEffect) {
        // 防御力0なので、ダメージ計算を変更
        // 通常のダメージ計算ではなく、攻撃力999で計算
        // これは特殊なので、ここでは通常のダメージ計算を維持
        // 実際の防御力は敵側で処理する必要がある
    }
    
    return finalDamage;
}

void BattleState::updateLastChanceJudgeResultPhase(float deltaTime) {
    // フェーズが既に変更されている場合は処理をスキップ
    if (currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
        return;
    }
    
    // JSONからプレイヤーと敵の位置を取得
    auto& uiConfigManager = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = uiConfigManager.getBattleConfig();
    int screenWidth = BattleConstants::SCREEN_WIDTH;
    int screenHeight = BattleConstants::SCREEN_HEIGHT;
    
    int playerX, playerY, enemyX, enemyY;
    uiConfigManager.calculatePosition(playerX, playerY, battleConfig.playerPosition, screenWidth, screenHeight);
    uiConfigManager.calculatePosition(enemyX, enemyY, battleConfig.enemyPosition, screenWidth, screenHeight);
    
    auto stats = battleLogic->getStats();
    int playerWins = stats.playerWins;
    
    // デバッグログ：関数の最初の状態を確認
    static bool firstCall = true;
    if (firstCall) {
        std::cout << "[DEBUG] LAST_CHANCE: updateLastChanceJudgeResultPhase()初回呼び出し - pendingDamages.size()=" 
                  << pendingDamages.size() << ", currentExecutingTurn=" << currentExecutingTurn 
                  << ", playerWins=" << playerWins << std::endl;
        firstCall = false;
    }
    
    // ダメージリストの準備（初回のみ）
    // LAST_CHANCE_JUDGE_RESULTフェーズでは、前のフェーズで設定されたpendingDamagesをクリアして、
    // 改めて5ターン分のダメージを準備する必要がある
    if (!damageListPrepared) {
        // 勝利数が3以上の場合のみ、ダメージリストを準備
        if (playerWins >= 3) {
            float multiplier = 1.5f; // 最後のチャンスモードでは1.5倍
            std::cout << "[DEBUG] LAST_CHANCE: prepareDamageList()を呼び出します。playerWins=" << playerWins 
                      << ", commandTurnCount=" << battleLogic->getCommandTurnCount() << std::endl;
            prepareDamageList(multiplier);
            std::cout << "[DEBUG] LAST_CHANCE: prepareDamageList()完了。pendingDamages.size()=" << pendingDamages.size() 
                      << ", spellWinTurns.size()=" << spellWinTurns.size() << std::endl;
            for (size_t i = 0; i < pendingDamages.size(); i++) {
                std::cout << "[DEBUG] LAST_CHANCE: pendingDamages[" << i << "] - damage=" << pendingDamages[i].damage 
                          << ", commandType=" << pendingDamages[i].commandType << std::endl;
            }
            
            // 窮地モードでは、呪文で勝利したターンも含めて、全ての勝利ターンでダメージを適用する
            // 呪文で勝利したターンについては、自動で攻撃魔法を選択してダメージを適用する
            for (int turnIndex : spellWinTurns) {
                int baseAttack = player->getTotalAttack();
                if (player->hasNextTurnBonusActive()) {
                    baseAttack = static_cast<int>(baseAttack * player->getNextTurnMultiplier());
                }
                int baseDamage = baseAttack;
                int damage = std::max(1, static_cast<int>((baseDamage - enemy->getEffectiveDefense()) * multiplier));
                
                if (damage > 0) {
                    BattleLogic::DamageInfo spellDamage;
                    spellDamage.damage = damage;
                    spellDamage.isPlayerHit = false;
                    spellDamage.isDraw = false;
                    spellDamage.playerDamage = 0;
                    spellDamage.enemyDamage = 0;
                    spellDamage.commandType = BattleConstants::COMMAND_SPELL;
                    spellDamage.isCounterRush = false;
                    spellDamage.skipAnimation = false;
                    pendingDamages.push_back(spellDamage);
                    std::cout << "[DEBUG] LAST_CHANCE: 呪文で勝利したターン" << turnIndex << "のダメージを追加。damage=" << damage << std::endl;
                }
            }
            
            std::cout << "[DEBUG] LAST_CHANCE: 呪文ダメージ追加後。pendingDamages.size()=" << pendingDamages.size() << std::endl;
            
            // prepareDamageList()は全ての勝利ターンのダメージを準備する
            // 防御で勝利した場合は5回のダメージエントリが追加されるため、
            // 全てのダメージエントリをそのまま使用する（制限しない）
            
            damageAppliedInAnimation = false;
            std::cout << "[DEBUG] LAST_CHANCE: currentExecutingTurnを0にリセット（以前の値=" << currentExecutingTurn << "）" << std::endl;
            currentExecutingTurn = 0;
            executeDelayTimer = 0.0f;
            
            // 最初のアニメーションを開始
            if (!pendingDamages.empty()) {
                std::cout << "[DEBUG] LAST_CHANCE: resetResultAnimation()を呼び出します。" << std::endl;
                animationController->resetResultAnimation();
            } else {
                std::cout << "[DEBUG] LAST_CHANCE: 警告！pendingDamagesが空です！" << std::endl;
            }
            
            addBattleLog("勝利数: " + std::to_string(playerWins) + "回！連続攻撃を実行！");
        } else {
            // 勝利数が3未満の場合、即座にゲームオーバー
            addBattleLog("勝利数が3未満です。ゲームオーバー...");
            phaseTimer += deltaTime;
            if (phaseTimer >= 2.0f) {
                // 敵から攻撃を受けてゲームオーバー
                int enemyDamage = battleLogic->calculateEnemyAttackDamage();
                player->takeDamage(enemyDamage);
                checkBattleEnd();
                return;
            }
            return;
        }
        damageListPrepared = true;
    }
    
    // アニメーション更新（結果発表アニメーション用）
    animationController->updateResultAnimation(deltaTime, false);
    auto& resultState = animationController->getResultState();
    
    // デバッグログ（最初の数フレームのみ）
    static int debugFrameCount = 0;
    if (debugFrameCount < 20) {
        std::cout << "[DEBUG] LAST_CHANCE: update() - pendingDamages.size()=" << pendingDamages.size() 
                  << ", currentExecutingTurn=" << currentExecutingTurn 
                  << ", resultAnimationTimer=" << resultState.resultAnimationTimer << std::endl;
        debugFrameCount++;
    }
    
    // 現在のダメージに対してアニメーションを実行
    if (currentExecutingTurn < pendingDamages.size()) {
        if (debugFrameCount < 10) {
            std::cout << "[DEBUG] LAST_CHANCE: アニメーション実行中 - damageInfo.skipAnimation=" 
                      << pendingDamages[currentExecutingTurn].skipAnimation << std::endl;
        }
        auto& damageInfo = pendingDamages[currentExecutingTurn];
        
        // アニメーションをスキップする場合は、アニメーション更新をスキップ
        if (!damageInfo.skipAnimation) {
            // カウンターラッシュの場合はアニメーション時間を短縮
            bool isCounterRush = damageInfo.isCounterRush;
            float animStartTime = isCounterRush ? 0.05f : 0.5f;
            float animDuration = isCounterRush ? 0.15f : 1.0f;
            
            // アニメーション更新（現在のダメージ用）
            animationController->updateResultCharacterAnimation(
                deltaTime, true, false, resultState.resultAnimationTimer,
                damageAppliedInAnimation, false,
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
            float animStartTime = isCounterRush ? 0.05f : 0.5f;
            float animDuration = isCounterRush ? 0.15f : 1.0f;
        
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
                    
                    int damage = damageInfo.damage;
                    bool isPlayerHit = damageInfo.isPlayerHit;
                    
                    // ダミーのダメージエントリ（damage == 0）の場合はダメージ適用をスキップ
                    if (damage > 0 && !isPlayerHit) {
                        // 攻撃アニメーションが当たる瞬間に効果音を再生
                        // 防御で勝った場合（カウンターラッシュ）はrush.ogg、それ以外はattack.ogg
                        if (damageInfo.commandType == BattleConstants::COMMAND_DEFEND || damageInfo.isCounterRush) {
                            AudioManager::getInstance().playSound("attack", 0);
                        } else {
                            AudioManager::getInstance().playSound("attack", 0);
                        }
                        
                        enemy->takeDamage(damage);
                        effectManager->triggerHitEffect(damage, enemyX, enemyY, false);
                        
                        // 通常攻撃または攻撃呪文の場合、ステータスアップ効果を切る
                        if (damageInfo.commandType == BattleConstants::COMMAND_ATTACK || 
                            damageInfo.commandType == BattleConstants::COMMAND_SPELL) {
                            if (player->hasNextTurnBonusActive()) {
                                player->clearNextTurnBonus();
                            }
                        }
                        
                        float intensity = BattleConstants::VICTORY_SHAKE_INTENSITY;
                        float shakeDuration = 0.3f;
                        effectManager->triggerScreenShake(intensity, shakeDuration, false, false);
                        
                        // 敵が倒れたかチェック
                        if (!enemy->getIsAlive()) {
                            // 敵が倒れたら勝利
                            std::cout << "[DEBUG] LAST_CHANCE: ダメージ適用時に敵が倒れました。checkBattleEnd()を呼びます。" << std::endl;
                            checkBattleEnd();
                            // checkBattleEnd()でフェーズが変更された場合は処理を終了
                            if (currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
                                std::cout << "[DEBUG] LAST_CHANCE: フェーズが変更されました。currentPhase=" << static_cast<int>(currentPhase) << std::endl;
                                return;
                            }
                            std::cout << "[DEBUG] LAST_CHANCE: 警告！checkBattleEnd()後もフェーズが変更されていません！" << std::endl;
                            return;
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
            }
        }
    }
    
    // 最初のアニメーション開始時のスクリーンシェイク（勝利）
    float shakeDuration = 0.3f;
    if (resultState.resultAnimationTimer < shakeDuration && !resultState.resultShakeActive && currentExecutingTurn == 0) {
        resultState.resultShakeActive = true;
        float intensity = BattleConstants::VICTORY_SHAKE_INTENSITY;
        effectManager->triggerScreenShake(intensity, shakeDuration, true, false);
    }
    
    // 全ダメージ適用完了後、戦闘終了判定
    if (currentExecutingTurn >= pendingDamages.size()) {
        // アニメーション完了後、さらに1.5秒待機（合計3秒）
        constexpr float animTotalDuration = 0.5f + 1.0f; // 1.5秒
        float finalWaitDuration = animTotalDuration * 2.0f; // 3.0秒
        float maxWaitTime = 4.0f;
        
        if (resultState.resultAnimationTimer >= finalWaitDuration || phaseTimer >= maxWaitTime) {
            updateStatus();
            
            // 敵が倒れたかチェック
            if (!enemy->getIsAlive()) {
                // 敵が倒れたら勝利
                std::cout << "[DEBUG] LAST_CHANCE: 全ダメージ適用完了後に敵が倒れました。checkBattleEnd()を呼びます。" << std::endl;
                checkBattleEnd();
                // checkBattleEnd()でフェーズが変更された場合は処理を終了
                if (currentPhase != BattlePhase::LAST_CHANCE_JUDGE_RESULT) {
                    std::cout << "[DEBUG] LAST_CHANCE: フェーズが変更されました。currentPhase=" << static_cast<int>(currentPhase) << std::endl;
                    return;
                }
                std::cout << "[DEBUG] LAST_CHANCE: 警告！checkBattleEnd()後もフェーズが変更されていません！" << std::endl;
                return;
            } else {
                // 敵が倒れなかった場合、敵の攻撃アニメーションを表示してからダメージを適用
                if (!enemyAttackStarted) {
                    std::cout << "[DEBUG] LAST_CHANCE: 敵が倒れませんでした。敵の攻撃アニメーションを開始します。" << std::endl;
                    enemyAttackStarted = true;
                    enemyAttackTimer = 0.0f;
                    animationController->resetResultAnimation();
                    addBattleLog("敵が倒れませんでした。敵の攻撃を受けてゲームオーバー...");
                }
                
                // 敵の攻撃アニメーションを更新
                enemyAttackTimer += deltaTime;
                float animStartTime = 0.5f;
                float animDuration = 1.0f;
                
                animationController->updateResultCharacterAnimation(
                    deltaTime, false, true, enemyAttackTimer,
                    damageAppliedInAnimation, false,
                    animStartTime, animDuration);
                
                // アニメーションのピーク時（攻撃が当たる瞬間）にダメージを適用
                if (enemyAttackTimer >= animStartTime) {
                    float animTime = enemyAttackTimer - animStartTime;
                    if (animTime >= 0.0f && animTime <= animDuration) {
                        float progress = animTime / animDuration;
                        if (progress >= BattleConstants::ATTACK_ANIMATION_HIT_MOMENT - BattleConstants::ATTACK_ANIMATION_HIT_WINDOW &&
                            progress <= BattleConstants::ATTACK_ANIMATION_HIT_MOMENT + BattleConstants::ATTACK_ANIMATION_HIT_WINDOW &&
                            !damageAppliedInAnimation) {
                            
                            int enemyDamage = battleLogic->calculateEnemyAttackDamage();
                            player->takeDamage(enemyDamage);
                            effectManager->triggerHitEffect(enemyDamage, playerX, playerY, true);
                            AudioManager::getInstance().playSound("attack", 0);
                            float intensity = BattleConstants::DRAW_SHAKE_INTENSITY;
                            float shakeDuration = 0.3f;
                            effectManager->triggerScreenShake(intensity, shakeDuration, true, false);
                            damageAppliedInAnimation = true;
                        }
                    }
                }
                
                // アニメーション完了後、ゲームオーバー
                float animTotalDuration = animStartTime + animDuration;
                if (enemyAttackTimer >= animTotalDuration) {
                    checkBattleEnd();
                    return;
                }
                
                // 敵の攻撃アニメーション中は、ここで処理を終了
                return;
            }
            
            // リセット（敵の攻撃アニメーションが完了した場合のみ）
            animationController->resetResultAnimation();
            currentExecutingTurn = 0;
            executeDelayTimer = 0.0f;
            damageAppliedInAnimation = false;
            pendingDamages.clear();
            phaseTimer = 0;
        }
    }
}