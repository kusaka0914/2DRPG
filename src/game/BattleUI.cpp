#include "BattleUI.h"
#include "BattleConstants.h"
#include <cmath>
#include <algorithm>
#include <random>

BattleUI::BattleUI(Graphics* graphics, std::shared_ptr<Player> player, Enemy* enemy,
                   BattleLogic* battleLogic, BattleAnimationController* animationController)
    : graphics(graphics), player(player), enemy(enemy),
      battleLogic(battleLogic), animationController(animationController) {
}

void BattleUI::renderJudgeAnimation(const JudgeRenderParams& params) {
    if (params.currentJudgingTurnIndex >= params.commandTurnCount) return;
    
    int screenWidth = graphics->getScreenWidth();
    int screenHeight = graphics->getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    int leftX = screenWidth / 4;
    int rightX = screenWidth * 3 / 4;
    
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
    
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    
    SDL_Texture* playerTex = graphics->getTexture("player");
    if (playerTex) {
        graphics->drawTexture(playerTex, playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int enemyHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    
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
        graphics->drawTexture(enemyTex, enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
    
    // 住民戦の場合は10ターン制限を表示
    std::string turnText;
    if (params.residentTurnCount > 0) {
        turnText = "ターン " + std::to_string(params.residentTurnCount) + " / 10";
    } else {
        turnText = "ターン " + std::to_string(params.currentJudgingTurnIndex + 1) + " / " + std::to_string(params.commandTurnCount);
    }
    SDL_Color turnColor = {255, 255, 255, 255};
    
    // ターンテキストの背景を描画
    SDL_Texture* turnTexture = graphics->createTextTexture(turnText, "default", turnColor);
    if (turnTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(turnTexture, nullptr, nullptr, &textWidth, &textHeight);
        
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        int bgX = 20 - padding;
        int bgY = 20 - padding;
        int bgWidth = textWidth + padding * 2;
        int bgHeight = textHeight + padding * 2;
        
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
        
        SDL_DestroyTexture(turnTexture);
    }
    
    graphics->drawText(turnText, 20, 20, "default", turnColor);
    
    renderHP(playerBaseX, playerBaseY, enemyBaseX, enemyBaseY, playerHeight, enemyHeight, params.residentBehaviorHint);
    
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
        
        // プレイヤーコマンドの背景を描画（黒）
        SDL_Texture* playerCmdTexture = graphics->createTextTexture(playerCmd, "default", playerCmdColor);
        if (playerCmdTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerCmdTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            // テキストを大きく表示
            int scaledWidth = static_cast<int>(textWidth * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
            int scaledHeight = static_cast<int>(textHeight * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
            
            int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_LARGE;
            int bgX = cmdTextX - padding;
            int bgY = cmdTextY - padding;
            int bgWidth = scaledWidth + padding * 2;
            int bgHeight = scaledHeight + padding * 2;
            
            // 黒い背景
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
            
            // スケールしたテキストを描画
            graphics->drawTexture(playerCmdTexture, cmdTextX, cmdTextY, scaledWidth, scaledHeight);
            
            SDL_DestroyTexture(playerCmdTexture);
        } else {
            // フォールバック
            graphics->drawText(playerCmd, cmdTextX, cmdTextY, "default", playerCmdColor);
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
            std::string vsText = "VS";
            SDL_Color vsTextColor = {255, 255, 255, 255}; // 白文字
            
            // VSテキストを大きく表示（コマンドとコマンドの間、中央に配置）
            SDL_Texture* vsTexture = graphics->createTextTexture(vsText, "default", vsTextColor);
            if (vsTexture) {
                int textWidth, textHeight;
                SDL_QueryTexture(vsTexture, nullptr, nullptr, &textWidth, &textHeight);
                
                // バンと出るアニメーション（スケールアニメーション）
                float vsScale = BattleConstants::JUDGE_VS_BASE_SCALE;
                if (params.judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
                    // SHOW_ENEMY_COMMANDフェーズではアニメーション
                    float progress = std::min(1.0f, params.judgeDisplayTimer / BattleConstants::JUDGE_VS_ANIMATION_DURATION);
                    float eased = progress * progress * (3.0f - 2.0f * progress); // smoothstep
                    vsScale = BattleConstants::JUDGE_VS_BASE_SCALE * eased; // 0からbaseScaleへ
                }
                // SHOW_RESULTフェーズでは最大スケールで固定表示
                
                int scaledWidth = static_cast<int>(textWidth * vsScale);
                int scaledHeight = static_cast<int>(textHeight * vsScale);
                
                // 中央に配置（コマンドと同じY位置）
                int vsX = centerX - scaledWidth / 2;
                int vsY = centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET - scaledHeight / 2;
                
                // 黒背景を描画
                int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_LARGE;
                int bgX = centerX - (scaledWidth / 2) - padding;
                int bgY = centerY + BattleConstants::JUDGE_COMMAND_Y_OFFSET - (scaledHeight / 2) - padding;
                int bgWidth = scaledWidth + padding * 2;
                int bgHeight = scaledHeight + padding * 2;
                
                graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
                graphics->setDrawColor(255, 255, 255, 255);
                graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
                
                // スケールしたテキストを描画
                graphics->drawTexture(vsTexture, vsX, vsY, scaledWidth, scaledHeight);
                
                SDL_DestroyTexture(vsTexture);
            } else {
                // フォールバック
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
                
                // 敵コマンドの背景を描画（黒）
                SDL_Texture* enemyCmdTexture = graphics->createTextTexture(enemyCmd, "default", enemyCmdColor);
                if (enemyCmdTexture) {
                    int textWidth, textHeight;
                    SDL_QueryTexture(enemyCmdTexture, nullptr, nullptr, &textWidth, &textHeight);
                    
                    // テキストを大きく表示
                    int scaledWidth = static_cast<int>(textWidth * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
                    int scaledHeight = static_cast<int>(textHeight * BattleConstants::JUDGE_COMMAND_TEXT_SCALE);
                    
                    int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_LARGE;
                    int bgX = cmdTextX - padding;
                    int bgY = cmdTextY - padding;
                    int bgWidth = scaledWidth + padding * 2;
                    int bgHeight = scaledHeight + padding * 2;
                    
                    // 黒い背景
                    graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                    graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
                    graphics->setDrawColor(255, 255, 255, 255);
                    graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
                    
                    // スケールしたテキストを描画
                    graphics->drawTexture(enemyCmdTexture, cmdTextX, cmdTextY, scaledWidth, scaledHeight);
                    
                    SDL_DestroyTexture(enemyCmdTexture);
                } else {
                    // フォールバック
                    graphics->drawText(enemyCmd, cmdTextX, cmdTextY, "default", enemyCmdColor);
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
        
        std::string resultText;
        SDL_Color resultColor;
        
        float scaleProgress = std::min(1.0f, params.judgeDisplayTimer / BattleConstants::JUDGE_RESULT_SCALE_ANIMATION_DURATION);
        float scale = BattleConstants::JUDGE_RESULT_MIN_SCALE + scaleProgress * BattleConstants::JUDGE_RESULT_SCALE_RANGE;
        
        if (result == 1) {
            resultText = "勝ち！";
            resultColor = {255, 215, 0, 255};
        } else if (result == -1) {
            resultText = "負け...";
            resultColor = {255, 0, 0, 255};
        } else {
            resultText = "引き分け";
            resultColor = {200, 200, 200, 255};
        }
        
        int textWidth = 200;
        int textHeight = 60;
        int scaledWidth = (int)(textWidth * scale);
        int scaledHeight = (int)(textHeight * scale);
        int textX = centerX - scaledWidth / 2;
        int textY = centerY - scaledHeight / 2;
        
        graphics->setDrawColor(resultColor.r / 2, resultColor.g / 2, resultColor.b / 2, 150);
        graphics->drawRect(textX - 20, textY - 20, scaledWidth + 40, scaledHeight + 40, true);
        
        graphics->drawText(resultText, textX, textY, "default", resultColor);
        
        if (result == 1) {
            float glowProgress = std::sin(params.judgeDisplayTimer * 3.14159f * 4.0f) * 0.5f + 0.5f;
            graphics->setDrawColor(255, 215, 0, (Uint8)(glowProgress * 100));
            graphics->drawRect(textX - 30, textY - 30, scaledWidth + 60, scaledHeight + 60, false);
        }
    }
}

void BattleUI::renderCommandSelectionUI(const CommandSelectRenderParams& params) {
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
    
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    
    SDL_Texture* playerTex = graphics->getTexture("player");
    if (playerTex) {
        graphics->drawTexture(playerTex, playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int enemyHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    
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
        graphics->drawTexture(enemyTex, enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
    
    // 住民戦の場合は10ターン制限を表示
    if (params.residentTurnCount > 0) {
        renderTurnNumber(params.residentTurnCount, 10, params.isDesperateMode);
    } else {
        renderTurnNumber(params.currentSelectingTurn + 1, params.commandTurnCount, params.isDesperateMode);
    }
    
    // 選択済みコマンドを表示（ボタンの上）
    auto playerCmds = battleLogic->getPlayerCommands();
    if (params.currentSelectingTurn > 0) {
        std::string selectedCommandsText = "";
        for (int i = 0; i < params.currentSelectingTurn; i++) {
            if (i > 0) {
                selectedCommandsText += " → ";
            }
            selectedCommandsText += BattleLogic::getCommandName(playerCmds[i]);
        }
        
        SDL_Color selectedCmdColor = {255, 255, 255, 255};
        int selectedCmdY = centerY - 150; // ボタンの上に配置
        
        // 選択済みコマンドのテキスト背景を描画
        SDL_Texture* selectedCmdTexture = graphics->createTextTexture(selectedCommandsText, "default", selectedCmdColor);
        if (selectedCmdTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(selectedCmdTexture, nullptr, nullptr, &textWidth, &textHeight);
            
            int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
            int bgX = centerX - 200 - padding;
            int bgY = selectedCmdY - padding;
            int bgWidth = textWidth + padding * 2;
            int bgHeight = textHeight + padding * 2;
            
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
            
            SDL_DestroyTexture(selectedCmdTexture);
        }
        
        graphics->drawText(selectedCommandsText, centerX - 200, selectedCmdY, "default", selectedCmdColor);
    }
    
    auto& cmdSelectState = animationController->getCommandSelectState();
    float buttonSlideProgress = std::min(1.0f, cmdSelectState.commandSelectSlideProgress);
    int baseY = centerY - 50;
    int slideOffset = (int)((1.0f - buttonSlideProgress) * 200);
    
    int buttonWidth = 200;
    int buttonHeight = 60;
    int buttonSpacing = 80;
    int startX = centerX - (buttonWidth / 2);
    int startY = baseY + slideOffset;
    
    for (size_t i = 0; i < params.currentOptions->size(); i++) {
        int buttonY = startY + (int)(i * buttonSpacing);
        
        bool isSelected = (static_cast<int>(i) == params.selectedOption);
        
        SDL_Color bgColor;
        if (isSelected) {
            float glowProgress = std::sin(cmdSelectState.commandSelectAnimationTimer * 3.14159f * 4.0f) * 0.3f + 0.7f;
            bgColor = {(Uint8)(100 * glowProgress), (Uint8)(200 * glowProgress), (Uint8)(255 * glowProgress), 200};
        } else {
            bgColor = {50, 50, 50, 150};
        }
        
        graphics->setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        graphics->drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, true);
        
        if (isSelected) {
            SDL_Color borderColor = {255, 215, 0, 255};
            graphics->setDrawColor(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            graphics->drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, false);
            graphics->drawRect(startX - 8, buttonY - 3, buttonWidth + 16, buttonHeight + 6, false);
        } else {
            SDL_Color borderColor = {150, 150, 150, 255};
            graphics->setDrawColor(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            graphics->drawRect(startX - 10, buttonY - 5, buttonWidth + 20, buttonHeight + 10, false);
        }
        
        SDL_Color textColor;
        if (isSelected) {
            textColor = {255, 255, 255, 255};
        } else {
            textColor = {200, 200, 200, 255};
        }
        
        int textX = startX + (buttonWidth / 2) - 50;
        int textY = buttonY + (buttonHeight / 2) - 15;
        
        if (isSelected) {
            graphics->drawText("▶", textX - 30, textY, "default", {255, 215, 0, 255});
            graphics->drawText((*params.currentOptions)[i], textX, textY, "default", textColor);
        } else {
            graphics->drawText((*params.currentOptions)[i], textX, textY, "default", textColor);
        }
    }
    
    // 住民戦の場合は「Qで戻る」を表示しない
    std::string hintText;
    if (params.residentTurnCount > 0) {
        hintText = "↑↓で選択  Enterで決定";
    } else {
        hintText = "↑↓で選択  Enterで決定  Qで戻る";
    }
    SDL_Color hintColor = {255, 255, 255, 255};
    
    // 選択方法のテキスト背景を描画
    SDL_Texture* hintTexture = graphics->createTextTexture(hintText, "default", hintColor);
    if (hintTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(hintTexture, nullptr, nullptr, &textWidth, &textHeight);
        
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        int bgX = centerX - 120 - padding;
        int bgY = screenHeight - 100 - padding;
        int bgWidth = textWidth + padding * 2;
        int bgHeight = textHeight + padding * 2;
        
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
        
        SDL_DestroyTexture(hintTexture);
    }
    
    graphics->drawText(hintText, centerX - 120, screenHeight - 100, "default", hintColor);
    
    renderHP(playerBaseX, playerBaseY, enemyBaseX, enemyBaseY, playerHeight, enemyHeight, params.residentBehaviorHint);
}

void BattleUI::renderResultAnnouncement(const ResultAnnouncementRenderParams& params) {
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
    
    // メイン結果テキスト
    std::string mainText;
    SDL_Color mainColor;
    
    if (params.isVictory) {
        if (params.isDesperateMode) {
            mainText = "🎉 一発逆転成功！ 🎉";
        } else {
            mainText = "🎯 勝利！";
        }
        mainColor = {255, 215, 0, 255};
    } else if (params.isDefeat) {
        if (params.isDesperateMode) {
            mainText = "💀 大敗北... 💀";
        } else {
            mainText = "❌ 敗北...";
        }
        mainColor = {255, 0, 0, 255};
    } else {
        mainText = "⚖️ 引き分け";
        mainColor = {200, 200, 200, 255};
    }
    
    auto& resultState = animationController->getResultState();
    float displayScale = (resultState.resultScale > 0.0f) ? resultState.resultScale : 1.0f;
    displayScale = std::max(0.5f, displayScale); // 最小スケールを0.5fに設定
    int textWidth = 400;
    int textHeight = 100;
    int scaledWidth = (int)(textWidth * displayScale);
    int scaledHeight = (int)(textHeight * displayScale);
    int textX = centerX - scaledWidth / 2;
    int textY = centerY - scaledHeight / 2 - 100;
    
    float glowIntensity = std::sin(resultState.resultAnimationTimer * 3.14159f * 4.0f) * 0.3f + 0.7f;
    graphics->setDrawColor((Uint8)(mainColor.r * glowIntensity * 0.5f), 
                          (Uint8)(mainColor.g * glowIntensity * 0.5f), 
                          (Uint8)(mainColor.b * glowIntensity * 0.5f), 
                          200);
    graphics->drawRect(textX - 30, textY - 30, scaledWidth + 60, scaledHeight + 60, true);
    
    if (params.isVictory) {
        float outerGlow = std::sin(resultState.resultAnimationTimer * 3.14159f * 6.0f) * 0.5f + 0.5f;
        graphics->setDrawColor(255, 215, 0, (Uint8)(outerGlow * 150));
        graphics->drawRect(textX - 50, textY - 50, scaledWidth + 100, scaledHeight + 100, false);
        graphics->drawRect(textX - 45, textY - 45, scaledWidth + 90, scaledHeight + 90, false);
    }
    
    if (params.hasThreeWinStreak && params.isVictory) {
        float streakScale = 0.5f + std::sin(resultState.resultAnimationTimer * 3.14159f * 4.0f) * 0.3f;
        std::string streakText = "🔥 3連勝！ 🔥";
        SDL_Color streakColor = {255, 215, 0, 255};
        
        int streakTextWidth = 300;
        int streakTextHeight = 80;
        int streakScaledWidth = (int)(streakTextWidth * streakScale);
        int streakScaledHeight = (int)(streakTextHeight * streakScale);
        int streakTextX = centerX - streakScaledWidth / 2;
        int streakTextY = centerY - 150 - streakScaledHeight / 2;
        
        graphics->setDrawColor(255, 200, 0, 200);
        graphics->drawRect(streakTextX - 20, streakTextY - 20, streakScaledWidth + 40, streakScaledHeight + 40, true);
        
        float glowProgress = std::sin(resultState.resultAnimationTimer * 3.14159f * 6.0f) * 0.5f + 0.5f;
        graphics->setDrawColor(255, 255, 0, (Uint8)(glowProgress * 150));
        graphics->drawRect(streakTextX - 30, streakTextY - 30, streakScaledWidth + 60, streakScaledHeight + 60, false);
        graphics->drawRect(streakTextX - 25, streakTextY - 25, streakScaledWidth + 50, streakScaledHeight + 50, false);
        
        graphics->drawText(streakText, streakTextX, streakTextY, "default", streakColor);
    }
    
    graphics->drawText(mainText, textX, textY, "default", mainColor);
    
    
    if (params.hasThreeWinStreak && params.isVictory) {
        std::string bonusText = "✨ ダメージ1.5倍ボーナス！ ✨";
        SDL_Color bonusColor = {255, 255, 100, 255};
        graphics->drawText(bonusText, centerX - 150, centerY + 80, "default", bonusColor);
    }
    
    
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
    
    auto& charState = animationController->getCharacterState();
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
    int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyX = enemyBaseX + (int)charState.enemyAttackOffsetX + (int)charState.enemyHitOffsetX;
    int enemyY = enemyBaseY + (int)charState.enemyAttackOffsetY + (int)charState.enemyHitOffsetY;
    
    int playerWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int playerHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    int enemyWidth = BattleConstants::BATTLE_CHARACTER_SIZE;
    int enemyHeight = BattleConstants::BATTLE_CHARACTER_SIZE;
    
    renderHP(playerX, playerY, enemyX, enemyY, playerHeight, enemyHeight, params.residentBehaviorHint);
    
    renderCharacters(playerX, playerY, enemyX, enemyY, playerWidth, playerHeight, enemyWidth, enemyHeight);
}

void BattleUI::renderCharacters(int playerX, int playerY, int enemyX, int enemyY,
                                 int playerWidth, int playerHeight, int enemyWidth, int enemyHeight) {
    SDL_Texture* playerTex = graphics->getTexture("player");
    if (playerTex) {
        graphics->drawTexture(playerTex, playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(playerX - playerWidth / 2, playerY - playerHeight / 2, playerWidth, playerHeight, false);
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
        graphics->drawTexture(enemyTex, enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(enemyX - enemyWidth / 2, enemyY - enemyHeight / 2, enemyWidth, enemyHeight, false);
    }
}

void BattleUI::renderHP(int playerX, int playerY, int enemyX, int enemyY,
                        int playerHeight, int enemyHeight, const std::string& residentBehaviorHint) {
    SDL_Color whiteColor = {255, 255, 255, 255};
    int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
    
    // プレイヤーの名前とレベル（HPの上に表示）- 住民戦でも表示
    std::string playerNameText = player->getName() + " Lv." + std::to_string(player->getLevel());
    SDL_Texture* playerNameTexture = graphics->createTextTexture(playerNameText, "default", whiteColor);
    if (playerNameTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(playerNameTexture, nullptr, nullptr, &textWidth, &textHeight);
        int bgX = playerX - 100 - padding;
        int bgY = playerY - playerHeight / 2 - 80 - padding;
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
        SDL_DestroyTexture(playerNameTexture);
    }
    graphics->drawText(playerNameText, playerX - 100, playerY - playerHeight / 2 - 80, "default", whiteColor);
    
    // 住民戦の場合はプレイヤーのHP表示をスキップ
    if (!enemy->isResident()) {
        std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
        SDL_Texture* playerHpTexture = graphics->createTextTexture(playerHpText, "default", whiteColor);
        if (playerHpTexture) {
            int textWidth, textHeight;
            SDL_QueryTexture(playerHpTexture, nullptr, nullptr, &textWidth, &textHeight);
            int bgX = playerX - 100 - padding;
            int bgY = playerY - playerHeight / 2 - 40 - padding;
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
            SDL_DestroyTexture(playerHpTexture);
        }
        graphics->drawText(playerHpText, playerX - 100, playerY - playerHeight / 2 - 40, "default", whiteColor);
        
        // ステータス上昇呪文の状態を表示（HPの下）
        if (player->hasNextTurnBonusActive()) {
            float multiplier = player->getNextTurnMultiplier();
            int turns = player->getNextTurnBonusTurns();
            // 倍率を文字列に変換（小数点以下1桁まで表示）
            int multiplierInt = static_cast<int>(multiplier * 10);
            std::string multiplierStr = std::to_string(multiplierInt / 10) + "." + std::to_string(multiplierInt % 10);
            std::string statusText = "攻撃倍率: " + multiplierStr + "倍 (残り" + std::to_string(turns) + "ターン)";
            SDL_Color statusColor = {255, 255, 100, 255}; // 黄色
            SDL_Texture* statusTexture = graphics->createTextTexture(statusText, "default", statusColor);
            if (statusTexture) {
                int textWidth, textHeight;
                SDL_QueryTexture(statusTexture, nullptr, nullptr, &textWidth, &textHeight);
                int bgX = playerX - 100 - padding;
                int bgY = playerY - playerHeight / 2 + padding;
                graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
                graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
                graphics->setDrawColor(255, 255, 100, 255);
                graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
                SDL_DestroyTexture(statusTexture);
            }
            graphics->drawText(statusText, playerX - 100, playerY - playerHeight / 2 + padding, "default", statusColor);
        }
    }
    
    // 敵の名前とレベル（HPの上に表示）
    // 住民の場合は住民の名前を使用、それ以外は通常の敵名を使用
    std::string enemyName = enemy->isResident() ? enemy->getName() : enemy->getTypeName();
    std::string enemyNameText = enemyName + " Lv." + std::to_string(enemy->getLevel());
    SDL_Texture* enemyNameTexture = graphics->createTextTexture(enemyNameText, "default", whiteColor);
    if (enemyNameTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(enemyNameTexture, nullptr, nullptr, &textWidth, &textHeight);
        int bgX = enemyX - 100 - padding;
        int bgY = enemyY - enemyHeight / 2 - 80 - padding;
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
        SDL_DestroyTexture(enemyNameTexture);
    }
    graphics->drawText(enemyNameText, enemyX - 100, enemyY - enemyHeight / 2 - 80, "default", whiteColor);
    
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
    
    if (hasHint) {
        SDL_Texture* hintTexture = graphics->createTextTexture(hintText, "default", whiteColor);
        if (hintTexture) {
            int hintWidth, hintHeight;
            SDL_QueryTexture(hintTexture, nullptr, nullptr, &hintWidth, &hintHeight);
            int hintBgX = enemyX - 100 - padding;
            int hintBgY = enemyY - enemyHeight / 2 - 40 - padding;
            graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
            graphics->drawRect(hintBgX, hintBgY, hintWidth + padding * 2, hintHeight + padding * 2, true);
            graphics->setDrawColor(255, 255, 255, 255);
            graphics->drawRect(hintBgX, hintBgY, hintWidth + padding * 2, hintHeight + padding * 2, false);
            graphics->drawText(hintText, enemyX - 100, enemyY - enemyHeight / 2 - 40, "default", whiteColor);
            SDL_DestroyTexture(hintTexture);
        }
    }
    
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Texture* enemyHpTexture = graphics->createTextTexture(enemyHpText, "default", whiteColor);
    if (enemyHpTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(enemyHpTexture, nullptr, nullptr, &textWidth, &textHeight);
        int bgX = enemyX - 100 - padding;
        // ヒントが表示されている場合は、HPの位置を下にずらす（住民戦でも通常戦でもヒントを表示するため）
        int hpOffsetY = hasHint ? 40 : 0;
        int bgY = enemyY - enemyHeight / 2 - 40 - padding + hpOffsetY;
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, textWidth + padding * 2, textHeight + padding * 2, false);
        SDL_DestroyTexture(enemyHpTexture);
    }
    // ヒントが表示されている場合は、HPの位置を下にずらす（住民戦でも通常戦でもヒントを表示するため）
    int hpOffsetY = hasHint ? 40 : 0;
    graphics->drawText(enemyHpText, enemyX - 100, enemyY - enemyHeight / 2 - 40 + hpOffsetY, "default", whiteColor);
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
        int padding = BattleConstants::JUDGE_COMMAND_TEXT_PADDING_SMALL;
        int bgX = 20 - padding;
        int bgY = 20 - padding;
        int bgWidth = textWidth + padding * 2;
        int bgHeight = textHeight + padding * 2;
        
        graphics->setDrawColor(0, 0, 0, BattleConstants::BATTLE_BACKGROUND_ALPHA);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, true);
        graphics->setDrawColor(255, 255, 255, 255);
        graphics->drawRect(bgX, bgY, bgWidth, bgHeight, false);
        
        // テキストを描画
        SDL_Color turnColor = {255, 255, 255, 255};
        graphics->drawText(turnText, 20, 20, "default", turnColor);
        
        SDL_DestroyTexture(textTexture);
    } else {
        // フォールバック：通常のテキスト描画
        SDL_Color turnColor = {255, 255, 255, 255};
        graphics->drawText(turnText, 20, 20, "default", turnColor);
    }
}

SDL_Texture* BattleUI::getBattleBackgroundTexture() const {
    // 住民の場合は夜の背景、それ以外は通常の戦闘背景を使用
    SDL_Texture* bgTexture = nullptr;
    if (enemy->isResident()) {
        bgTexture = graphics->getTexture("night_bg");
        if (!bgTexture) {
            bgTexture = graphics->loadTexture("assets/textures/bg/night_bg.png", "night_bg");
        }
    } else {
        bgTexture = graphics->getTexture("battle_bg");
        if (!bgTexture) {
            bgTexture = graphics->loadTexture("assets/textures/bg/battle_bg.png", "battle_bg");
        }
    }
    return bgTexture;
}

