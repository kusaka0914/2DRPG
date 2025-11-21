#include "BattleUI.h"
#include "BattleConstants.h"
#include "../core/utils/ui_config_manager.h"
#include <cmath>
#include <algorithm>
#include <random>

BattleUI::BattleUI(Graphics* graphics, std::shared_ptr<Player> player, Enemy* enemy,
                   BattleLogic* battleLogic, BattleAnimationController* animationController)
    : graphics(graphics), player(player), enemy(enemy),
      battleLogic(battleLogic), animationController(animationController),
      hasUsedLastChanceMode(false) {
}

void BattleUI::setHasUsedLastChanceMode(bool hasUsed) {
    hasUsedLastChanceMode = hasUsed;
}

void BattleUI::renderJudgeAnimation(const JudgeRenderParams& params) {
    if (params.currentJudgingTurnIndex >= params.commandTurnCount) return;
    
    int screenWidth = graphics->getScreenWidth();
    int screenHeight = graphics->getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // JSONからプレイヤーと敵の位置を取得
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    
    int playerBaseX, playerBaseY;
    config.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
    
    int enemyBaseX, enemyBaseY;
    config.calculatePosition(enemyBaseX, enemyBaseY, battleConfig.enemyPosition, screenWidth, screenHeight);
    
    int leftX = playerBaseX;
    int rightX = enemyBaseX;
    
    // 画面をクリア（背景画像で覆う前に）
    graphics->setDrawColor(0, 0, 0, 255);
    graphics->clear();
    
    // 背景画像を描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
    SDL_Texture* bgTexture = getBattleBackgroundTexture();
    if (bgTexture) {
        graphics->drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
    }
    
    // オーバーレイを削除して背景画像が見えるようにする
    // graphics->setDrawColor(0, 0, 0, 100);
    // graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    
    // 窮地モードではplayer_adversity.pngを使用
    SDL_Texture* playerTex = hasUsedLastChanceMode ? graphics->getTexture("player_adversity") : graphics->getTexture("player");
    if (!playerTex && hasUsedLastChanceMode) {
        playerTex = graphics->getTexture("player"); // フォールバック
    }
    
    if (playerTex) {
        graphics->drawTextureAspectRatio(playerTex, playerBaseX, playerBaseY, BattleConstants::BATTLE_CHARACTER_SIZE);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerBaseX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerBaseY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
    }
    
    // 住民の場合は住民の画像を使用、それ以外は通常の敵画像を使用
    SDL_Texture* enemyTex = nullptr;
    if (enemy->isResident()) {
        int textureIndex = enemy->getResidentTextureIndex();
        std::string textureName = "resident_" + std::to_string(textureIndex + 1);
        enemyTex = graphics->getTexture(textureName);
    } else {
        enemyTex = graphics->getTexture("enemy_" + enemy->getTypeName());
    }
    
    if (enemyTex) {
        graphics->drawTextureAspectRatio(enemyTex, enemyBaseX, enemyBaseY, BattleConstants::BATTLE_CHARACTER_SIZE);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyBaseX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, enemyBaseY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
    }
    
    // 住民戦の場合は10ターン制限を表示
    std::string turnText;
    if (params.residentTurnCount > 0) {
        turnText = "ターン " + std::to_string(params.residentTurnCount) + " / 10";
    } else {
        turnText = "ターン " + std::to_string(params.currentJudgingTurnIndex + 1) + " / " + std::to_string(params.commandTurnCount);
    }
    SDL_Color turnColor = {255, 255, 255, 255};
    
    // ターンテキストの背景を描画（コマンド選択フェーズと同じ位置）
    SDL_Texture* turnTexture = graphics->createTextTexture(turnText, "default", turnColor);
    if (turnTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(turnTexture, nullptr, nullptr, &textWidth, &textHeight);
        
        // コマンド選択フェーズと同じ位置に設定
        int turnNumberY = 70; // コマンド選択フェーズと同じ位置
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        int bgX = 18 - padding; // コマンド選択フェーズと同じ位置
        int bgY = turnNumberY - padding;
        int bgWidth = textWidth + padding * 2;
        int bgHeight = textHeight + padding * 2;
        
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
        
        SDL_DestroyTexture(turnTexture);
    }
    
    // コマンド選択フェーズと同じ位置にテキストを描画
    int turnNumberY = 70; // コマンド選択フェーズと同じ位置
    graphics->drawText(turnText, 18, turnNumberY, "default", turnColor);
    
    int hitCount = params.residentHitCount;
    renderHP(playerBaseX, playerBaseY, enemyBaseX, enemyBaseY, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, params.residentBehaviorHint, false, hitCount);
    
    int playerDisplayX = leftX;
    if (params.judgeSubPhase == JudgeSubPhase::SHOW_PLAYER_COMMAND) {
        float slideProgress = std::min(1.0f, params.judgeDisplayTimer / BattleConstants::JUDGE_COMMAND_SLIDE_ANIMATION_DURATION);
        int slideOffset = (int)((1.0f - slideProgress) * BattleConstants::JUDGE_COMMAND_SLIDE_OFFSET);
        playerDisplayX = leftX - slideOffset;
    }
    
    if (params.judgeSubPhase >= JudgeSubPhase::SHOW_PLAYER_COMMAND) {
        std::string playerCmd;
        if (!params.playerCommandName.empty()) {
            // 住民戦用：コマンド名を直接使用
            playerCmd = params.playerCommandName;
        } else {
            // 通常戦闘：battleLogicから取得
            auto playerCmds = battleLogic->getPlayerCommands();
            playerCmd = BattleLogic::getCommandName(playerCmds[params.currentJudgingTurnIndex]);
        }
        SDL_Color playerCmdColor = {255, 255, 255, 255}; // 白いテキスト
        int cmdTextX = playerDisplayX - BattleConstants::JUDGE_COMMAND_X_OFFSET;
        int cmdTextY = centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET;
        
        // プレイヤーコマンドを画像で表示
        SDL_Texture* playerCmdImage = getCommandTexture(playerCmd);
        if (playerCmdImage) {
            int imageWidth, imageHeight;
            SDL_QueryTexture(playerCmdImage, nullptr, nullptr, &imageWidth, &imageHeight);
            
            // 画像を適切なサイズで表示（80px幅に固定、アスペクト比を保持）
            float aspectRatio = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
            int displayWidth, displayHeight;
            if (imageWidth > imageHeight) {
                // 横長の画像
                displayWidth = 80;
                displayHeight = static_cast<int>(80 / aspectRatio);
            } else {
                // 縦長または正方形の画像
                displayHeight = 80;
                displayWidth = static_cast<int>(80 * aspectRatio);
            }
            
            // 画像を中央に配置
            int imageX = cmdTextX - (displayWidth / 2)+300;
            int imageY = cmdTextY - (displayHeight / 2)-120;
            
            // 画像を描画（背景なし）
            graphics->drawTexture(playerCmdImage, imageX, imageY, displayWidth, displayHeight);
        } else {
            // フォールバック：テキスト表示
        SDL_Texture* playerCmdTexture = graphics->createTextTexture(playerCmd, "default", playerCmdColor);
        if (playerCmdTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerCmdTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            int scaledWidth = static_cast<int>(textWidth * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
            int scaledHeight = static_cast<int>(textHeight * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
            
            int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_LARGE;
            int bgX = cmdTextX - padding;
            int bgY = cmdTextY - padding;
            int bgWidth = scaledWidth + padding * 2;
            int bgHeight = scaledHeight + padding * 2;
            
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
            
            graphics->drawTexture(playerCmdTexture, cmdTextX, cmdTextY, scaledWidth, scaledHeight);
            SDL_DestroyTexture(playerCmdTexture);
        } else {
            graphics->drawText(playerCmd, cmdTextX, cmdTextY, "default", playerCmdColor);
            }
        }
    }
    
    int enemyDisplayX = rightX;
    if (params.judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        float slideProgress = std::min(1.0f, params.judgeDisplayTimer / BattleConstants::JUDGE_COMMAND_SLIDE_ANIMATION_DURATION);
        int slideOffset = (int)((1.0f - slideProgress) * BattleConstants::JUDGE_COMMAND_SLIDE_OFFSET);
        enemyDisplayX = rightX + slideOffset;
    } else if (params.judgeSubPhase >= JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        // SHOW_RESULTフェーズでも敵の位置は固定
        enemyDisplayX = rightX;
    }
    
    if (params.judgeSubPhase >= JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        // VSは常に表示（SHOW_ENEMY_COMMANDフェーズとSHOW_RESULTフェーズの両方で表示）
        bool shouldShowVS = true;
        
        if (shouldShowVS) {
            // VS画像を表示
            SDL_Texture* vsImage = graphics->getTexture("vs_image");
            if (vsImage) {
                int imageWidth, imageHeight;
                SDL_QueryTexture(vsImage, nullptr, nullptr, &imageWidth, &imageHeight);
                
                // 画像をコマンド画像と同じサイズで表示（80px幅に固定）
                float aspectRatio = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
                int displayWidth, displayHeight;
                if (imageWidth > imageHeight) {
                    // 横長の画像
                    displayWidth = 80;
                    displayHeight = static_cast<int>(80 / aspectRatio);
                } else {
                    // 縦長または正方形の画像
                    displayHeight = 80;
                    displayWidth = static_cast<int>(80 * aspectRatio);
                }
                
                // 中央に配置（コマンドと同じY位置）
                int vsX = centerX - displayWidth / 2;
                int vsY = centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET - displayHeight / 2-120;
                
                // 画像を描画（背景なし）
                graphics->drawTexture(vsImage, vsX, vsY, displayWidth, displayHeight);
            } else {
                // フォールバック：テキスト表示
                std::string vsText = "VS";
                SDL_Color vsTextColor = {255, 255, 255, 255};
                graphics->drawText(vsText, centerX - 30, centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET, "default", vsTextColor);
            }
        }
        
        // 敵コマンドの表示（VS表示中以外、またはSHOW_RESULTフェーズでは常に表示）
        // SHOW_ENEMY_COMMANDフェーズで1秒経過後、またはSHOW_RESULTフェーズでは敵のコマンドを表示
        bool shouldShowEnemyCmd = false;
        if (params.judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND && params.judgeDisplayTimer >= BattleConstants::JUDGE_VS_DISPLAY_TIME) {
            shouldShowEnemyCmd = true;
        } else if (params.judgeSubPhase == JudgeSubPhase::SHOW_RESULT) {
            shouldShowEnemyCmd = true;
        }
        
        if (shouldShowEnemyCmd) {
            // 敵のコマンドを表示
            std::string enemyCmd;
            if (!params.enemyCommandName.empty()) {
                // 住民戦用：コマンド名を直接使用
                enemyCmd = params.enemyCommandName;
            } else {
                // 通常戦闘：battleLogicから取得
                auto enemyCmds = battleLogic->getEnemyCommands();
                // 範囲チェック
                if (params.currentJudgingTurnIndex >= 0 && 
                    params.currentJudgingTurnIndex < static_cast<int>(enemyCmds.size())) {
                    enemyCmd = BattleLogic::getCommandName(enemyCmds[params.currentJudgingTurnIndex]);
                } else {
                    enemyCmd = "不明";
                }
            }
            
            if (!enemyCmd.empty()) {
                SDL_Color enemyCmdColor = {255, 255, 255, 255}; // 白いテキスト
                int cmdTextX = enemyDisplayX - BattleConstants::JUDGE_COMMAND_X_OFFSET;
                int cmdTextY = centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET;
                
                // 敵コマンドを画像で表示
                SDL_Texture* enemyCmdImage = getCommandTexture(enemyCmd);
                if (enemyCmdImage) {
                    int imageWidth, imageHeight;
                    SDL_QueryTexture(enemyCmdImage, nullptr, nullptr, &imageWidth, &imageHeight);
                    
                    // 画像を適切なサイズで表示（80px幅に固定、アスペクト比を保持）
                    float aspectRatio = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
                    int displayWidth, displayHeight;
                    if (imageWidth > imageHeight) {
                        // 横長の画像
                        displayWidth = 80;
                        displayHeight = static_cast<int>(80 / aspectRatio);
                    } else {
                        // 縦長または正方形の画像
                        displayHeight = 80;
                        displayWidth = static_cast<int>(80 * aspectRatio);
                    }
                    
                    // 画像を中央に配置
                    int imageX = cmdTextX - (displayWidth / 2) -80;
                    int imageY = cmdTextY - (displayHeight / 2) -120;
                    
                    // 画像を描画（背景なし）
                    graphics->drawTexture(enemyCmdImage, imageX, imageY, displayWidth, displayHeight);
                } else {
                    // フォールバック：テキスト表示
                SDL_Texture* enemyCmdTexture = graphics->createTextTexture(enemyCmd, "default", enemyCmdColor);
                if (enemyCmdTexture) {
                    int textWidth, textHeight;
                    SDL_QueryTexture(enemyCmdTexture, nullptr, nullptr, &textWidth, &textHeight);
                    
                    int scaledWidth = static_cast<int>(textWidth * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
                    int scaledHeight = static_cast<int>(textHeight * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
                    
                    int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_LARGE;
                    int bgX = cmdTextX - padding;
                    int bgY = cmdTextY - padding;
                    int bgWidth = scaledWidth + padding * 2;
                    int bgHeight = scaledHeight + padding * 2;
                    
                    graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                    graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
                    graphics->setDrawColor(255, 255, 255, 255);
                    graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
                    
                    graphics->drawTexture(enemyCmdTexture, cmdTextX, cmdTextY, scaledWidth, scaledHeight);
                    SDL_DestroyTexture(enemyCmdTexture);
                } else {
                    graphics->drawText(enemyCmd, cmdTextX, cmdTextY, "default", enemyCmdColor);
                    }
                }
            }
        }
    }
    
    if (params.judgeSubPhase == JudgeSubPhase::SHOW_RESULT) {
        int result;
        if (params.judgeResult != -999) {
            // 住民戦用：判定結果を直接使用
            result = params.judgeResult;
        } else {
            // 通常戦闘：battleLogicから取得
            auto playerCmds = battleLogic->getPlayerCommands();
            auto enemyCmds = battleLogic->getEnemyCommands();
            result = battleLogic->judgeRound(playerCmds[params.currentJudgingTurnIndex], 
                                   enemyCmds[params.currentJudgingTurnIndex]);
        }
        
        // JSONから設定を取得
        auto& judgePhaseConfig = battleConfig.judgePhase;
        
        std::string resultText;
        SDL_Color resultTextColor;
        SDL_Color resultBackgroundColor;
        
        float scaleProgress = std::min(1.0f, params.judgeDisplayTimer / BattleConstants::JUDGE_RESULT_SCALE_ANIMATION_DURATION);
        float scale = BattleConstants::JUDGE_RESULT_MIN_SCALE + scaleProgress * BattleConstants::JUDGE_RESULT_SCALE_RANGE;
        
        if (result == 1) {
            resultText = judgePhaseConfig.win.text;
            resultTextColor = judgePhaseConfig.win.textColor;
            resultBackgroundColor = judgePhaseConfig.win.backgroundColor;
        } else if (result == -1) {
            resultText = judgePhaseConfig.lose.text;
            resultTextColor = judgePhaseConfig.lose.textColor;
            resultBackgroundColor = judgePhaseConfig.lose.backgroundColor;
        } else {
            resultText = judgePhaseConfig.draw.text;
            resultTextColor = judgePhaseConfig.draw.textColor;
            resultBackgroundColor = judgePhaseConfig.draw.backgroundColor;
        }
        
        int textWidth = judgePhaseConfig.baseWidth;
        int textHeight = judgePhaseConfig.baseHeight;
        int scaledWidth = (int)(textWidth * scale);
        int scaledHeight = (int)(textHeight * scale);
        
        // 位置を計算（JSONから取得）
        int textX, textY;
        if (judgePhaseConfig.position.useRelative) {
            textX = static_cast<int>(centerX + judgePhaseConfig.position.offsetX - scaledWidth / 2);
            textY = static_cast<int>(centerY + judgePhaseConfig.position.offsetY - scaledHeight / 2);
        } else {
            textX = static_cast<int>(judgePhaseConfig.position.absoluteX);
            textY = static_cast<int>(judgePhaseConfig.position.absoluteY);
        }
        
        int backgroundPadding = judgePhaseConfig.backgroundPadding;
        // 背景色を描画（アルファ値を考慮）
        graphics->setDrawColor(resultBackgroundColor.r, resultBackgroundColor.g, resultBackgroundColor.b, resultBackgroundColor.a);
        graphics->drawRect(textX - backgroundPadding, textY - backgroundPadding, scaledWidth + backgroundPadding * 2, scaledHeight + backgroundPadding * 2, true);
        
        // テキストを描画（テキスト色を使用）
        graphics->drawText(resultText, textX, textY, "default", resultTextColor);
        
        if (result == 1) {
            float glowProgress = std::sin(params.judgeDisplayTimer * 3.14159f * 4.0f) * 0.5f + 0.5f;
            SDL_Color glowColor = judgePhaseConfig.glowColor;
            graphics->setDrawColor(glowColor.r, glowColor.g, glowColor.b, (Uint8)(glowProgress * 100));
            graphics->drawRect(textX - 30, textY - 30, scaledWidth + 60, scaledHeight + 60, false);
        }
    }
}

void BattleUI::renderCommandSelectionUI(const CommandSelectRenderParams& params) {
    // フォントが読み込まれていない場合は描画をスキップ
    if (!graphics->getFont("default")) {
        return;
    }
    
    int screenWidth = graphics->getScreenWidth();
    int screenHeight = graphics->getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // 画面をクリア（背景画像で覆う前に）
    graphics->setDrawColor(0, 0, 0, 255);
    graphics->clear();
    
    // 背景画像を描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
    SDL_Texture* bgTexture = getBattleBackgroundTexture();
    if (bgTexture) {
        graphics->drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
    }
    
    // オーバーレイを削除して背景画像が見えるようにする
    // graphics->setDrawColor(0, 0, 0, 100);
    // graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    
    // JSONからプレイヤーと敵の位置を取得
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    
    int playerBaseX, playerBaseY;
    config.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
    
    // 窮地モードではplayer_adversity.pngを使用
    SDL_Texture* playerTex = hasUsedLastChanceMode ? graphics->getTexture("player_adversity") : graphics->getTexture("player");
    if (!playerTex && hasUsedLastChanceMode) {
        playerTex = graphics->getTexture("player"); // フォールバック
    }
    
    if (playerTex) {
        graphics->drawTextureAspectRatio(playerTex, playerBaseX, playerBaseY, BattleConstants::BATTLE_CHARACTER_SIZE);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerBaseX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerBaseY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
    }
    
    int enemyBaseX, enemyBaseY;
    config.calculatePosition(enemyBaseX, enemyBaseY, battleConfig.enemyPosition, screenWidth, screenHeight);
    
    // 住民の場合は住民の画像を使用、それ以外は通常の敵画像を使用
    SDL_Texture* enemyTex = nullptr;
    if (enemy->isResident()) {
        int textureIndex = enemy->getResidentTextureIndex();
        std::string textureName = "resident_" + std::to_string(textureIndex + 1);
        enemyTex = graphics->getTexture(textureName);
    } else {
        enemyTex = graphics->getTexture("enemy_" + enemy->getTypeName());
    }
    
    if (enemyTex) {
        graphics->drawTextureAspectRatio(enemyTex, enemyBaseX, enemyBaseY, BattleConstants::BATTLE_CHARACTER_SIZE);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyBaseX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, enemyBaseY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
    }
    
    // 住民戦の場合は10ターン制限を表示
    if (params.residentTurnCount > 0) {
        renderTurnNumber(params.residentTurnCount, 10, params.isDesperateMode);
    } else {
    renderTurnNumber(params.currentSelectingTurn + 1, params.commandTurnCount, params.isDesperateMode);
    }
    
    // 選択済みコマンドを表示（ボタンの上）
    if (!battleLogic) {
        return; // battleLogicがnullの場合は描画をスキップ
    }
    
    auto playerCmds = battleLogic->getPlayerCommands();
    auto& cmdSelectConfig = battleConfig.commandSelection;
    
    // currentOptionsがnullptrの場合は描画をスキップ
    if (!params.currentOptions) {
        return;
    }
    
    if (params.currentSelectingTurn > 0) {
        // 選択済みコマンドを画像で表示
        int selectedCmdY = static_cast<int>(centerY + cmdSelectConfig.selectedCommandOffsetY);
        int imageSpacing = cmdSelectConfig.imageSpacing;
        int startX = centerX - ((params.currentSelectingTurn - 1) * imageSpacing / 2);
        
        int totalWidth = 0;
        std::vector<SDL_Texture*> commandImages;
        std::vector<int> imageWidths;
        
        // まず全ての画像を取得してサイズを計算
        for (int i = 0; i < params.currentSelectingTurn && i < static_cast<int>(playerCmds.size()); i++) {
            std::string cmdName = BattleLogic::getCommandName(playerCmds[i]);
            SDL_Texture* cmdImage = getCommandTexture(cmdName);
            if (cmdImage) {
                int imgWidth, imgHeight;
                if (SDL_QueryTexture(cmdImage, nullptr, nullptr, &imgWidth, &imgHeight) != 0) {
                    // テクスチャのクエリに失敗した場合はスキップ
                    commandImages.push_back(nullptr);
                    imageWidths.push_back(0);
                    continue;
                }
                int displayWidth = cmdSelectConfig.selectedCommandImageSize;
                // ゼロ除算を防ぐ
                if (imgWidth <= 0 || imgHeight <= 0) {
                    commandImages.push_back(nullptr);
                    imageWidths.push_back(0);
                    continue;
                }
                int displayHeight = static_cast<int>(imgHeight * (static_cast<float>(displayWidth) / imgWidth));
                if (displayWidth <= 0 || displayHeight <= 0) {
                    commandImages.push_back(nullptr);
                    imageWidths.push_back(0);
                    continue;
                }
                commandImages.push_back(cmdImage);
                imageWidths.push_back(displayWidth);
                totalWidth += displayWidth;
                if (i < params.currentSelectingTurn - 1) {
                    totalWidth += cmdSelectConfig.arrowSpacing;
                }
            } else {
                commandImages.push_back(nullptr);
                imageWidths.push_back(0);
            }
        }
        
        // 画像を描画（背景なし）
        int currentX = centerX - (totalWidth / 2);
        for (int i = 0; i < params.currentSelectingTurn && i < static_cast<int>(playerCmds.size()) && i < static_cast<int>(commandImages.size()); i++) {
            if (commandImages[i]) {
                std::string cmdName = BattleLogic::getCommandName(playerCmds[i]);
                SDL_Texture* cmdImage = commandImages[i];
                int imgWidth, imgHeight;
                if (SDL_QueryTexture(cmdImage, nullptr, nullptr, &imgWidth, &imgHeight) != 0) {
                    // テクスチャのクエリに失敗した場合はスキップ
                    continue;
                }
                int displayWidth = cmdSelectConfig.selectedCommandImageSize;
                // ゼロ除算を防ぐ
                if (imgWidth <= 0 || imgHeight <= 0) {
                    continue;
                }
                int displayHeight = static_cast<int>(imgHeight * (static_cast<float>(displayWidth) / imgWidth));
                if (displayWidth <= 0 || displayHeight <= 0) {
                    continue;
                }
                int imageX = currentX;
                int imageY = selectedCmdY - (displayHeight / 2);
                
                // テクスチャが有効な場合のみ描画
                if (cmdImage && displayWidth > 0 && displayHeight > 0) {
                    graphics->drawTexture(cmdImage, imageX, imageY, displayWidth, displayHeight);
                }
                currentX += displayWidth;
                
            } else {
                // フォールバック：テキスト表示
                std::string cmdName = BattleLogic::getCommandName(playerCmds[i]);
                SDL_Color selectedCmdColor = {255, 255, 255, 255};
                graphics->drawText(cmdName, currentX, selectedCmdY - 15, "default", selectedCmdColor);
                currentX += 60;
            }
        }
    }
    
    auto& cmdSelectState = animationController->getCommandSelectState();
    float buttonSlideProgress = std::min(1.0f, cmdSelectState.commandSelectSlideProgress);
    int baseY = static_cast<int>(centerY + cmdSelectConfig.buttonBaseOffsetY);
    int slideOffset = (int)((1.0f - buttonSlideProgress) * 200);
    
    int buttonWidth = cmdSelectConfig.buttonWidth;
    int buttonHeight = cmdSelectConfig.buttonHeight;
    int buttonSpacing = cmdSelectConfig.buttonSpacing;
    int startX = centerX - (buttonWidth / 2);
    int startY = baseY + slideOffset;
    
    for (size_t i = 0; i < params.currentOptions->size(); i++) {
        int buttonY = startY + (int)(i * buttonSpacing);
        
        bool isSelected = (static_cast<int>(i) == params.selectedOption);
        
        SDL_Color bgColor;
        if (isSelected) {
            float glowProgress = std::sin(cmdSelectState.commandSelectAnimationTimer * 3.14159f * 4.0f) * 0.3f + 0.7f;
            bgColor = {
                (Uint8)(cmdSelectConfig.selectedBgColor.r * glowProgress),
                (Uint8)(cmdSelectConfig.selectedBgColor.g * glowProgress),
                (Uint8)(cmdSelectConfig.selectedBgColor.b * glowProgress),
                cmdSelectConfig.selectedBgColor.a
            };
        } else {
            bgColor = cmdSelectConfig.unselectedBgColor;
        }
        
        graphics->setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        graphics->drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, true);
        
        if (isSelected) {
            graphics->setDrawColor(cmdSelectConfig.selectedBorderColor.r, cmdSelectConfig.selectedBorderColor.g, cmdSelectConfig.selectedBorderColor.b, cmdSelectConfig.selectedBorderColor.a);
            graphics->drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, false);
            graphics->drawRect(startX - 8, buttonY - 3, buttonWidth + 16, buttonHeight + 6, false);
        } else {
            graphics->setDrawColor(cmdSelectConfig.unselectedBorderColor.r, cmdSelectConfig.unselectedBorderColor.g, cmdSelectConfig.unselectedBorderColor.b, cmdSelectConfig.unselectedBorderColor.a);
            graphics->drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, false);
        }
        
        SDL_Color textColor = isSelected ? cmdSelectConfig.selectedTextColor : cmdSelectConfig.unselectedTextColor;
        
        // コマンド名を画像で表示
        if (i >= params.currentOptions->size()) {
            continue; // 範囲外チェック
        }
        std::string commandName = (*params.currentOptions)[i];
        SDL_Texture* commandImage = getCommandTexture(commandName);
        
        if (commandImage) {
            // 画像を適切なサイズで表示
            int imageWidth, imageHeight;
            if (SDL_QueryTexture(commandImage, nullptr, nullptr, &imageWidth, &imageHeight) != 0) {
                // テクスチャのクエリに失敗した場合はテキスト表示にフォールバック
                int textX = startX + (buttonWidth / 2) - 50;
                int textY = buttonY + (buttonHeight / 2) - 15;
                if (isSelected) {
                    graphics->drawText("▶", textX - 30, textY, "default", {255, 215, 0, 255});
                    graphics->drawText(commandName, textX, textY, "default", textColor);
                } else {
                    graphics->drawText(commandName, textX, textY, "default", textColor);
                }
                continue;
            }
            
            // ゼロ除算を防ぐ
            if (imageWidth <= 0 || imageHeight <= 0) {
                // テキスト表示にフォールバック
                int textX = startX + (buttonWidth / 2) - 50;
                int textY = buttonY + (buttonHeight / 2) - 15;
                if (isSelected) {
                    graphics->drawText("▶", textX - 30, textY, "default", {255, 215, 0, 255});
                    graphics->drawText(commandName, textX, textY, "default", textColor);
                } else {
                    graphics->drawText(commandName, textX, textY, "default", textColor);
                }
                continue;
            }
            
            int displayWidth = cmdSelectConfig.buttonImageSize;
            int displayHeight = static_cast<int>(imageHeight * (static_cast<float>(displayWidth) / imageWidth));
            
            // 表示サイズが無効な場合はスキップ
            if (displayWidth <= 0 || displayHeight <= 0) {
                // テキスト表示にフォールバック
                int textX = startX + (buttonWidth / 2) - 50;
                int textY = buttonY + (buttonHeight / 2) - 15;
                if (isSelected) {
                    graphics->drawText("▶", textX - 30, textY, "default", {255, 215, 0, 255});
                    graphics->drawText(commandName, textX, textY, "default", textColor);
                } else {
                    graphics->drawText(commandName, textX, textY, "default", textColor);
                }
                continue;
            }
            
            int imageX = startX + (buttonWidth / 2) - (displayWidth / 2);
            int imageY = buttonY + (buttonHeight / 2) - (displayHeight / 2);
            
        if (isSelected) {
                graphics->drawText("▶", startX + 10, buttonY + (buttonHeight / 2) - 15, "default", cmdSelectConfig.selectedBorderColor);
        }
            // テクスチャが有効な場合のみ描画
            if (commandImage && displayWidth > 0 && displayHeight > 0) {
                graphics->drawTexture(commandImage, imageX, imageY, displayWidth, displayHeight);
            }
        } else {
            // フォールバック：テキスト表示
        int textX = startX + (buttonWidth / 2) - 50;
        int textY = buttonY + (buttonHeight / 2) - 15;
        
        if (isSelected) {
            graphics->drawText("▶", textX - 30, textY, "default", {255, 215, 0, 255});
                graphics->drawText(commandName, textX, textY, "default", textColor);
        } else {
                graphics->drawText(commandName, textX, textY, "default", textColor);
        }
    }
    }
    
    // コマンド選択ヒントテキスト（JSONから設定を取得）
    auto& commandHintConfig = battleConfig.commandHint;
    std::string hintText;
    if (params.residentTurnCount > 0) {
        hintText = commandHintConfig.residentText;
    } else {
        hintText = commandHintConfig.normalText;
    }
    SDL_Color hintColor = commandHintConfig.color;
    
    // 位置を計算（JSONから取得）
    int hintX, hintY;
    if (commandHintConfig.position.useRelative) {
        hintX = static_cast<int>(centerX + commandHintConfig.position.offsetX);
        hintY = static_cast<int>(screenHeight + commandHintConfig.position.offsetY);
    } else {
        hintX = static_cast<int>(commandHintConfig.position.absoluteX);
        hintY = static_cast<int>(commandHintConfig.position.absoluteY);
    }
    
    // 選択方法のテキスト背景を描画
    SDL_Texture* hintTexture = graphics->createTextTexture(hintText, "default", hintColor);
    if (hintTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(hintTexture, nullptr, nullptr, &textWidth, &textHeight);
        
        int padding = commandHintConfig.padding;
        int bgX = hintX - padding;
        int bgY = hintY - padding;
        int bgWidth = textWidth + padding * 2;
        int bgHeight = textHeight + padding * 2;
        
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
        
        SDL_DestroyTexture(hintTexture);
    }
    
    graphics->drawText(hintText, hintX, hintY, "default", hintColor);
    
    int hitCount = params.residentHitCount;
    renderHP(playerBaseX, playerBaseY, enemyBaseX, enemyBaseY, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, params.residentBehaviorHint, false, hitCount);
}

void BattleUI::renderResultAnnouncement(const ResultAnnouncementRenderParams& params) {
    int screenWidth = graphics->getScreenWidth();
    int screenHeight = graphics->getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    // JSONから設定を取得（ホットリロード対応のため、毎回取得）
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    
    // 画面をクリア（背景画像で覆う前に）
    graphics->setDrawColor(0, 0, 0, 255);
    graphics->clear();
    
    // 背景画像を描画（画面サイズに完全に合わせて描画、アスペクト比は無視）
    SDL_Texture* bgTexture = getBattleBackgroundTexture();
    if (bgTexture) {
        graphics->drawTexture(bgTexture, 0, 0, screenWidth, screenHeight);
    }
    
    // オーバーレイを削除して背景画像が見えるようにする
    // graphics->setDrawColor(0, 0, 0, 100);
    // graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    
    // メイン結果テキスト（JSONから設定を取得）
    std::string mainText;
    SDL_Color mainTextColor;
    SDL_Color mainBackgroundColor;
    
    auto& resultTextConfig = battleConfig.judgeResult.resultText;
    
    if (params.isVictory) {
        if (params.isDesperateMode) {
            mainText = "一発逆転成功！";
            mainTextColor = resultTextConfig.desperateVictory.textColor;
            mainBackgroundColor = resultTextConfig.desperateVictory.backgroundColor;
        } else {
            mainText = "勝利！";
            mainTextColor = resultTextConfig.victory.textColor;
            mainBackgroundColor = resultTextConfig.victory.backgroundColor;
        }
    } else if (params.isDefeat) {
        if (params.isDesperateMode) {
            mainText = "大敗北...";
            mainTextColor = resultTextConfig.desperateDefeat.textColor;
            mainBackgroundColor = resultTextConfig.desperateDefeat.backgroundColor;
        } else {
            mainText = "敗北...";
            mainTextColor = resultTextConfig.defeat.textColor;
            mainBackgroundColor = resultTextConfig.defeat.backgroundColor;
        }
    } else {
        mainText = "引き分け";
        mainTextColor = resultTextConfig.draw.textColor;
        mainBackgroundColor = resultTextConfig.draw.backgroundColor;
    }
    
    auto& resultState = animationController->getResultState();
    float displayScale = (resultState.resultScale > 0.0f) ? resultState.resultScale : 1.0f;
    displayScale = std::max(0.5f, displayScale); // 最小スケールを0.5fに設定
    int textWidth = resultTextConfig.baseWidth;
    int textHeight = resultTextConfig.baseHeight;
    int scaledWidth = (int)(textWidth * displayScale);
    int scaledHeight = (int)(textHeight * displayScale);
    
    // 位置を計算（JSONから取得）
    int textX, textY;
    if (resultTextConfig.position.useRelative) {
        textX = centerX - scaledWidth / 2;
        textY = static_cast<int>(centerY + resultTextConfig.position.offsetY - scaledHeight / 2);
    } else {
        textX = static_cast<int>(resultTextConfig.position.absoluteX);
        textY = static_cast<int>(resultTextConfig.position.absoluteY);
    }
    
    float glowIntensity = std::sin(resultState.resultAnimationTimer * 3.14159f * 4.0f) * 0.3f + 0.7f;
    graphics->setDrawColor((Uint8)(mainBackgroundColor.r * glowIntensity), 
                          (Uint8)(mainBackgroundColor.g * glowIntensity), 
                          (Uint8)(mainBackgroundColor.b * glowIntensity), 
                          mainBackgroundColor.a);
    graphics->drawRect(textX - 30, textY - 30, scaledWidth + 60, scaledHeight + 60, true);
    
    graphics->drawText(mainText, textX, textY, "default", mainTextColor);
    
    if (params.isVictory) {
        float outerGlow = std::sin(resultState.resultAnimationTimer * 3.14159f * 6.0f) * 0.5f + 0.5f;
        // 勝利時の背景色を使用してグロー効果を描画
        SDL_Color glowColor = params.isDesperateMode ? resultTextConfig.desperateVictory.backgroundColor : resultTextConfig.victory.backgroundColor;
        graphics->setDrawColor(glowColor.r, glowColor.g, glowColor.b, (Uint8)(outerGlow * 150));
        graphics->drawRect(textX - 50, textY - 50, scaledWidth + 100, scaledHeight + 100, false);
        graphics->drawRect(textX - 45, textY - 45, scaledWidth + 90, scaledHeight + 90, false);
    }
    
    if (params.hasThreeWinStreak && params.isVictory) {
        auto& threeWinStreakConfig = battleConfig.judgeResult.threeWinStreak;
        // テキストは動かないように固定スケール（1.0f）を使用
        float streakScale = 1.0f;
        // 3連勝ボーナスのテキストを直接生成（プレースホルダーを使わずに固定文字列）
        std::string streakText = "3連勝！ダメージ2.5倍ボーナス！";
        SDL_Color streakColor = threeWinStreakConfig.color;
        
        int streakTextWidth = threeWinStreakConfig.baseWidth;
        int streakTextHeight = threeWinStreakConfig.baseHeight;
        // テキストのサイズは固定（スケール1.0f）
        int streakScaledWidth = streakTextWidth;
        int streakScaledHeight = streakTextHeight;
        
        // 背景とグロー効果は固定サイズ（アニメーションなし）
        float bgScale = 1.0f;
        int bgScaledWidth = (int)(streakTextWidth * bgScale);
        int bgScaledHeight = (int)(streakTextHeight * bgScale);
        int bgX = centerX - bgScaledWidth / 2;
        int bgY = static_cast<int>(centerY + threeWinStreakConfig.position.offsetY - bgScaledHeight / 2);
        
        // 背景を黒に変更
        graphics->setDrawColor(0, 0, 0, 200);
        graphics->drawRect(bgX - 20, bgY - 20, bgScaledWidth + 40, bgScaledHeight + 40, true);
        // 背景の周りに白い枠線を描画（他のUIと同じスタイル）
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX - 20, bgY - 20, bgScaledWidth + 40, bgScaledHeight + 40, false);
        
        // グロー効果も固定（アニメーションなし）
        graphics->setDrawColor(255, 255, 255, 200);
        graphics->drawRect(bgX - 30, bgY - 30, bgScaledWidth + 60, bgScaledHeight + 60, false);
        graphics->drawRect(bgX - 25, bgY - 25, bgScaledWidth + 50, bgScaledHeight + 50, false);
        
        // テキストを背景の中央に配置
        int streakTextX = bgX + bgScaledWidth / 2 - streakScaledWidth / 2;
        int streakTextY = bgY + bgScaledHeight / 2 - streakScaledHeight / 2;
        graphics->drawText(streakText, streakTextX, streakTextY, "default", streakColor);
    }
    
    
    // if (params.hasThreeWinStreak && params.isVictory) {
    //     auto& damageBonusConfig = battleConfig.judgeResult.damageBonus;
    //     // JSONからフォーマットを取得
    //     std::string bonusText = damageBonusConfig.format;
    //     // プレースホルダーを置換
    //     size_t pos = bonusText.find("{multiplier}");
    //     if (pos != std::string::npos) {
    //         std::string multiplierStr = "2.5";  // THREE_WIN_STREAK_MULTIPLIER = 2.5f
    //         bonusText.replace(pos, 13, multiplierStr);
    //     }
    //     SDL_Color bonusColor = damageBonusConfig.color;
        
    //     // 位置を計算（JSONから取得）
    //     int bonusX, bonusY;
    //     if (damageBonusConfig.position.useRelative) {
    //         bonusX = static_cast<int>(centerX + damageBonusConfig.position.offsetX);
    //         bonusY = static_cast<int>(centerY + damageBonusConfig.position.offsetY);
    //     } else {
    //         bonusX = static_cast<int>(damageBonusConfig.position.absoluteX);
    //         bonusY = static_cast<int>(damageBonusConfig.position.absoluteY);
    //     }
        
    //     graphics->drawText(bonusText, bonusX, bonusY, "default", bonusColor);
    // }
    
    
    if (params.isVictory) {
        float starGlow = std::sin(resultState.resultAnimationTimer * 3.14159f * 8.0f) * 0.5f + 0.5f;
        SDL_Color starColor = {255, 255, 0, (Uint8)(starGlow * 255)};
        
        int starRadius = 150;
        float starDisplayScale = std::max(0.1f, resultState.resultScale);
        for (int i = 0; i < 8; i++) {
            float angle = (i * 3.14159f * 2.0f) / 8.0f;
            int starX = centerX + (int)(std::cos(angle) * starRadius * starDisplayScale);
            int starY = centerY - 100 + (int)(std::sin(angle) * starRadius * starDisplayScale);
            graphics->drawText("★", starX, starY, "default", starColor);
        }
    }
    
    // 敗北時の暗いオーバーレイを削除（勝った時と同じように背景が見えるように）
    
    // プレイヤーと敵の位置を取得（battleConfigは既に取得済み）
    int playerBaseX, playerBaseY;
    config.calculatePosition(playerBaseX, playerBaseY, battleConfig.playerPosition, screenWidth, screenHeight);
    
    int enemyBaseX, enemyBaseY;
    config.calculatePosition(enemyBaseX, enemyBaseY, battleConfig.enemyPosition, screenWidth, screenHeight);
    
    auto& charState = animationController->getCharacterState();
    int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
    int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
    
    int enemyX = enemyBaseX + (int)charState.enemyAttackOffsetX + (int)charState.enemyHitOffsetX;
    int enemyY = enemyBaseY + (int)charState.enemyAttackOffsetY + (int)charState.enemyHitOffsetY;
    
    int playerWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    int enemyWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int enemyHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    
    int hitCount = params.residentHitCount;
    renderHP(playerX, playerY, enemyX, enemyY, playerHeight, enemyHeight, params.residentBehaviorHint, false, hitCount);
    
    renderCharacters(playerX, playerY, enemyX, enemyY, playerWidth, playerHeight, enemyWidth, enemyHeight);
    
    // ジャッジ結果フェーズでは三すくみ画像を表示しない（renderRockPaperScissorsImageで表示されるため）
}

void BattleUI::renderCharacters(int playerX, int playerY, int enemyX, int enemyY,
                                 int playerWidth, int playerHeight, int enemyWidth, int enemyHeight) {
    // 窮地モードではplayer_adversity.pngを使用
    SDL_Texture* playerTex = hasUsedLastChanceMode ? graphics->getTexture("player_adversity") : graphics->getTexture("player");
    if (!playerTex && hasUsedLastChanceMode) {
        playerTex = graphics->getTexture("player"); // フォールバック
    }
    
    if (playerTex) {
        graphics->drawTextureAspectRatio(playerTex, playerX, playerY, BattleConstants::BATTLE_CHARACTER_SIZE);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(playerX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, playerY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, false);
    }
    
    // 住民の場合は住民の画像を使用、それ以外は通常の敵画像を使用
    SDL_Texture* enemyTex = nullptr;
    if (enemy->isResident()) {
        int textureIndex = enemy->getResidentTextureIndex();
        std::string textureName = "resident_" + std::to_string(textureIndex + 1);
        enemyTex = graphics->getTexture(textureName);
    } else {
        enemyTex = graphics->getTexture("enemy_" + enemy->getTypeName());
    }
    
    if (enemyTex) {
        graphics->drawTextureAspectRatio(enemyTex, enemyX, enemyY, BattleConstants::BATTLE_CHARACTER_SIZE);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, enemyY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(enemyX - BattleConstants::BATTLE_CHARACTER_SIZE / 2, enemyY - BattleConstants::BATTLE_CHARACTER_SIZE / 2, BattleConstants::BATTLE_CHARACTER_SIZE, BattleConstants::BATTLE_CHARACTER_SIZE, false);
    }
}

void BattleUI::renderHP(int playerX, int playerY, int enemyX, int enemyY,
                  int playerHeight, int enemyHeight, const std::string& residentBehaviorHint, bool hideEnemyUI, int residentHitCount) {
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto battleConfig = config.getBattleConfig();
    
    int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
    
    // プレイヤーの名前とレベル（HPの上に表示）- 住民戦でも表示
    std::string playerNameText = player->getName() + " Lv." + std::to_string(player->getLevel());
    SDL_Texture* playerNameTexture = graphics->createTextTexture(playerNameText, "default", battleConfig.playerName.color);
    if (playerNameTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(playerNameTexture, nullptr, nullptr, &textWidth, &textHeight);
        int bgX = static_cast<int>(playerX + battleConfig.playerName.offsetX - padding);
        int bgY = static_cast<int>(playerY - playerHeight / 2 + battleConfig.playerName.offsetY - padding);
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
        SDL_DestroyTexture(playerNameTexture);
    }
    int playerNameX = static_cast<int>(playerX + battleConfig.playerName.offsetX);
    int playerNameY = static_cast<int>(playerY - playerHeight / 2 + battleConfig.playerName.offsetY);
    graphics->drawText(playerNameText, playerNameX, playerNameY, "default", battleConfig.playerName.color);
    
    // 住民戦の場合はプレイヤーのHP表示をスキップ
    if (!enemy->isResident()) {
        // プレイヤーのHPを体力バーとして表示
        int barWidth = battleConfig.healthBar.width;
        int barHeight = battleConfig.healthBar.height;
        int barX = static_cast<int>(playerX + battleConfig.healthBar.offsetX);
        int barY = static_cast<int>(playerY - playerHeight / 2 + battleConfig.healthBar.offsetY);
        
        // HPの割合を計算
        int playerHp = player->getHp();
        int playerMaxHp = player->getMaxHp();
        float hpRatio = static_cast<float>(playerHp) / static_cast<float>(playerMaxHp);
        if (hpRatio < 0.0f) hpRatio = 0.0f;
        if (hpRatio > 1.0f) hpRatio = 1.0f;
        
        int currentBarWidth = static_cast<int>(barWidth * hpRatio);
        
        // バーの背景（空の部分）を描画
        graphics->setDrawColor(battleConfig.healthBar.bgColor.r, battleConfig.healthBar.bgColor.g, battleConfig.healthBar.bgColor.b, battleConfig.healthBar.bgColor.a);
        graphics->drawRect(barX, barY, barWidth, barHeight, true);
        
        // 現在のHPに応じた黄緑色のバーを描画
        if (currentBarWidth > 0) {
            graphics->setDrawColor(battleConfig.healthBar.barColor.r, battleConfig.healthBar.barColor.g, battleConfig.healthBar.barColor.b, battleConfig.healthBar.barColor.a);
            graphics->drawRect(barX, barY, currentBarWidth, barHeight, true);
        }
        
        // バーの枠線を描画
        graphics->setDrawColor(battleConfig.healthBar.borderColor.r, battleConfig.healthBar.borderColor.g, battleConfig.healthBar.borderColor.b, battleConfig.healthBar.borderColor.a);
        graphics->drawRect(barX, barY, barWidth, barHeight, false);
    
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
        SDL_Texture* statusTexture = graphics->createTextTexture(statusText, "default", statusColor);
        if (statusTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(statusTexture, nullptr, nullptr, &textWidth, &textHeight);
                int bgX = static_cast<int>(playerX + attackMultiplierConfig.offsetX - attackMultiplierConfig.padding);
                int bgY = static_cast<int>(playerY - playerHeight / 2 + attackMultiplierConfig.offsetY - attackMultiplierConfig.padding);
                graphics->setDrawColor(attackMultiplierConfig.bgColor.r, attackMultiplierConfig.bgColor.g, attackMultiplierConfig.bgColor.b, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                graphics->drawRect(bgX, bgY, textWidth + attackMultiplierConfig.padding * 2, textHeight + attackMultiplierConfig.padding * 2, true);
                graphics->setDrawColor(attackMultiplierConfig.borderColor.r, attackMultiplierConfig.borderColor.g, attackMultiplierConfig.borderColor.b, attackMultiplierConfig.borderColor.a);
                graphics->drawRect(bgX, bgY, textWidth + attackMultiplierConfig.padding * 2, textHeight + attackMultiplierConfig.padding * 2, false);
            SDL_DestroyTexture(statusTexture);
        }
            int statusTextX = static_cast<int>(playerX + attackMultiplierConfig.offsetX);
            int statusTextY = static_cast<int>(playerY - playerHeight / 2 + attackMultiplierConfig.offsetY);
            graphics->drawText(statusText, statusTextX, statusTextY, "default", statusColor);
        }
    }
    
    // 敵のUIを非表示にするフラグが立っている場合は、敵のUIを描画しない
    if (hideEnemyUI) {
        return;
    }
    
    // 敵の型のヒントを表示（敵の名前の下）
    // 住民戦の場合は住民の様子を表示、それ以外は通常のヒントを表示
    bool hasHint = false;
    std::string hintText = "";
    
    if (enemy->isResident() && !residentBehaviorHint.empty()) {
        // 住民戦の場合は住民の様子を表示
        hintText = residentBehaviorHint;
        hasHint = true;
    } else if (battleLogic) {
        // 通常戦の場合は従来のヒント表示
        if (!battleLogic->isBehaviorTypeDetermined()) {
            // 戦闘開始時は、保存された除外する型を使って「〜ではなさそうだ」と表示
            BattleLogic::EnemyBehaviorType excludedType = battleLogic->getExcludedBehaviorType();
            hintText = BattleLogic::getNegativeBehaviorTypeHint(excludedType);
            hasHint = !hintText.empty();
        } else {
            // 型が確定した後はヒントを表示
            hintText = battleLogic->getBehaviorTypeHint(battleLogic->getEnemyBehaviorType());
            hasHint = !hintText.empty();
        }
    }
    
    // 敵の名前とレベル（HPの位置に表示、体力に応じて色を変更）
    // 住民の場合は住民の名前を使用、それ以外は通常の敵名を使用
    std::string enemyName = enemy->isResident() ? enemy->getName() : enemy->getTypeName();
    std::string enemyNameText = enemyName + " Lv." + std::to_string(enemy->getLevel());
    
    // 体力に応じて色を決定
    SDL_Color whiteColor = {255, 255, 255, 255};
    SDL_Color enemyNameColor = whiteColor;
    int enemyHp = enemy->getHp();
    int enemyMaxHp = enemy->getMaxHp();
    float hpRatio = static_cast<float>(enemyHp) / static_cast<float>(enemyMaxHp);
    
    if (hpRatio <= 0.3f) {
        // 3割以下: 赤色
        enemyNameColor = {255, 100, 100, 255};
    } else if (hpRatio <= 0.6f) {
        // 6割以下: 黄色
        enemyNameColor = {255, 255, 100, 255};
    }
    
    // 住民戦の場合は、住民名を少し上に移動し、その下にlife.pngを3つ横並びで表示
    if (enemy->isResident()) {
        // 住民名を少し上に移動（通常より30ピクセル上）
        int residentNameOffsetY = -40; // 通常の-50から-80に変更
        SDL_Texture* enemyNameTexture = graphics->createTextTexture(enemyNameText, "default", enemyNameColor);
        if (enemyNameTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(enemyNameTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = enemyX - 50 - padding;
            int bgY = enemyY - enemyHeight / 2 - padding + residentNameOffsetY;
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(enemyNameTexture);
        }
        graphics->drawText(enemyNameText, enemyX - 50, enemyY - enemyHeight / 2 + residentNameOffsetY, "default", enemyNameColor);
        
        // life.pngを3つ横並びで表示（住民名の下）
        SDL_Texture* lifeTexture = graphics->getTexture("life");
        if (lifeTexture) {
            int lifeSize = 50; // life.pngの表示サイズ
            int lifeSpacing = 5; // life.pngの間隔
            int totalWidth = lifeSize * 3 + lifeSpacing * 2;
            int startX = enemyX - totalWidth / 2;
            int lifeY = enemyY - enemyHeight / 2 + residentNameOffsetY + 35; // 住民名の下30ピクセル
            
            for (int i = 0; i < 3; i++) {
                // residentHitCountより小さいインデックスのlife.pngは非表示（攻撃が当たった分だけ減る）
                if (i < residentHitCount) {
                    continue; // このlife.pngは表示しない
                }
                int lifeX = startX + i * (lifeSize + lifeSpacing);
                graphics->drawTexture(lifeTexture, lifeX, lifeY, lifeSize, lifeSize);
            }
        }
    } else {
        // 通常の戦闘の場合は従来通り
        SDL_Texture* enemyNameTexture = graphics->createTextTexture(enemyNameText, "default", enemyNameColor);
        if (enemyNameTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(enemyNameTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = enemyX - 50 - padding;
            // ヒントが表示されている場合は、名前の位置を下にずらす
            int nameOffsetY = hasHint ? 40 : 0;
            int bgY = enemyY - enemyHeight / 2 - padding + nameOffsetY-50;
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(enemyNameTexture);
        }
        // ヒントが表示されている場合は、名前の位置を下にずらす
        int nameOffsetY = hasHint ? 40 : 0;
        graphics->drawText(enemyNameText, enemyX - 50, enemyY - enemyHeight / 2 + nameOffsetY-50, "default", enemyNameColor);
    }
    
    if (hasHint) {
            SDL_Texture* hintTexture = graphics->createTextTexture(hintText, "default", whiteColor);
            if (hintTexture) {
                int hintWidth, hintHeight;
                SDL_QueryTexture(hintTexture, nullptr, nullptr, &hintWidth, &hintHeight);
                int hintBgX = enemyX - 80 - padding;
                int hintOffsetY = hasHint ? 40 : 0;
                int hintBgY = enemyY - enemyHeight / 2 + 250 - padding + hintOffsetY;
                graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                graphics->drawRect(hintBgX, hintBgY, hintWidth + padding * 2, hintHeight + padding * 2, true);
                graphics->setDrawColor(255, 255, 255, 255);
                graphics->drawRect(hintBgX, hintBgY, hintWidth + padding * 2, hintHeight + padding * 2, false);
                graphics->drawText(hintText, enemyX - 80, enemyY - enemyHeight / 2 + 250 + hintOffsetY, "default", whiteColor);
                SDL_DestroyTexture(hintTexture);
            }
        }
}

void BattleUI::renderTurnNumber(int turnNumber, int totalTurns, bool isDesperateMode) {
    std::string turnText = "ターン " + std::to_string(turnNumber) + " / " + std::to_string(totalTurns);
    if (isDesperateMode) {
        turnText += "  ⚡ 大勝負 ⚡";
    }
    
    // テキストのサイズを取得して背景を描画
    SDL_Texture* textTexture = graphics->createTextTexture(turnText, "default", {255, 255, 255, 255});
    if (textTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
        
        // 背景を描画（パディング付き）
        // ターン数UIを少し下に移動（夜のタイマーUIの下に表示）
        int turnNumberY = 70; // 20から60に変更
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        int bgX = 18 - padding;
        int bgY = turnNumberY - padding;
        int bgWidth = textWidth + padding * 2;
        int bgHeight = textHeight + padding * 2;
        
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
        
        // テキストを描画
        SDL_Color turnColor = {255, 255, 255, 255};
        graphics->drawText(turnText, 18, turnNumberY, "default", turnColor);
        
        SDL_DestroyTexture(textTexture);
    } else {
        // フォールバック：通常のテキスト描画
        int turnNumberY = 60; // 20から60に変更
        SDL_Color turnColor = {255, 255, 255, 255};
        graphics->drawText(turnText, 20, turnNumberY, "default", turnColor);
    }
}

SDL_Texture* BattleUI::getCommandTexture(const std::string& commandName) const {
    if (commandName == "攻撃") {
        return graphics->getTexture("command_attack");
    } else if (commandName == "防御") {
        return graphics->getTexture("command_defend");
    } else if (commandName == "呪文") {
        return graphics->getTexture("command_magic");
    } else if (commandName == "身を隠す") {
        return graphics->getTexture("command_hide");
    } else if (commandName == "怯える") {
        return graphics->getTexture("command_fear");
    } else if (commandName == "助けを呼ぶ") {
        return graphics->getTexture("command_help");
    }
    return nullptr;
}

void BattleUI::drawCommandImage(const std::string& commandName, int x, int y, int width, int height) const {
    SDL_Texture* texture = getCommandTexture(commandName);
    if (texture) {
        if (width == 0 || height == 0) {
            int imgWidth, imgHeight;
            SDL_QueryTexture(texture, nullptr, nullptr, &imgWidth, &imgHeight);
            graphics->drawTexture(texture, x, y, imgWidth, imgHeight);
        } else {
            graphics->drawTexture(texture, x, y, width, height);
        }
    }
}

SDL_Texture* BattleUI::getBattleBackgroundTexture() const {
    // 住民の場合は夜の背景、衛兵・王様の場合は城の背景、魔王の場合は魔王の背景、それ以外は通常の戦闘背景を使用
    SDL_Texture* bgTexture = nullptr;
    if (enemy->isResident()) {
        bgTexture = graphics->getTexture("night_bg");
        if (!bgTexture) {
            bgTexture = graphics->loadTexture("assets/textures/bg/night_bg.png", "night_bg");
        }
    } else if (enemy->getType() == EnemyType::GUARD || enemy->getType() == EnemyType::KING) {
        bgTexture = graphics->getTexture("castle_bg");
        if (!bgTexture) {
            bgTexture = graphics->loadTexture("assets/textures/bg/castle_bg.png", "castle_bg");
        }
    } else if (enemy->getType() == EnemyType::DEMON_LORD) {
        bgTexture = graphics->getTexture("demon_bg");
        if (!bgTexture) {
            bgTexture = graphics->loadTexture("assets/textures/bg/demon_bg.png", "demon_bg");
        }
    } else {
        bgTexture = graphics->getTexture("battle_bg");
        if (!bgTexture) {
            bgTexture = graphics->loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
    }
    return bgTexture;
}

