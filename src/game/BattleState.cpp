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
      isDesperateMode(false), commandTurnCount(3), currentSelectingTurn(0),
      playerWins(0), enemyWins(0), hasThreeWinStreak(false), isFirstCommandSelection(true),
      currentJudgingTurn(0), shakeOffsetX(0.0f), shakeOffsetY(0.0f), 
      shakeTimer(0.0f), shakeIntensity(0.0f), isVictoryShake(false), shakeTargetPlayer(false),
      judgeSubPhase(JudgeSubPhase::SHOW_PLAYER_COMMAND), judgeDisplayTimer(0.0f), currentJudgingTurnIndex(0),
      commandSelectAnimationTimer(0.0f), commandSelectSlideProgress(0.0f),
      resultAnimationTimer(0.0f), resultScale(0.0f), resultRotation(0.0f), resultShakeActive(false),
      playerAttackOffsetX(0.0f), playerAttackOffsetY(0.0f), enemyAttackOffsetX(0.0f), enemyAttackOffsetY(0.0f),
      enemyHitOffsetX(0.0f), enemyHitOffsetY(0.0f), playerHitOffsetX(0.0f), playerHitOffsetY(0.0f), attackAnimationFrame(0) {
    
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
    
    // 画面揺れの更新
    updateScreenShake(deltaTime);
    
    // コマンド選択UIのアニメーション更新
    if (currentPhase == BattlePhase::COMMAND_SELECT || 
        currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) {
        commandSelectAnimationTimer += deltaTime;
        // スライドアニメーション（0.3秒で完了）
        commandSelectSlideProgress = std::min(1.0f, commandSelectAnimationTimer / 0.3f);
    } else {
        commandSelectAnimationTimer = 0.0f;
        commandSelectSlideProgress = 0.0f;
    }
    
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
            if (phaseTimer > 1.0f) { // 1秒後にコマンド選択へ
                // 新しいシステム: 3ターン分のコマンド選択へ
                isDesperateMode = false;
                commandTurnCount = 3;
                isFirstCommandSelection = true;
                currentPhase = BattlePhase::COMMAND_SELECT;
                initializeCommandSelection();
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::COMMAND_SELECT:
            // 窮地モードチェック（最初のターンのみ）
            if (isFirstCommandSelection && checkDesperateModeCondition()) {
                isFirstCommandSelection = false;
                currentPhase = BattlePhase::DESPERATE_MODE_PROMPT;
                showDesperateModePrompt();
                break;
            }
            isFirstCommandSelection = false;
            
            // コマンド選択中
            if (currentSelectingTurn < commandTurnCount) {
                if (!isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
            } else {
                // 全選択完了 → 敵のコマンド生成 → 判定へ
                generateEnemyCommands();
                currentPhase = BattlePhase::JUDGE;
                prepareJudgeResults();
                currentJudgingTurn = 0;
                currentJudgingTurnIndex = 0;
                judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                judgeDisplayTimer = 0.0f;
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::JUDGE:
            // アニメーション付きで各ターンの結果を順に表示
            if (currentJudgingTurnIndex < commandTurnCount) {
                judgeDisplayTimer += deltaTime;
                
                switch (judgeSubPhase) {
                    case JudgeSubPhase::SHOW_PLAYER_COMMAND:
                        // プレイヤーのコマンド表示（1秒）
                        if (judgeDisplayTimer >= 1.0f) {
                            judgeSubPhase = JudgeSubPhase::SHOW_ENEMY_COMMAND;
                            judgeDisplayTimer = 0.0f;
                        }
                        break;
                        
                    case JudgeSubPhase::SHOW_ENEMY_COMMAND:
                        // 敵のコマンド表示（1秒）
                        if (judgeDisplayTimer >= 1.0f) {
                            judgeSubPhase = JudgeSubPhase::SHOW_RESULT;
                            judgeDisplayTimer = 0.0f;
                        }
                        break;
                        
                    case JudgeSubPhase::SHOW_RESULT:
                        // 勝敗結果表示（1.5秒）
                        if (judgeDisplayTimer >= 1.5f) {
                            // 次のターンへ
                            currentJudgingTurnIndex++;
                            judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                            judgeDisplayTimer = 0.0f;
                            
                            if (currentJudgingTurnIndex >= commandTurnCount) {
                                // 全ターン表示完了 → 結果発表へ
                                currentPhase = BattlePhase::JUDGE_RESULT;
                                showFinalResult();
                                phaseTimer = 0;
                            }
                        }
                        break;
                }
            }
            break;
            
        case BattlePhase::JUDGE_RESULT:
            // 結果発表のアニメーション更新
            resultAnimationTimer += deltaTime;
            
            // 拡大アニメーション（0.5秒で完了、その後は脈動）
            if (resultAnimationTimer < 0.5f) {
                resultScale = resultAnimationTimer / 0.5f; // 0.0から1.0へ
            } else {
                // 脈動効果
                float pulse = std::sin((resultAnimationTimer - 0.5f) * 3.14159f * 2.0f) * 0.1f;
                resultScale = 1.0f + pulse;
            }
            
            // 回転アニメーション（最初の1秒で360度回転）
            if (resultAnimationTimer < 1.0f) {
                resultRotation = resultAnimationTimer * 360.0f;
            } else {
                resultRotation = 0.0f;
            }
            
            // キャラクターアニメーション更新
            updateResultCharacterAnimation(deltaTime);
            
            // 画面揺れ（最初の0.3秒）
            if (resultAnimationTimer < 0.3f && !resultShakeActive) {
                resultShakeActive = true;
                // 激しい画面揺れ
                if (playerWins > enemyWins) {
                    triggerScreenShake(15.0f, 0.3f, true, false); // 勝利時：爽快感のある揺れ
                } else if (enemyWins > playerWins) {
                    triggerScreenShake(20.0f, 0.3f, false, true); // 敗北時：激しい揺れ
                } else {
                    triggerScreenShake(10.0f, 0.3f, false, false); // 引き分け：通常の揺れ
                }
            }
            
            // 結果発表を3秒表示
            if (phaseTimer > 3.0f) {
                currentPhase = BattlePhase::EXECUTE;
                phaseTimer = 0;
                resultAnimationTimer = 0.0f;
                resultScale = 0.0f;
                resultRotation = 0.0f;
                resultShakeActive = false;
            }
            break;
            
        case BattlePhase::EXECUTE:
            // 勝ったターン数分のコマンドを実行
            if (phaseTimer > 2.0f) {
                // 3連勝の場合は1.5倍、そうでない場合は通常倍率
                float multiplier = hasThreeWinStreak ? 1.5f : 1.0f;
                executeWinningTurns(multiplier);
                
                // 戦闘継続判定
                if (enemy->getIsAlive() && player->getIsAlive()) {
                    // 次の3ターンへ
                    currentPhase = BattlePhase::COMMAND_SELECT;
                    initializeCommandSelection();
                } else {
                    checkBattleEnd();
                }
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::DESPERATE_MODE_PROMPT:
            // 窮地モード発動確認（入力待ち）
            break;
            
        case BattlePhase::DESPERATE_COMMAND_SELECT:
            // 6ターン分のコマンド選択
            if (currentSelectingTurn < commandTurnCount) {
                if (!isShowingOptions) {
                    selectCommandForTurn(currentSelectingTurn);
                }
            } else {
                // 全選択完了 → 敵のコマンド生成 → 判定へ
                generateEnemyCommands();
                currentPhase = BattlePhase::DESPERATE_JUDGE;
                prepareJudgeResults();
                currentJudgingTurn = 0;
                currentJudgingTurnIndex = 0;
                judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                judgeDisplayTimer = 0.0f;
                phaseTimer = 0;
            }
            break;
            
        case BattlePhase::DESPERATE_JUDGE:
            // アニメーション付きで各ターンの結果を順に表示
            if (currentJudgingTurnIndex < commandTurnCount) {
                judgeDisplayTimer += deltaTime;
                
                switch (judgeSubPhase) {
                    case JudgeSubPhase::SHOW_PLAYER_COMMAND:
                        // プレイヤーのコマンド表示（1秒）
                        if (judgeDisplayTimer >= 1.0f) {
                            judgeSubPhase = JudgeSubPhase::SHOW_ENEMY_COMMAND;
                            judgeDisplayTimer = 0.0f;
                        }
                        break;
                        
                    case JudgeSubPhase::SHOW_ENEMY_COMMAND:
                        // 敵のコマンド表示（1秒）
                        if (judgeDisplayTimer >= 1.0f) {
                            judgeSubPhase = JudgeSubPhase::SHOW_RESULT;
                            judgeDisplayTimer = 0.0f;
                        }
                        break;
                        
                    case JudgeSubPhase::SHOW_RESULT:
                        // 勝敗結果表示（1.5秒）
                        if (judgeDisplayTimer >= 1.5f) {
                            // 次のターンへ
                            currentJudgingTurnIndex++;
                            judgeSubPhase = JudgeSubPhase::SHOW_PLAYER_COMMAND;
                            judgeDisplayTimer = 0.0f;
                            
                            if (currentJudgingTurnIndex >= commandTurnCount) {
                                // 全ターン表示完了 → 結果発表へ
                                currentPhase = BattlePhase::DESPERATE_JUDGE_RESULT;
                                showFinalResult();
                                phaseTimer = 0;
                            }
                        }
                        break;
                }
            }
            break;
            
        case BattlePhase::DESPERATE_JUDGE_RESULT:
            // 結果発表のアニメーション更新（窮地モードはより激しく）
            resultAnimationTimer += deltaTime;
            
            // 拡大アニメーション（0.5秒で完了、その後は脈動）
            if (resultAnimationTimer < 0.5f) {
                resultScale = resultAnimationTimer / 0.5f; // 0.0から1.0へ
            } else {
                // 脈動効果（窮地モードはより激しい）
                float pulse = std::sin((resultAnimationTimer - 0.5f) * 3.14159f * 3.0f) * 0.15f;
                resultScale = 1.0f + pulse;
            }
            
            // 回転アニメーション（最初の1秒で720度回転、より激しく）
            if (resultAnimationTimer < 1.0f) {
                resultRotation = resultAnimationTimer * 720.0f;
            } else {
                resultRotation = 0.0f;
            }
            
            // キャラクターアニメーション更新（窮地モードはより激しく）
            updateResultCharacterAnimation(deltaTime);
            
            // 画面揺れ（最初の0.5秒、より激しく）
            if (resultAnimationTimer < 0.5f && !resultShakeActive) {
                resultShakeActive = true;
                // 激しい画面揺れ
                if (playerWins > enemyWins) {
                    triggerScreenShake(25.0f, 0.5f, true, false); // 勝利時：超爽快感のある揺れ
                } else if (enemyWins > playerWins) {
                    triggerScreenShake(30.0f, 0.5f, false, true); // 敗北時：超激しい揺れ
                } else {
                    triggerScreenShake(15.0f, 0.5f, false, false); // 引き分け：激しい揺れ
                }
            }
            
            // 結果発表を3秒表示
            if (phaseTimer > 3.0f) {
                currentPhase = BattlePhase::DESPERATE_EXECUTE;
                phaseTimer = 0;
                resultAnimationTimer = 0.0f;
                resultScale = 0.0f;
                resultRotation = 0.0f;
                resultShakeActive = false;
            }
            break;
            
        case BattlePhase::DESPERATE_EXECUTE:
            // 勝ったターン数分の攻撃を実行（ダメージ1.5倍）
            if (phaseTimer > 2.0f) {
                executeWinningTurns(1.5f); // 1.5倍
                
                // 戦闘継続判定
                if (enemy->getIsAlive() && player->getIsAlive()) {
                    // 通常戦闘に戻る（3ターン）
                    isDesperateMode = false;
                    commandTurnCount = 3;
                    currentPhase = BattlePhase::COMMAND_SELECT;
                    initializeCommandSelection();
                } else {
                    checkBattleEnd();
                }
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
    
    // 読み合い判定時の特別な描画
    if (currentPhase == BattlePhase::JUDGE || currentPhase == BattlePhase::DESPERATE_JUDGE) {
        renderJudgeAnimation(graphics);
        // UI描画（画面揺れを適用しない）
        ui.render(graphics);
        
        // 夜のタイマーを表示（画面揺れを適用しない）
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
            CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
            CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    // コマンド選択時の特別な描画
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) && 
        isShowingOptions) {
        renderCommandSelectionUI(graphics);
        // UI描画は行わない（新しいUIを使用するため）
        // ui.render(graphics);
        
        // 夜のタイマーを表示（画面揺れを適用しない）
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
            CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
            CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    // 結果発表時の特別な描画
    if (currentPhase == BattlePhase::JUDGE_RESULT || 
        currentPhase == BattlePhase::DESPERATE_JUDGE_RESULT) {
        renderResultAnnouncement(graphics);
        // UI描画（画面揺れを適用しない）
        ui.render(graphics);
        
        // 夜のタイマーを表示（画面揺れを適用しない）
        if (nightTimerActive) {
            CommonUI::drawNightTimer(graphics, nightTimer, nightTimerActive, false);
            CommonUI::drawTargetLevel(graphics, TownState::s_targetLevel, TownState::s_levelGoalAchieved, player->getLevel());
            CommonUI::drawTrustLevels(graphics, player, nightTimerActive, false);
        }
        
        graphics.present();
        return;
    }
    
    // 画面揺れオフセットを適用（ゲーム要素のみ、UIは後で描画）
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
    // プレイヤーの描画（左側、固定位置、3倍サイズ）
    int playerBaseX = screenWidth / 4; // 左側1/4の位置
    int playerBaseY = screenHeight / 2; // 中央の高さ
    int playerX = playerBaseX;
    int playerY = playerBaseY;
    
    // 画面揺れオフセットを適用
    if (shakeTargetPlayer && shakeTimer > 0.0f) {
        playerX += static_cast<int>(shakeOffsetX);
        playerY += static_cast<int>(shakeOffsetY);
    }
    
    // 3倍のサイズ
    int playerWidth = 300; // 通常100px × 3
    int playerHeight = 300; // 通常100px × 3
    
    // プレイヤーのHP表示（頭上）
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    SDL_Color playerHpColor = {100, 255, 100, 255};
    int playerHpX = playerX - 100;
    int playerHpY = playerY - playerHeight / 2 - 40;
    if (shakeTargetPlayer && shakeTimer > 0.0f) {
        playerHpX += static_cast<int>(shakeOffsetX);
        playerHpY += static_cast<int>(shakeOffsetY);
    }
    graphics.drawText(playerHpText, playerHpX, playerHpY, "default", playerHpColor);
    
    SDL_Texture* playerTexture = graphics.getTexture("player");
    if (playerTexture) {
        // 画像が存在する場合は画像を描画（中央揃え）
        graphics.drawTexture(playerTexture, playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        // 画像が存在しない場合は四角形で描画（フォールバック）
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, false);
    }
    
    // 敵の描画（右側、固定位置、3倍サイズ）
    int enemyBaseX = screenWidth * 3 / 4; // 右側3/4の位置
    int enemyBaseY = screenHeight / 2; // 中央の高さ
    int enemyX = enemyBaseX;
    int enemyY = enemyBaseY;
    
    // 画面揺れオフセットを適用
    if (!shakeTargetPlayer && shakeTimer > 0.0f) {
        enemyX += static_cast<int>(shakeOffsetX);
        enemyY += static_cast<int>(shakeOffsetY);
    }
    
    // 3倍のサイズ
    int enemyWidth = 300; // 通常100px × 3
    int enemyHeight = 300; // 通常100px × 3
    
    // 敵のHP表示（頭上）
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    int enemyHpX = enemyX - 100;
    int enemyHpY = enemyY - enemyHeight / 2 - 40;
    if (!shakeTargetPlayer && shakeTimer > 0.0f) {
        enemyHpX += static_cast<int>(shakeOffsetX);
        enemyHpY += static_cast<int>(shakeOffsetY);
    }
    graphics.drawText(enemyHpText, enemyHpX, enemyHpY, "default", enemyHpColor);
    
    SDL_Texture* enemyTexture = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTexture) {
        // 画像が存在する場合は画像を描画（中央揃え）
        graphics.drawTexture(enemyTexture, enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        // 画像が存在しない場合は四角形で描画（フォールバック）
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, false);
    }
    
    // UI描画（画面揺れを適用しない）
    ui.render(graphics);
    
    // 夜のタイマーを表示（画面揺れを適用しない）
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
    
    // 新しいコマンド選択システム
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) && 
        !isShowingOptions) {
        selectCommandForTurn(currentSelectingTurn);
    }
    // 窮地モード発動確認
    else if (currentPhase == BattlePhase::DESPERATE_MODE_PROMPT && !isShowingOptions) {
        showDesperateModePrompt();
    }
    // 既存のシステム（後方互換性）
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
    
    // Qキーで1つ前のコマンド選択に戻る（コマンド選択中のみ）
    if ((currentPhase == BattlePhase::COMMAND_SELECT || 
         currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) &&
        isShowingOptions) {
        if (input.isKeyJustPressed(InputKey::Q)) {
            if (currentSelectingTurn > 0) {
                // 1つ前のターンに戻る
                currentSelectingTurn--;
                // そのターンのコマンドをクリア
                playerCommands[currentSelectingTurn] = -1;
                // 再選択画面を表示
                selectCommandForTurn(currentSelectingTurn);
            }
            return; // Qキーで戻った場合は他の処理をスキップ
        }
    }
    
    // AボタンまたはEnterキーで選択
    if (input.isKeyJustPressed(InputKey::GAMEPAD_A) || input.isKeyJustPressed(InputKey::ENTER)) {
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
    
    if (currentPhase == BattlePhase::DESPERATE_MODE_PROMPT) {
        if (selected == "大勝負に挑む") {
            isDesperateMode = true;
            commandTurnCount = 6;
            currentPhase = BattlePhase::DESPERATE_COMMAND_SELECT;
            initializeCommandSelection();
        } else if (selected == "通常戦闘を続ける") {
            isDesperateMode = false;
            commandTurnCount = 3;
            currentPhase = BattlePhase::COMMAND_SELECT;
            initializeCommandSelection();
        }
    } else if (currentPhase == BattlePhase::COMMAND_SELECT || 
               currentPhase == BattlePhase::DESPERATE_COMMAND_SELECT) {
        // コマンド選択
        int cmd = -1;
        if (selected == "攻撃") {
            cmd = 0;
        } else if (selected == "防御") {
            cmd = 1;
        } else if (selected == "呪文") {
            // 呪文を選択した場合、MPをチェック
            if (player->getMp() > 0) {
                cmd = 2;
            } else {
                addBattleLog("MPが足りません！");
                return; // 選択をキャンセル
            }
        }
        
        if (cmd >= 0 && currentSelectingTurn < commandTurnCount) {
            playerCommands[currentSelectingTurn] = cmd;
            currentSelectingTurn++;
            
            if (currentSelectingTurn < commandTurnCount) {
                // 次のターンのコマンド選択へ
                selectCommandForTurn(currentSelectingTurn);
            } else {
                // 全選択完了（update()で処理される）
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

// 新しいコマンド選択システムの実装

void BattleState::initializeCommandSelection() {
    playerCommands.clear();
    enemyCommands.clear();
    playerCommands.resize(commandTurnCount, -1);
    enemyCommands.resize(commandTurnCount, -1);
    currentSelectingTurn = 0;
    playerWins = 0;
    enemyWins = 0;
    hasThreeWinStreak = false;
    isShowingOptions = false;
}

void BattleState::selectCommandForTurn(int turnIndex) {
    currentOptions.clear();
    currentOptions.push_back("攻撃");
    currentOptions.push_back("防御");
    
    // 呪文はMPがある場合のみ選択可能
    if (player->getMp() > 0) {
        currentOptions.push_back("呪文");
    }
    
    selectedOption = 0;
    isShowingOptions = true;
    
    // アニメーションタイマーをリセット
    commandSelectAnimationTimer = 0.0f;
    commandSelectSlideProgress = 0.0f;
    
    // 古いUIの表示は行わない（新しいUIを使用するため）
    // updateOptionDisplay();
}

int BattleState::judgeRound(int playerCmd, int enemyCmd) {
    // 0=攻撃, 1=防御, 2=呪文
    // 戻り値: 1=プレイヤー勝ち, -1=敵勝ち, 0=引き分け
    
    // 3すくみシステム: 攻撃 > 呪文 > 防御 > 攻撃
    
    // 攻撃 vs 防御 → 防御が勝ち
    if (playerCmd == 0 && enemyCmd == 1) return -1;
    if (playerCmd == 1 && enemyCmd == 0) return 1;
    
    // 攻撃 vs 呪文 → 攻撃が勝ち（呪文を中断）
    if (playerCmd == 0 && enemyCmd == 2) return 1;
    if (playerCmd == 2 && enemyCmd == 0) return -1;
    
    // 呪文 vs 防御 → 呪文が勝ち（防御を貫通）
    if (playerCmd == 2 && enemyCmd == 1) return 1;
    if (playerCmd == 1 && enemyCmd == 2) return -1;
    
    // 同じコマンド同士 → 引き分け
    if (playerCmd == enemyCmd) return 0;
    
    return 0;
}

void BattleState::prepareJudgeResults() {
    playerWins = 0;
    enemyWins = 0;
    hasThreeWinStreak = false;
    turnResults.clear();
    
    // 各ターンの結果を準備
    for (int i = 0; i < commandTurnCount; i++) {
        int result = judgeRound(playerCommands[i], enemyCommands[i]);
        
        std::string playerCmd = getCommandName(playerCommands[i]);
        std::string enemyCmd = getCommandName(enemyCommands[i]);
        
        std::string turnText = "ターン" + std::to_string(i + 1) + ": ";
        turnText += "自分[" + playerCmd + "] vs 敵[" + enemyCmd + "] → ";
        
        if (result == 1) {
            turnText += "自分の勝ち ✅";
            playerWins++;
        } else if (result == -1) {
            turnText += "敵の勝ち ❌";
            enemyWins++;
        } else {
            turnText += "引き分け";
        }
        
        turnResults.push_back(turnText);
    }
    
    // 3連勝の判定（最初の3ターンで連続勝利）
    if (commandTurnCount >= 3 && playerWins >= 3) {
        bool isThreeWinStreak = true;
        for (int i = 0; i < 3; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) != 1) {
                isThreeWinStreak = false;
                break;
            }
        }
        hasThreeWinStreak = isThreeWinStreak;
    }
    
    // 最初のターンの結果を表示
    if (!turnResults.empty()) {
        showTurnResult(0);
    }
}

void BattleState::showTurnResult(int turnIndex) {
    if (turnIndex < 0 || turnIndex >= turnResults.size()) return;
    
    // 古いUIのヘッダーは削除（新しいUIを使用するため）
    // std::string header = "";
    // if (isDesperateMode) {
    //     header = "⚡ 大勝負 ⚡\n\n";
    // } else {
    //     header = "📊 読み合い 📊\n\n";
    // }
    
    // バトルログにも表示しない（新しいUIで表示するため）
    // addBattleLog(header + turnResults[turnIndex]);
}

void BattleState::showFinalResult() {
    // アニメーションタイマーをリセット
    resultAnimationTimer = 0.0f;
    resultScale = 0.0f;
    resultRotation = 0.0f;
    resultShakeActive = false;
    
    // キャラクターアニメーションをリセット
    playerAttackOffsetX = 0.0f;
    playerAttackOffsetY = 0.0f;
    enemyAttackOffsetX = 0.0f;
    enemyAttackOffsetY = 0.0f;
    enemyHitOffsetX = 0.0f;
    enemyHitOffsetY = 0.0f;
    playerHitOffsetX = 0.0f;
    playerHitOffsetY = 0.0f;
    attackAnimationFrame = 0;
    
    // テキストログにも表示（後方互換性のため）
    std::string resultText = "\n【結果発表】\n";
    resultText += "自分 " + std::to_string(playerWins) + "勝";
    resultText += " " + std::to_string(enemyWins) + "敗\n\n";
    
    if (playerWins > enemyWins) {
        if (hasThreeWinStreak) {
            resultText += "🔥 3連勝！ダメージ1.5倍！ 🔥\n";
        } else if (isDesperateMode) {
            resultText += "🎉 一発逆転成功！ 🎉\n";
        } else {
            resultText += "🎯 勝利！\n";
        }
        resultText += "自分が" + std::to_string(playerWins) + "ターン分の攻撃を実行！";
    } else if (enemyWins > playerWins) {
        if (isDesperateMode) {
            resultText += "💀 大敗北... 💀\n";
        } else {
            resultText += "❌ 敗北...\n";
        }
        resultText += "敵が" + std::to_string(enemyWins) + "ターン分の攻撃を実行！";
    } else {
        resultText += "⚖️ 引き分け\n";
        resultText += "両方がダメージを受ける";
    }
    
    addBattleLog(resultText);
}

void BattleState::judgeBattle() {
    // 後方互換性のため残す（prepareJudgeResultsを使用）
    prepareJudgeResults();
}

void BattleState::executeWinningTurns(float damageMultiplier) {
    if (playerWins > enemyWins) {
        // プレイヤーが勝ったターン数分の攻撃を実行
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == 1) {
                // 勝ったターンのコマンドを実行
                int cmd = playerCommands[i];
                if (cmd == 0) { // 攻撃
                    int baseDamage = player->calculateDamageWithBonus(*enemy);
                    int damage = static_cast<int>(baseDamage * damageMultiplier);
                    enemy->takeDamage(damage);
                    // プレイヤーの攻撃時に爽快感のある画面揺れを発動（敵を揺らす）
                    // 3連勝の場合はさらに激しく
                    float shakeIntensity = hasThreeWinStreak ? 30.0f : 20.0f;
                    triggerScreenShake(shakeIntensity, 0.6f, true, false);
                    std::string msg = player->getName() + "の攻撃！";
                    if (hasThreeWinStreak) {
                        msg += "【3連勝ボーナス！】";
                    }
                    msg += "\n" + enemy->getTypeName() + "は" + 
                           std::to_string(damage) + "のダメージを受けた！";
                    if (damageMultiplier > 1.0f) {
                        msg += "（" + std::to_string(static_cast<int>(damageMultiplier * 100)) + "%）";
                    }
                    addBattleLog(msg);
                } else if (cmd == 1) { // 防御
                    // 防御は実行しない（勝ったので攻撃を防いだ）
                    addBattleLog(player->getName() + "は防御に成功した！");
                } else if (cmd == 2) { // 呪文
                    // 呪文はMPを消費して、防御を貫通する高ダメージ
                    if (player->getMp() > 0) {
                        player->useMp(1);
                        int baseDamage = player->calculateDamageWithBonus(*enemy);
                        int damage = static_cast<int>(baseDamage * 1.5f * damageMultiplier); // 呪文は1.5倍ダメージ
                        enemy->takeDamage(damage);
                        // 呪文はさらに爽快感のある画面揺れを発動（敵を揺らす）
                        // 3連勝の場合はさらに激しく
                        float shakeIntensity = hasThreeWinStreak ? 35.0f : 25.0f;
                        triggerScreenShake(shakeIntensity, 0.7f, true, false);
                        std::string msg = player->getName() + "の呪文！";
                        if (hasThreeWinStreak) {
                            msg += "【3連勝ボーナス！】";
                        }
                        msg += "\n" + enemy->getTypeName() + "は" + 
                               std::to_string(damage) + "のダメージを受けた！";
                        if (damageMultiplier > 1.0f) {
                            msg += "（" + std::to_string(static_cast<int>(damageMultiplier * 100)) + "%）";
                        }
                        addBattleLog(msg);
                    } else {
                        // MPが足りない場合は通常攻撃にフォールバック
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
    } else if (enemyWins > playerWins) {
        // 敵が勝ったターン数分の攻撃を実行
        int enemyAttackCount = 0;
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == -1) {
                enemyAttackCount++;
                // 敵の攻撃を実行
                int damage = enemy->getAttack() - player->getDefense();
                if (damage < 0) damage = 0;
                player->takeDamage(damage);
                // ダメージ時に画面揺れを発動（プレイヤーを揺らす）
                triggerScreenShake(15.0f, 0.5f, false, true);
                std::string msg = enemy->getTypeName() + "の攻撃！\n" + 
                                 player->getName() + "は" + 
                                 std::to_string(damage) + "のダメージを受けた！";
                addBattleLog(msg);
            }
        }
    } else {
        // 引き分け：両方がダメージを受ける（相打ちのターンのみ）
        bool hasDraw = false;
        for (int i = 0; i < commandTurnCount; i++) {
            if (judgeRound(playerCommands[i], enemyCommands[i]) == 0) {
                if (!hasDraw) {
                    hasDraw = true;
                    addBattleLog("相打ち！両方がダメージを受けた！");
                }
                // 相打ち
                int playerDamage = enemy->getAttack() / 2;
                int enemyDamage = player->calculateDamageWithBonus(*enemy) / 2;
                if (playerDamage > 0) {
                    player->takeDamage(playerDamage);
                    // ダメージ時に画面揺れを発動（プレイヤーを揺らす）
                    triggerScreenShake(10.0f, 0.3f, false, true);
                }
                if (enemyDamage > 0) enemy->takeDamage(enemyDamage);
            }
        }
    }
    
    updateStatus();
}

bool BattleState::checkDesperateModeCondition() {
    float hpRatio = (float)player->getHp() / player->getMaxHp();
    return hpRatio <= 0.3f; // 30%以下
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

void BattleState::generateEnemyCommands() {
    // 敵のコマンドを生成（戦略的AI）
    // プレイヤーのHPが低い場合は攻撃を多めに、高い場合は防御を多めに
    float playerHpRatio = static_cast<float>(player->getHp()) / static_cast<float>(player->getMaxHp());
    
    for (int i = 0; i < commandTurnCount; i++) {
        int cmd;
        int randVal = rand() % 100;
        
        if (playerHpRatio < 0.3f) {
            // プレイヤーHPが低い → 攻撃重視（60%攻撃、30%防御、10%呪文）
            if (randVal < 60) {
                cmd = 0; // 攻撃
            } else if (randVal < 90) {
                cmd = 1; // 防御
            } else {
                cmd = 2; // 呪文
            }
        } else if (playerHpRatio < 0.6f) {
            // プレイヤーHPが中程度 → バランス（40%攻撃、40%防御、20%呪文）
            if (randVal < 40) {
                cmd = 0; // 攻撃
            } else if (randVal < 80) {
                cmd = 1; // 防御
            } else {
                cmd = 2; // 呪文
            }
        } else {
            // プレイヤーHPが高い → 防御重視（30%攻撃、50%防御、20%呪文）
            if (randVal < 30) {
                cmd = 0; // 攻撃
            } else if (randVal < 80) {
                cmd = 1; // 防御
            } else {
                cmd = 2; // 呪文
            }
        }
        
        enemyCommands[i] = cmd;
    }
}

std::string BattleState::getCommandName(int cmd) {
    switch (cmd) {
        case 0: return "攻撃";
        case 1: return "防御";
        case 2: return "呪文";
        default: return "不明";
    }
}

void BattleState::triggerScreenShake(float intensity, float duration, bool victoryShake, bool targetPlayer) {
    shakeIntensity = intensity;
    shakeTimer = duration;
    isVictoryShake = victoryShake;
    shakeTargetPlayer = targetPlayer;
}

void BattleState::updateScreenShake(float deltaTime) {
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime;
        
        // ランダムな揺れを生成
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        
        float currentIntensity;
        if (isVictoryShake) {
            // 勝利時の爽快感のある揺れ：より激しく、複数回のパルス
            float progress = 1.0f - (shakeTimer / (shakeTimer + deltaTime));
            // パルス効果：sin波を使って複数回の強い揺れを生成
            float pulse = std::sin(progress * 3.14159f * 4.0f); // 4回のパルス
            currentIntensity = shakeIntensity * (1.0f - progress * 0.5f) * (0.5f + 0.5f * std::abs(pulse));
        } else {
            // 通常の揺れ：時間が経つにつれて弱くする
            currentIntensity = shakeIntensity * (shakeTimer / (shakeTimer + deltaTime));
        }
        
        shakeOffsetX = dis(gen) * currentIntensity;
        shakeOffsetY = dis(gen) * currentIntensity;
        
        if (shakeTimer <= 0.0f) {
            shakeTimer = 0.0f;
            shakeOffsetX = 0.0f;
            shakeOffsetY = 0.0f;
            shakeIntensity = 0.0f;
            isVictoryShake = false;
            shakeTargetPlayer = false;
        }
    } else {
        shakeOffsetX = 0.0f;
        shakeOffsetY = 0.0f;
        isVictoryShake = false;
        shakeTargetPlayer = false;
    }
}

void BattleState::renderJudgeAnimation(Graphics& graphics) {
    if (currentJudgingTurnIndex >= commandTurnCount) return;
    
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    int leftX = screenWidth / 4;
    int rightX = screenWidth * 3 / 4;
    
    // 背景を暗く（緊張感）
    graphics.setDrawColor(0, 0, 0, 220);
    graphics.drawRect(0, 0, screenWidth, screenHeight, true);
    
    // キャラクターの描画（左側と右側、3倍サイズ、固定位置）
    // プレイヤーの描画（左側、固定位置）
    int playerBaseX = screenWidth / 4; // 左側1/4の位置
    int playerBaseY = screenHeight / 2; // 中央の高さ
    int playerWidth = 300; // 3倍サイズ
    int playerHeight = 300;
    
    SDL_Texture* playerTex = graphics.getTexture("player");
    if (playerTex) {
        graphics.drawTexture(playerTex, playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    // 敵の描画（右側、固定位置）
    int enemyBaseX = screenWidth * 3 / 4; // 右側3/4の位置
    int enemyBaseY = screenHeight / 2; // 中央の高さ
    int enemyWidth = 300; // 3倍サイズ
    int enemyHeight = 300;
    
    SDL_Texture* enemyTex = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTex) {
        graphics.drawTexture(enemyTex, enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
    
    // ターン表示（左上端）
    std::string turnText = "ターン " + std::to_string(currentJudgingTurnIndex + 1) + " / " + std::to_string(commandTurnCount);
    SDL_Color turnColor = {255, 255, 255, 255};
    graphics.drawText(turnText, 20, 20, "default", turnColor);
    
    // プレイヤーのHP表示（頭上）
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    SDL_Color playerHpColor = {100, 255, 100, 255};
    graphics.drawText(playerHpText, playerBaseX - 100, playerBaseY - playerHeight / 2 - 40, "default", playerHpColor);
    
    // 敵のHP表示（頭上）
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    graphics.drawText(enemyHpText, enemyBaseX - 100, enemyBaseY - enemyHeight / 2 - 40, "default", enemyHpColor);
    
    // プレイヤーのコマンド表示（左側、キャラクターの下）
    int playerDisplayX = leftX;
    if (judgeSubPhase == JudgeSubPhase::SHOW_PLAYER_COMMAND) {
        // アニメーション：左からスライドイン
        float slideProgress = std::min(1.0f, judgeDisplayTimer / 0.3f);
        int slideOffset = (int)((1.0f - slideProgress) * 300); // 左から300pxスライド
        playerDisplayX = leftX - slideOffset;
    }
    
    // プレイヤーのコマンド表示
    if (judgeSubPhase >= JudgeSubPhase::SHOW_PLAYER_COMMAND) {
        // プレイヤー名
        SDL_Color playerNameColor = {100, 200, 255, 255};
        graphics.drawText(player->getName(), playerDisplayX - 100, centerY + 180, "default", playerNameColor);
        
        // プレイヤーのコマンド
        std::string playerCmd = getCommandName(playerCommands[currentJudgingTurnIndex]);
        SDL_Color playerCmdColor = {150, 220, 255, 255}; // 明るい青系
        int cmdTextX = playerDisplayX - 100;
        int cmdTextY = centerY + 220;
        
        // コマンドテキストを大きく表示
        graphics.drawText(playerCmd, cmdTextX, cmdTextY, "default", playerCmdColor);
        
        // コマンドの枠（光る効果）
        if (judgeSubPhase == JudgeSubPhase::SHOW_PLAYER_COMMAND) {
            float glowProgress = std::sin(judgeDisplayTimer * 3.14159f * 2.0f) * 0.5f + 0.5f;
            Uint8 glowAlpha = (Uint8)(glowProgress * 100);
            graphics.setDrawColor(100, 200, 255, glowAlpha);
            graphics.drawRect(cmdTextX - 10, cmdTextY - 5, 220, 40, false);
        }
    }
    
    // 敵のコマンド表示（右側、キャラクターの下）
    int enemyDisplayX = rightX;
    if (judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        // アニメーション：右からスライドイン
        float slideProgress = std::min(1.0f, judgeDisplayTimer / 0.3f);
        int slideOffset = (int)((1.0f - slideProgress) * 300); // 右から300pxスライド
        enemyDisplayX = rightX + slideOffset;
    }
    
    if (judgeSubPhase >= JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        // 敵名
        SDL_Color enemyNameColor = {255, 100, 100, 255};
        graphics.drawText(enemy->getTypeName(), enemyDisplayX - 100, centerY + 180, "default", enemyNameColor);
        
        // 敵のコマンド
        std::string enemyCmd = getCommandName(enemyCommands[currentJudgingTurnIndex]);
        SDL_Color enemyCmdColor = {255, 150, 150, 255}; // 明るい赤系
        int cmdTextX = enemyDisplayX - 100;
        int cmdTextY = centerY + 220;
        
        graphics.drawText(enemyCmd, cmdTextX, cmdTextY, "default", enemyCmdColor);
        
        // コマンドの枠（光る効果）
        if (judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
            float glowProgress = std::sin(judgeDisplayTimer * 3.14159f * 2.0f) * 0.5f + 0.5f;
            Uint8 glowAlpha = (Uint8)(glowProgress * 100);
            graphics.setDrawColor(255, 100, 100, glowAlpha);
            graphics.drawRect(cmdTextX - 10, cmdTextY - 5, 220, 40, false);
        }
    }
    
    // VS表示（中央、キャラクターの上）
    if (judgeSubPhase >= JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        SDL_Color vsColor = {255, 255, 255, 255};
        // VSを点滅させる
        float blinkProgress = std::sin(judgeDisplayTimer * 3.14159f * 4.0f) * 0.5f + 0.5f;
        vsColor.a = (Uint8)(blinkProgress * 255);
        graphics.drawText("VS", centerX - 30, centerY - 200, "default", vsColor);
    }
    
    // 勝敗結果表示（真ん中、キャラクターの間）
    if (judgeSubPhase == JudgeSubPhase::SHOW_RESULT) {
        int result = judgeRound(playerCommands[currentJudgingTurnIndex], 
                               enemyCommands[currentJudgingTurnIndex]);
        
        std::string resultText;
        SDL_Color resultColor;
        
        // アニメーション：拡大しながら表示
        float scaleProgress = std::min(1.0f, judgeDisplayTimer / 0.5f);
        float scale = 0.3f + scaleProgress * 0.7f; // 0.3倍から1.0倍へ
        
        if (result == 1) {
            resultText = "勝ち！";
            resultColor = {255, 215, 0, 255}; // 金色
        } else if (result == -1) {
            resultText = "負け...";
            resultColor = {255, 0, 0, 255}; // 赤色
        } else {
            resultText = "引き分け";
            resultColor = {200, 200, 200, 255}; // 灰色
        }
        
        // 結果テキストを大きく表示（拡大アニメーション）
        int textWidth = 200; // 仮の幅
        int textHeight = 60; // 仮の高さ
        int scaledWidth = (int)(textWidth * scale);
        int scaledHeight = (int)(textHeight * scale);
        int textX = centerX - scaledWidth / 2;
        int textY = centerY - scaledHeight / 2;
        
        // 背景を描画（強調）
        graphics.setDrawColor(resultColor.r / 2, resultColor.g / 2, resultColor.b / 2, 150);
        graphics.drawRect(textX - 20, textY - 20, scaledWidth + 40, scaledHeight + 40, true);
        
        // テキストを描画（簡易版：実際には大きなフォントが必要）
        graphics.drawText(resultText, textX, textY, "default", resultColor);
        
        // 勝利時の特別演出
        if (result == 1) {
            // 金色の光る効果
            float glowProgress = std::sin(judgeDisplayTimer * 3.14159f * 4.0f) * 0.5f + 0.5f;
            graphics.setDrawColor(255, 215, 0, (Uint8)(glowProgress * 100));
            graphics.drawRect(textX - 30, textY - 30, scaledWidth + 60, scaledHeight + 60, false);
        }
    }
}

void BattleState::renderCommandSelectionUI(Graphics& graphics) {
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // 背景を半透明で暗く（緊張感）
    graphics.setDrawColor(0, 0, 0, 180);
    graphics.drawRect(0, 0, screenWidth, screenHeight, true);
    
    // キャラクターの描画（左側と右側、3倍サイズ）
    // プレイヤーの描画（左側、固定位置）
    int playerBaseX = screenWidth / 4; // 左側1/4の位置
    int playerBaseY = screenHeight / 2; // 中央の高さ
    int playerWidth = 300; // 3倍サイズ
    int playerHeight = 300;
    
    SDL_Texture* playerTex = graphics.getTexture("player");
    if (playerTex) {
        graphics.drawTexture(playerTex, playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    // 敵の描画（右側、固定位置）
    int enemyBaseX = screenWidth * 3 / 4; // 右側3/4の位置
    int enemyBaseY = screenHeight / 2; // 中央の高さ
    int enemyWidth = 300; // 3倍サイズ
    int enemyHeight = 300;
    
    SDL_Texture* enemyTex = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTex) {
        graphics.drawTexture(enemyTex, enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
    
    // ターン数表示（左上端）
    std::string turnText = "ターン " + std::to_string(currentSelectingTurn + 1) + " / " + std::to_string(commandTurnCount);
    if (isDesperateMode) {
        turnText += "  ⚡ 大勝負 ⚡";
    }
    SDL_Color turnColor = {255, 255, 255, 255};
    graphics.drawText(turnText, 20, 20, "default", turnColor);
    
    // コマンド選択ボタンを中央に表示（下からスライドイン）
    float buttonSlideProgress = std::min(1.0f, commandSelectSlideProgress);
    int baseY = centerY - 50;
    int slideOffset = (int)((1.0f - buttonSlideProgress) * 200); // 下から200pxスライド
    
    int buttonWidth = 200;
    int buttonHeight = 60;
    int buttonSpacing = 80;
    int startX = centerX - (buttonWidth / 2);
    int startY = baseY + slideOffset;
    
    for (size_t i = 0; i < currentOptions.size(); i++) {
        int buttonY = startY + (int)(i * buttonSpacing);
        
        // 選択中のボタンを強調
        bool isSelected = (static_cast<int>(i) == selectedOption);
        
        // ボタンの背景
        SDL_Color bgColor;
        if (isSelected) {
            // 選択中：明るい色、光る効果
            float glowProgress = std::sin(commandSelectAnimationTimer * 3.14159f * 4.0f) * 0.3f + 0.7f;
            bgColor = {(Uint8)(100 * glowProgress), (Uint8)(200 * glowProgress), (Uint8)(255 * glowProgress), 200};
        } else {
            // 非選択：暗い色
            bgColor = {50, 50, 50, 150};
        }
        
        graphics.setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        graphics.drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, true);
        
        // ボタンの枠
        if (isSelected) {
            // 選択中：金色の枠
            SDL_Color borderColor = {255, 215, 0, 255};
            graphics.setDrawColor(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            graphics.drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, false);
            // 二重枠（強調）
            graphics.drawRect(startX - 8, buttonY - 3, buttonWidth + 16, buttonHeight + 6, false);
        } else {
            // 非選択：灰色の枠
            SDL_Color borderColor = {150, 150, 150, 255};
            graphics.setDrawColor(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            graphics.drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, false);
        }
        
        // コマンドテキスト
        SDL_Color textColor;
        if (isSelected) {
            textColor = {255, 255, 255, 255}; // 白色（強調）
        } else {
            textColor = {200, 200, 200, 255}; // 灰色
        }
        
        // 選択中の場合は少し大きく表示
        int textX = startX + (buttonWidth / 2) - 50;
        int textY = buttonY + (buttonHeight / 2) - 15;
        
        if (isSelected) {
            // 選択中：金色の矢印を表示
            graphics.drawText("▶", textX - 30, textY, "default", {255, 215, 0, 255}); // 金色の矢印
            graphics.drawText(currentOptions[i], textX, textY, "default", textColor);
        } else {
            graphics.drawText(currentOptions[i], textX, textY, "default", textColor);
        }
    }
    
    // 説明文（下に表示）
    std::string hintText = "↑↓で選択  Enterで決定  Qで戻る";
    SDL_Color hintColor = {150, 150, 150, 255};
    graphics.drawText(hintText, centerX - 120, screenHeight - 100, "default", hintColor);
    
    // プレイヤーのHP表示（頭上）
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    SDL_Color playerHpColor = {100, 255, 100, 255};
    graphics.drawText(playerHpText, playerBaseX - 100, playerBaseY - playerHeight / 2 - 40, "default", playerHpColor);
    
    // 敵のHP表示（頭上）
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    graphics.drawText(enemyHpText, enemyBaseX - 100, enemyBaseY - enemyHeight / 2 - 40, "default", enemyHpColor);
}

void BattleState::renderResultAnnouncement(Graphics& graphics) {
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // 背景を暗く（緊張感）
    graphics.setDrawColor(0, 0, 0, 220);
    graphics.drawRect(0, 0, screenWidth, screenHeight, true);
    
    // 結果判定
    bool isVictory = (playerWins > enemyWins);
    bool isDefeat = (enemyWins > playerWins);
    
    // メイン結果テキスト
    std::string mainText;
    SDL_Color mainColor;
    
    if (isVictory) {
        if (isDesperateMode) {
            mainText = "🎉 一発逆転成功！ 🎉";
        } else {
            mainText = "🎯 勝利！";
        }
        mainColor = {255, 215, 0, 255}; // 金色
    } else if (isDefeat) {
        if (isDesperateMode) {
            mainText = "💀 大敗北... 💀";
        } else {
            mainText = "❌ 敗北...";
        }
        mainColor = {255, 0, 0, 255}; // 赤色
    } else {
        mainText = "⚖️ 引き分け";
        mainColor = {200, 200, 200, 255}; // 灰色
    }
    
    // メインテキストの描画（拡大＋回転アニメーション）
    // resultScaleが0の場合でも最小サイズを確保
    float displayScale = std::max(0.3f, resultScale); // 最小30%のサイズで表示
    int textWidth = 400;
    int textHeight = 100;
    int scaledWidth = (int)(textWidth * displayScale);
    int scaledHeight = (int)(textHeight * displayScale);
    int textX = centerX - scaledWidth / 2;
    int textY = centerY - scaledHeight / 2 - 100;
    
    // 背景を描画（強調、脈動効果）
    float glowIntensity = std::sin(resultAnimationTimer * 3.14159f * 4.0f) * 0.3f + 0.7f;
    graphics.setDrawColor((Uint8)(mainColor.r * glowIntensity * 0.5f), 
                          (Uint8)(mainColor.g * glowIntensity * 0.5f), 
                          (Uint8)(mainColor.b * glowIntensity * 0.5f), 
                          200);
    graphics.drawRect(textX - 30, textY - 30, scaledWidth + 60, scaledHeight + 60, true);
    
    // 外側の光る枠（勝利時は特に激しく）
    if (isVictory) {
        float outerGlow = std::sin(resultAnimationTimer * 3.14159f * 6.0f) * 0.5f + 0.5f;
        graphics.setDrawColor(255, 215, 0, (Uint8)(outerGlow * 150));
        graphics.drawRect(textX - 50, textY - 50, scaledWidth + 100, scaledHeight + 100, false);
        graphics.drawRect(textX - 45, textY - 45, scaledWidth + 90, scaledHeight + 90, false);
    }
    
    // 3連勝の強調表示（メインテキストの上に大きく表示）
    if (hasThreeWinStreak && isVictory) {
        float streakScale = 0.5f + std::sin(resultAnimationTimer * 3.14159f * 4.0f) * 0.3f; // 0.2倍から0.8倍へ
        std::string streakText = "🔥 3連勝！ 🔥";
        SDL_Color streakColor = {255, 215, 0, 255}; // 金色
        
        int streakTextWidth = 300;
        int streakTextHeight = 80;
        int streakScaledWidth = (int)(streakTextWidth * streakScale);
        int streakScaledHeight = (int)(streakTextHeight * streakScale);
        int streakTextX = centerX - streakScaledWidth / 2;
        int streakTextY = centerY - 150 - streakScaledHeight / 2;
        
        // 背景を描画（強調）
        graphics.setDrawColor(255, 200, 0, 200);
        graphics.drawRect(streakTextX - 20, streakTextY - 20, streakScaledWidth + 40, streakScaledHeight + 40, true);
        
        // 光る効果
        float glowProgress = std::sin(resultAnimationTimer * 3.14159f * 6.0f) * 0.5f + 0.5f;
        graphics.setDrawColor(255, 255, 0, (Uint8)(glowProgress * 150));
        graphics.drawRect(streakTextX - 30, streakTextY - 30, streakScaledWidth + 60, streakScaledHeight + 60, false);
        graphics.drawRect(streakTextX - 25, streakTextY - 25, streakScaledWidth + 50, streakScaledHeight + 50, false);
        
        // テキストを描画
        graphics.drawText(streakText, streakTextX, streakTextY, "default", streakColor);
    }
    
    // メインテキストを描画
    graphics.drawText(mainText, textX, textY, "default", mainColor);
    
    // スコア表示（下に表示）
    std::string scoreText = "自分 " + std::to_string(playerWins) + "勝  " + 
                           "敵 " + std::to_string(enemyWins) + "勝";
    SDL_Color scoreColor = {255, 255, 255, 255};
    graphics.drawText(scoreText, centerX - 150, centerY + 50, "default", scoreColor);
    
    // 3連勝の場合は追加メッセージ
    if (hasThreeWinStreak && isVictory) {
        std::string bonusText = "✨ ダメージ1.5倍ボーナス！ ✨";
        SDL_Color bonusColor = {255, 255, 100, 255}; // 明るい黄色
        graphics.drawText(bonusText, centerX - 150, centerY + 80, "default", bonusColor);
    }
    
    // 詳細テキスト
    std::string detailText;
    if (isVictory) {
        detailText = "自分が" + std::to_string(playerWins) + "ターン分の攻撃を実行！";
    } else if (isDefeat) {
        detailText = "敵が" + std::to_string(enemyWins) + "ターン分の攻撃を実行！";
    } else {
        detailText = "両方がダメージを受ける";
    }
    SDL_Color detailColor = {200, 200, 200, 255};
    graphics.drawText(detailText, centerX - 200, centerY + 100, "default", detailColor);
    
    // 勝利時の追加演出（星やパーティクル風の効果）
    if (isVictory) {
        // 星の効果（簡易版：テキストで表現）
        float starGlow = std::sin(resultAnimationTimer * 3.14159f * 8.0f) * 0.5f + 0.5f;
        SDL_Color starColor = {255, 255, 0, (Uint8)(starGlow * 255)};
        
        // 周囲に星を配置
        int starRadius = 150;
        float displayScale = std::max(0.1f, resultScale); // 最小10%のサイズ
        for (int i = 0; i < 8; i++) {
            float angle = (i * 3.14159f * 2.0f) / 8.0f;
            int starX = centerX + (int)(std::cos(angle) * starRadius * displayScale);
            int starY = centerY - 100 + (int)(std::sin(angle) * starRadius * displayScale);
            graphics.drawText("★", starX, starY, "default", starColor);
        }
    }
    
    // 敗北時の追加演出（暗い効果）
    if (isDefeat) {
        // 暗いオーバーレイ
        float darkIntensity = std::sin(resultAnimationTimer * 3.14159f * 2.0f) * 0.2f + 0.3f;
        graphics.setDrawColor(0, 0, 0, (Uint8)(darkIntensity * 100));
        graphics.drawRect(0, 0, screenWidth, screenHeight, true);
    }
    
    // キャラクターの描画（アニメーション付き、大きく表示）
    // プレイヤーの描画（左側、固定位置）
    int playerBaseX = screenWidth / 4; // 左側1/4の位置
    int playerBaseY = screenHeight / 2; // 中央の高さ
    int playerX = playerBaseX + (int)playerAttackOffsetX + (int)playerHitOffsetX;
    int playerY = playerBaseY + (int)playerAttackOffsetY + (int)playerHitOffsetY;
    
    // 3倍のサイズ
    int playerWidth = 300; // 通常100px × 3
    int playerHeight = 300; // 通常100px × 3
    
    // プレイヤーのHP表示（頭上）
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    SDL_Color playerHpColor = {100, 255, 100, 255};
    graphics.drawText(playerHpText, playerX - 100, playerY - playerHeight / 2 - 40, "default", playerHpColor);
    
    SDL_Texture* playerTex = graphics.getTexture("player");
    if (playerTex) {
        graphics.drawTexture(playerTex, playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics.setDrawColor(100, 200, 255, 255);
        graphics.drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    // 敵の描画（右側、固定位置）
    int enemyBaseX = screenWidth * 3 / 4; // 右側3/4の位置
    int enemyBaseY = screenHeight / 2; // 中央の高さ
    int enemyX = enemyBaseX + (int)enemyAttackOffsetX + (int)enemyHitOffsetX;
    int enemyY = enemyBaseY + (int)enemyAttackOffsetY + (int)enemyHitOffsetY;
    
    // 3倍のサイズ
    int enemyWidth = 300; // 通常100px × 3
    int enemyHeight = 300; // 通常100px × 3
    
    // 敵のHP表示（頭上）
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    graphics.drawText(enemyHpText, enemyX - 100, enemyY - enemyHeight / 2 - 40, "default", enemyHpColor);
    
    SDL_Texture* enemyTex = graphics.getTexture("enemy_" + enemy->getTypeName());
    if (enemyTex) {
        graphics.drawTexture(enemyTex, enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics.setDrawColor(255, 100, 100, 255);
        graphics.drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
}

void BattleState::updateResultCharacterAnimation(float /* deltaTime */) {
    bool isVictory = (playerWins > enemyWins);
    bool isDefeat = (enemyWins > playerWins);
    
    // アニメーションのタイミング（結果発表開始から0.5秒後から1.5秒まで）
    float animStartTime = 0.5f;
    float animDuration = 1.0f;
    float animTime = resultAnimationTimer - animStartTime;
    
    if (animTime < 0.0f) {
        // アニメーション開始前：リセット
        playerAttackOffsetX = 0.0f;
        playerAttackOffsetY = 0.0f;
        enemyAttackOffsetX = 0.0f;
        enemyAttackOffsetY = 0.0f;
        enemyHitOffsetX = 0.0f;
        enemyHitOffsetY = 0.0f;
        playerHitOffsetX = 0.0f;
        playerHitOffsetY = 0.0f;
        attackAnimationFrame = 0;
        return;
    }
    
    if (animTime > animDuration) {
        // アニメーション終了後：リセット
        playerAttackOffsetX = 0.0f;
        playerAttackOffsetY = 0.0f;
        enemyAttackOffsetX = 0.0f;
        enemyAttackOffsetY = 0.0f;
        enemyHitOffsetX = 0.0f;
        enemyHitOffsetY = 0.0f;
        playerHitOffsetX = 0.0f;
        playerHitOffsetY = 0.0f;
        return;
    }
    
    // アニメーション進行度（0.0から1.0）
    float progress = animTime / animDuration;
    
    if (isVictory) {
        // プレイヤーが敵を殴る（激しく、爽快感のあるアニメーション）
        // パンチ動作：前に大きく移動して戻る
        if (progress < 0.2f) {
            // 前に大きく移動（パンチ）
            float punchProgress = progress / 0.2f;
            // イージング関数でより激しく
            float eased = punchProgress * punchProgress * (3.0f - 2.0f * punchProgress);
            playerAttackOffsetX = 200.0f * eased; // 右に200px移動（より大きく）
            playerAttackOffsetY = -40.0f * std::sin(punchProgress * 3.14159f); // 大きく上に跳ねる
        } else if (progress < 0.4f) {
            // 戻る（バウンス効果付き）
            float returnProgress = (progress - 0.2f) / 0.2f;
            float bounce = std::sin(returnProgress * 3.14159f) * 0.3f; // バウンス効果
            playerAttackOffsetX = 200.0f * (1.0f - returnProgress) + bounce * 20.0f;
            playerAttackOffsetY = -10.0f * std::sin(returnProgress * 3.14159f);
        } else {
            playerAttackOffsetX = 0.0f;
            playerAttackOffsetY = 0.0f;
        }
        
        // 敵がダメージを受ける（激しく後ろに吹き飛ぶ）
        if (progress >= 0.15f && progress < 0.4f) {
            // ダメージを受ける瞬間（より激しく）
            float hitProgress = (progress - 0.15f) / 0.25f;
            float impact = std::sin(hitProgress * 3.14159f);
            enemyHitOffsetX = 80.0f * (1.0f - hitProgress * 0.7f); // 右に80px移動（より大きく）
            enemyHitOffsetY = 30.0f * impact * (1.0f - hitProgress * 0.5f); // 大きく上下に揺れる
        } else if (progress >= 0.4f && progress < 0.7f) {
            // 激しく揺れ続ける
            float shakeProgress = (progress - 0.4f) / 0.3f;
            enemyHitOffsetX = 15.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            enemyHitOffsetY = 15.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
        } else {
            enemyHitOffsetX = 0.0f;
            enemyHitOffsetY = 0.0f;
        }
        
        // アニメーションフレーム更新
        attackAnimationFrame = (int)(progress * 10.0f) % 3;
        
    } else if (isDefeat) {
        // 敵がプレイヤーを殴る（激しく、爽快感のあるアニメーション）
        // パンチ動作：左に大きく移動して戻る
        if (progress < 0.2f) {
            // 前に大きく移動（パンチ）
            float punchProgress = progress / 0.2f;
            // イージング関数でより激しく
            float eased = punchProgress * punchProgress * (3.0f - 2.0f * punchProgress);
            enemyAttackOffsetX = -200.0f * eased; // 左に200px移動（より大きく）
            enemyAttackOffsetY = -40.0f * std::sin(punchProgress * 3.14159f); // 大きく上に跳ねる
        } else if (progress < 0.4f) {
            // 戻る（バウンス効果付き）
            float returnProgress = (progress - 0.2f) / 0.2f;
            float bounce = std::sin(returnProgress * 3.14159f) * 0.3f; // バウンス効果
            enemyAttackOffsetX = -200.0f * (1.0f - returnProgress) - bounce * 20.0f;
            enemyAttackOffsetY = -10.0f * std::sin(returnProgress * 3.14159f);
        } else {
            enemyAttackOffsetX = 0.0f;
            enemyAttackOffsetY = 0.0f;
        }
        
        // プレイヤーがダメージを受ける（激しく後ろに吹き飛ぶ）
        if (progress >= 0.15f && progress < 0.4f) {
            // ダメージを受ける瞬間（より激しく）
            float hitProgress = (progress - 0.15f) / 0.25f;
            float impact = std::sin(hitProgress * 3.14159f);
            playerHitOffsetX = -80.0f * (1.0f - hitProgress * 0.7f); // 左に80px移動（より大きく）
            playerHitOffsetY = 30.0f * impact * (1.0f - hitProgress * 0.5f); // 大きく上下に揺れる
        } else if (progress >= 0.4f && progress < 0.7f) {
            // 激しく揺れ続ける
            float shakeProgress = (progress - 0.4f) / 0.3f;
            playerHitOffsetX = -15.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            playerHitOffsetY = 15.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
        } else {
            playerHitOffsetX = 0.0f;
            playerHitOffsetY = 0.0f;
        }
        
        // アニメーションフレーム更新
        attackAnimationFrame = (int)(progress * 10.0f) % 3;
        
    } else {
        // 引き分け：両方が攻撃する（激しく）
        // プレイヤーの攻撃
        if (progress < 0.2f) {
            float punchProgress = progress / 0.2f;
            float eased = punchProgress * punchProgress;
            playerAttackOffsetX = 120.0f * eased; // より大きく
            playerAttackOffsetY = -30.0f * std::sin(punchProgress * 3.14159f);
        } else if (progress < 0.4f) {
            float returnProgress = (progress - 0.2f) / 0.2f;
            playerAttackOffsetX = 120.0f * (1.0f - returnProgress);
            playerAttackOffsetY = 0.0f;
        } else {
            playerAttackOffsetX = 0.0f;
            playerAttackOffsetY = 0.0f;
        }
        
        // 敵の攻撃
        if (progress < 0.2f) {
            float punchProgress = progress / 0.2f;
            float eased = punchProgress * punchProgress;
            enemyAttackOffsetX = -120.0f * eased; // より大きく
            enemyAttackOffsetY = -30.0f * std::sin(punchProgress * 3.14159f);
        } else if (progress < 0.4f) {
            float returnProgress = (progress - 0.2f) / 0.2f;
            enemyAttackOffsetX = -120.0f * (1.0f - returnProgress);
            enemyAttackOffsetY = 0.0f;
        } else {
            enemyAttackOffsetX = 0.0f;
            enemyAttackOffsetY = 0.0f;
        }
        
        // 両方がダメージを受ける（激しく）
        if (progress >= 0.15f && progress < 0.4f) {
            float hitProgress = (progress - 0.15f) / 0.25f;
            float impact = std::sin(hitProgress * 3.14159f);
            playerHitOffsetX = -50.0f * (1.0f - hitProgress * 0.7f);
            playerHitOffsetY = 20.0f * impact * (1.0f - hitProgress * 0.5f);
            enemyHitOffsetX = 50.0f * (1.0f - hitProgress * 0.7f);
            enemyHitOffsetY = 20.0f * impact * (1.0f - hitProgress * 0.5f);
        } else if (progress >= 0.4f && progress < 0.7f) {
            float shakeProgress = (progress - 0.4f) / 0.3f;
            playerHitOffsetX = -10.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            playerHitOffsetY = 10.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
            enemyHitOffsetX = 10.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            enemyHitOffsetY = 10.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
        } else {
            playerHitOffsetX = 0.0f;
            playerHitOffsetY = 0.0f;
            enemyHitOffsetX = 0.0f;
            enemyHitOffsetY = 0.0f;
        }
    }
} 