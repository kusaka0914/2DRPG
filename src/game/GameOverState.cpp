#include "GameOverState.h"
#include "MainMenuState.h"
#include "NightState.h"
#include "FieldState.h"
#include "TownState.h"
#include "BattleState.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "../core/utils/ui_config_manager.h"
#include "../core/AudioManager.h"
#include <iostream>

GameOverState::GameOverState(std::shared_ptr<Player> player, const std::string& reason, 
                             EnemyType enemyType, int enemyLevel,
                             bool isResident, const std::string& residentName,
                             int residentX, int residentY, int residentTextureIndex,
                             bool isTargetLevelEnemy)
    : player(player), gameOverReason(reason), 
      battleEnemyType(enemyType), battleEnemyLevel(enemyLevel),
      isResidentBattle(isResident), residentName(residentName),
      residentX(residentX), residentY(residentY), residentTextureIndex(residentTextureIndex),
      isTargetLevelEnemy(isTargetLevelEnemy),
      titleLabel(nullptr), reasonLabel(nullptr), instruction(nullptr),
      retryLabel(nullptr), extendTimeLabel(nullptr) {
    // 戦闘に敗北した場合または住民戦でゲームオーバーになった場合のみ敵情報を有効にする
    hasBattleEnemyInfo = (gameOverReason.find("戦闘に敗北") != std::string::npos) || isResidentBattle;
}

void GameOverState::enter() {
    setupUI();
    // ゲームオーバー時は gameover.ogg を再生
    AudioManager::getInstance().stopMusic();
    AudioManager::getInstance().playMusic("gameover", -1);
}

void GameOverState::exit() {
    
}

void GameOverState::update(float deltaTime) {
    ui.update(deltaTime);
    
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    if (!lastReloadState && currentReloadState) {
        setupUI();
    }
    lastReloadState = currentReloadState;
}

void GameOverState::render(Graphics& graphics) {
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
    // 通常戦の場合はbattle_bg.pngを背景に、住民戦の場合は黒背景
    if (!isResidentBattle) {
        // 通常戦：battle_bg.pngを背景に表示
        SDL_Texture* bgTexture = graphics.getTexture("battle_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        } else {
            // フォールバック：黒背景
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(0, 0, screenWidth, screenHeight, true);
        }
        
        // 画面の中心にplayer_defeat.pngを表示
        SDL_Texture* defeatTexture = graphics.getTexture("player_defeat");
        if (!defeatTexture) {
            defeatTexture = graphics.loadTexture("assets/textures/characters/player_defeat.png", "player_defeat");
        }
        if (defeatTexture) {
            int imgWidth, imgHeight;
            SDL_QueryTexture(defeatTexture, nullptr, nullptr, &imgWidth, &imgHeight);
            
            // アスペクト比を維持しながら適切なサイズで表示
            auto& config = UIConfig::UIConfigManager::getInstance();
            auto gameOverConfig = config.getGameOverConfig();
            int baseSize = gameOverConfig.image.baseSize;
            float aspectRatio = static_cast<float>(imgWidth) / static_cast<float>(imgHeight);
            int displayWidth, displayHeight;
            if (imgWidth > imgHeight) {
                displayWidth = baseSize;
                displayHeight = static_cast<int>(baseSize / aspectRatio);
            } else {
                displayHeight = baseSize;
                displayWidth = static_cast<int>(baseSize * aspectRatio);
            }
            
            // 画面の中心に配置
            int centerX = screenWidth / 2;
            int centerY = screenHeight / 2;
            int imageX = centerX - displayWidth / 2;
            int imageY = centerY - displayHeight / 2;
            
            graphics.drawTexture(defeatTexture, imageX, imageY, displayWidth, displayHeight);
        }
    } else {
        // 住民戦：night_bg.pngを背景に表示
        SDL_Texture* bgTexture = graphics.getTexture("night_bg");
        if (!bgTexture) {
            bgTexture = graphics.loadTexture("assets/textures/bg/night_bg.png", "night_bg");
        }
        if (bgTexture) {
            graphics.drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
        } else {
            // フォールバック：黒背景
    graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(0, 0, screenWidth, screenHeight, true);
        }
        
        // 画面の中心にplayer_captured.pngを表示
        SDL_Texture* capturedTexture = graphics.getTexture("player_captured");
        if (!capturedTexture) {
            capturedTexture = graphics.loadTexture("assets/textures/characters/player_captured.png", "player_captured");
        }
        if (capturedTexture) {
            int imgWidth, imgHeight;
            SDL_QueryTexture(capturedTexture, nullptr, nullptr, &imgWidth, &imgHeight);
            
            // アスペクト比を維持しながら適切なサイズで表示
            auto& config = UIConfig::UIConfigManager::getInstance();
            auto gameOverConfig = config.getGameOverConfig();
            int baseSize = gameOverConfig.image.baseSize;
            float aspectRatio = static_cast<float>(imgWidth) / static_cast<float>(imgHeight);
            int displayWidth, displayHeight;
            if (imgWidth > imgHeight) {
                displayWidth = baseSize;
                displayHeight = static_cast<int>(baseSize / aspectRatio);
            } else {
                displayHeight = baseSize;
                displayWidth = static_cast<int>(baseSize * aspectRatio);
            }
            
            // 画面の中心に配置
            int centerX = screenWidth / 2;
            int centerY = screenHeight / 2;
            int imageX = centerX - displayWidth / 2;
            int imageY = centerY - displayHeight / 2;
            
            graphics.drawTexture(capturedTexture, imageX, imageY, displayWidth, displayHeight);
        }
    }
    
    // テキストUIの背景を描画
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto gameOverConfig = config.getGameOverConfig();
    
    // title背景
    if (titleLabel && !titleLabel->getText().empty()) {
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, gameOverConfig.title.background.position, screenWidth, screenHeight);
        graphics.setDrawColor(gameOverConfig.title.backgroundColor.r, gameOverConfig.title.backgroundColor.g, 
                             gameOverConfig.title.backgroundColor.b, gameOverConfig.title.backgroundColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.title.background.width, gameOverConfig.title.background.height, true);
        graphics.setDrawColor(gameOverConfig.title.borderColor.r, gameOverConfig.title.borderColor.g, 
                             gameOverConfig.title.borderColor.b, gameOverConfig.title.borderColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.title.background.width, gameOverConfig.title.background.height);
    }
    
    // reason背景
    if (reasonLabel && !reasonLabel->getText().empty()) {
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, gameOverConfig.reason.background.position, screenWidth, screenHeight);
        graphics.setDrawColor(gameOverConfig.reason.backgroundColor.r, gameOverConfig.reason.backgroundColor.g, 
                             gameOverConfig.reason.backgroundColor.b, gameOverConfig.reason.backgroundColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.reason.background.width, gameOverConfig.reason.background.height, true);
        graphics.setDrawColor(gameOverConfig.reason.borderColor.r, gameOverConfig.reason.borderColor.g, 
                             gameOverConfig.reason.borderColor.b, gameOverConfig.reason.borderColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.reason.background.width, gameOverConfig.reason.background.height);
    }
    
    // instruction背景
    if (instruction && !instruction->getText().empty()) {
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, gameOverConfig.instruction.background.position, screenWidth, screenHeight);
        graphics.setDrawColor(gameOverConfig.instruction.backgroundColor.r, gameOverConfig.instruction.backgroundColor.g, 
                             gameOverConfig.instruction.backgroundColor.b, gameOverConfig.instruction.backgroundColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.instruction.background.width, gameOverConfig.instruction.background.height, true);
        graphics.setDrawColor(gameOverConfig.instruction.borderColor.r, gameOverConfig.instruction.borderColor.g, 
                             gameOverConfig.instruction.borderColor.b, gameOverConfig.instruction.borderColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.instruction.background.width, gameOverConfig.instruction.background.height);
    }
    
    // retry背景
    if (retryLabel && !retryLabel->getText().empty()) {
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, gameOverConfig.retry.background.position, screenWidth, screenHeight);
        graphics.setDrawColor(gameOverConfig.retry.backgroundColor.r, gameOverConfig.retry.backgroundColor.g, 
                             gameOverConfig.retry.backgroundColor.b, gameOverConfig.retry.backgroundColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.retry.background.width, gameOverConfig.retry.background.height, true);
        graphics.setDrawColor(gameOverConfig.retry.borderColor.r, gameOverConfig.retry.borderColor.g, 
                             gameOverConfig.retry.borderColor.b, gameOverConfig.retry.borderColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.retry.background.width, gameOverConfig.retry.background.height);
    }
    
    // extendTime背景
    if (extendTimeLabel && !extendTimeLabel->getText().empty()) {
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, gameOverConfig.extendTime.background.position, screenWidth, screenHeight);
        graphics.setDrawColor(gameOverConfig.extendTime.backgroundColor.r, gameOverConfig.extendTime.backgroundColor.g, 
                             gameOverConfig.extendTime.backgroundColor.b, gameOverConfig.extendTime.backgroundColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.extendTime.background.width, gameOverConfig.extendTime.background.height, true);
        graphics.setDrawColor(gameOverConfig.extendTime.borderColor.r, gameOverConfig.extendTime.borderColor.g, 
                             gameOverConfig.extendTime.borderColor.b, gameOverConfig.extendTime.borderColor.a);
        graphics.drawRect(bgX, bgY, gameOverConfig.extendTime.background.width, gameOverConfig.extendTime.background.height);
    }
    
    ui.render(graphics);
    
    graphics.present();
}

void GameOverState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // 目標レベル達成用の敵に負けた場合の特別な処理
    if (isTargetLevelEnemy) {
        if (input.isKeyJustPressed(InputKey::R)) {
            // Rキーで再戦
            if (stateManager) {
                float nightTimer;
                bool nightTimerActive;
                if (player->autoLoad(nightTimer, nightTimerActive)) {
                    player->heal(player->getMaxHp());
                    player->restoreMp(player->getMaxMp());
                    
                    // 目標レベルの敵との戦闘を再開
                    int targetLevel = TownState::s_targetLevel;
                    auto enemy = std::make_unique<Enemy>(Enemy::createTargetLevelEnemy(targetLevel));
                    auto battleState = std::make_unique<BattleState>(player, std::move(enemy));
                    // 目標レベル達成用の敵フラグを明示的に設定
                    battleState->setIsTargetLevelEnemy(true);
                    stateManager->changeState(std::move(battleState));
                    
                    TownState::s_nightTimerActive = nightTimerActive;
                    TownState::s_nightTimer = nightTimer;
                } else {
                    auto newPlayer = std::make_shared<Player>("勇者");
                    newPlayer->setKingTrust(50);
                    newPlayer->setCurrentNight(1);
                    stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
                }
            }
        } else if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            // Enterキーで3分延長してフィールドに戻る
            if (stateManager) {
                float nightTimer;
                bool nightTimerActive;
                if (player->autoLoad(nightTimer, nightTimerActive)) {
                    player->heal(player->getMaxHp());
                    player->restoreMp(player->getMaxMp());
                    
                    // タイマーを3分（180秒）延長
                    TownState::s_nightTimerActive = true;
                    TownState::s_nightTimer = 180.0f; // 3分 = 180秒
                    
                    // フィールドに戻る
                    stateManager->changeState(std::make_unique<FieldState>(player));
                } else {
                    auto newPlayer = std::make_shared<Player>("勇者");
                    newPlayer->setKingTrust(50);
                    newPlayer->setCurrentNight(1);
                    stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
                }
            }
        }
        return; // 目標レベル達成用の敵に負けた場合は、通常の処理をスキップ
    }
    
    // 通常の処理
    if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (stateManager) {
            float nightTimer;
            bool nightTimerActive;
            if (player->autoLoad(nightTimer, nightTimerActive)) {
                // デバッグ出力
                std::cout << "GameOverState: タイマー情報を復元 - Timer: " << nightTimer 
                          << ", Active: " << (nightTimerActive ? "true" : "false") << std::endl;
                
                player->heal(player->getMaxHp());
                player->restoreMp(player->getMaxMp());
                
                std::cout << "GameOverState: プレイヤーHP復元 - HP: " << player->getHp() 
                          << "/" << player->getMaxHp() << ", isAlive: " << (player->getIsAlive() ? "true" : "false") << std::endl;
                
                if (gameOverReason.find("戦闘に敗北") != std::string::npos || isResidentBattle) {
                    // 再戦可能な場合（敵情報がある場合）は再戦、そうでなければFieldStateに戻る
                    if (hasBattleEnemyInfo) {
                        // プレイヤーのHP/MPを回復してから再戦
                        player->heal(player->getMaxHp());
                        player->restoreMp(player->getMaxMp());
                        
                        // 住民戦の場合は住民の情報を復元して再戦
                        if (isResidentBattle) {
                            auto enemy = std::make_unique<Enemy>(EnemyType::SLIME);
                            enemy->setName(residentName);
                            enemy->setResidentTextureIndex(residentTextureIndex);
                            enemy->setResidentPosition(residentX, residentY);
                            stateManager->changeState(std::make_unique<BattleState>(player, std::move(enemy)));
                        } else {
                            // 通常の敵との戦闘を再開
                            auto enemy = std::make_unique<Enemy>(battleEnemyType);
                        // 敵のレベルを目標レベルまで上げる
                            enemy->setLevel(battleEnemyLevel);
                            stateManager->changeState(std::make_unique<BattleState>(player, std::move(enemy)));
                        }
                    } else {
                        stateManager->changeState(std::make_unique<FieldState>(player));
                    }
                } else {
                    stateManager->changeState(std::make_unique<NightState>(player));
                }
                
                TownState::s_nightTimerActive = nightTimerActive;
                TownState::s_nightTimer = nightTimer;
                
                std::cout << "GameOverState: 静的変数に復元 - s_nightTimer: " << TownState::s_nightTimer 
                          << ", s_nightTimerActive: " << (TownState::s_nightTimerActive ? "true" : "false") << std::endl;
            } else {
                auto newPlayer = std::make_shared<Player>("勇者");
                newPlayer->setKingTrust(50);
                newPlayer->setCurrentNight(1);
                stateManager->changeState(std::make_unique<MainMenuState>(newPlayer));
            }
        }
    }
}

void GameOverState::setupUI() {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto gameOverConfig = config.getGameOverConfig();
    
    int titleX, titleY;
    config.calculatePosition(titleX, titleY, gameOverConfig.title.text.position, 1100, 650);
    auto titleLabelPtr = std::make_unique<Label>(titleX, titleY, "GAME OVER", "default");
    titleLabelPtr->setColor(gameOverConfig.title.text.color);
    titleLabel = titleLabelPtr.get();
    ui.addElement(std::move(titleLabelPtr));
    
    int reasonX, reasonY;
    config.calculatePosition(reasonX, reasonY, gameOverConfig.reason.text.position, 1100, 650);
    auto reasonLabelPtr = std::make_unique<Label>(reasonX, reasonY, gameOverReason, "default");
    reasonLabelPtr->setColor(gameOverConfig.reason.text.color);
    reasonLabel = reasonLabelPtr.get();
    ui.addElement(std::move(reasonLabelPtr));
    
    int instructionX, instructionY;
    config.calculatePosition(instructionX, instructionY, gameOverConfig.instruction.text.position, 1100, 650);
    std::string instructionText;
    if (isTargetLevelEnemy) {
        // 目標レベル達成用の敵の場合は、retryとextendTimeを別々に表示
        instructionText = "";  // instructionは表示しない
    } else if (hasBattleEnemyInfo) {
        instructionText = "再挑戦 : ENTER";
    } else {
        instructionText = "再挑戦 : ENTER";
    }
    auto instructionLabelPtr = std::make_unique<Label>(instructionX, instructionY, instructionText, "default");
    instructionLabelPtr->setColor(gameOverConfig.instruction.text.color);
    instruction = instructionLabelPtr.get();
    ui.addElement(std::move(instructionLabelPtr));
    
    // 目標レベル達成用の敵の場合は、retryとextendTimeを別々に表示
    if (isTargetLevelEnemy) {
        int retryX, retryY;
        config.calculatePosition(retryX, retryY, gameOverConfig.retry.text.position, 1100, 650);
        auto retryLabelPtr = std::make_unique<Label>(retryX, retryY, "再挑戦 : Rキー", "default");
        retryLabelPtr->setColor(gameOverConfig.retry.text.color);
        retryLabel = retryLabelPtr.get();
        ui.addElement(std::move(retryLabelPtr));
        
        int extendTimeX, extendTimeY;
        config.calculatePosition(extendTimeX, extendTimeY, gameOverConfig.extendTime.text.position, 1100, 650);
        auto extendTimeLabelPtr = std::make_unique<Label>(extendTimeX, extendTimeY, "3分延長してフィールドに戻る : ENTER", "default");
        extendTimeLabelPtr->setColor(gameOverConfig.extendTime.text.color);
        extendTimeLabel = extendTimeLabelPtr.get();
        ui.addElement(std::move(extendTimeLabelPtr));
    } else {
        retryLabel = nullptr;
        extendTimeLabel = nullptr;
    }
} 