#include "BattleUI.h"
#include <cmath>
#include <algorithm>

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
    
    graphics->setDrawColor(0, 0, 0, 220);
    graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerWidth = 300;
    int playerHeight = 300;
    
    SDL_Texture* playerTex = graphics->getTexture("player");
    if (playerTex) {
        graphics->drawTexture(playerTex, playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyWidth = 300;
    int enemyHeight = 300;
    
    SDL_Texture* enemyTex = graphics->getTexture("enemy_" + enemy->getTypeName());
    if (enemyTex) {
        graphics->drawTexture(enemyTex, enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
    
    std::string turnText = "ターン " + std::to_string(params.currentJudgingTurnIndex + 1) + " / " + std::to_string(params.commandTurnCount);
    SDL_Color turnColor = {255, 255, 255, 255};
    graphics->drawText(turnText, 20, 20, "default", turnColor);
    
    renderHP(playerBaseX, playerBaseY, enemyBaseX, enemyBaseY, playerHeight, enemyHeight);
    
    int playerDisplayX = leftX;
    if (params.judgeSubPhase == JudgeSubPhase::SHOW_PLAYER_COMMAND) {
        float slideProgress = std::min(1.0f, params.judgeDisplayTimer / 0.3f);
        int slideOffset = (int)((1.0f - slideProgress) * 300);
        playerDisplayX = leftX - slideOffset;
    }
    
    if (params.judgeSubPhase >= JudgeSubPhase::SHOW_PLAYER_COMMAND) {
        SDL_Color playerNameColor = {100, 200, 255, 255};
        graphics->drawText(player->getName(), playerDisplayX - 100, centerY + 180, "default", playerNameColor);
        
        auto playerCmds = battleLogic->getPlayerCommands();
        std::string playerCmd = BattleLogic::getCommandName(playerCmds[params.currentJudgingTurnIndex]);
        SDL_Color playerCmdColor = {150, 220, 255, 255};
        int cmdTextX = playerDisplayX - 100;
        int cmdTextY = centerY + 220;
        
        graphics->drawText(playerCmd, cmdTextX, cmdTextY, "default", playerCmdColor);
        
        if (params.judgeSubPhase == JudgeSubPhase::SHOW_PLAYER_COMMAND) {
            float glowProgress = std::sin(params.judgeDisplayTimer * 3.14159f * 2.0f) * 0.5f + 0.5f;
            Uint8 glowAlpha = (Uint8)(glowProgress * 100);
            graphics->setDrawColor(100, 200, 255, glowAlpha);
            graphics->drawRect(cmdTextX - 10, cmdTextY - 5, 220, 40, false);
        }
    }
    
    int enemyDisplayX = rightX;
    if (params.judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        float slideProgress = std::min(1.0f, params.judgeDisplayTimer / 0.3f);
        int slideOffset = (int)((1.0f - slideProgress) * 300);
        enemyDisplayX = rightX + slideOffset;
    }
    
    if (params.judgeSubPhase >= JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        SDL_Color enemyNameColor = {255, 100, 100, 255};
        graphics->drawText(enemy->getTypeName(), enemyDisplayX - 100, centerY + 180, "default", enemyNameColor);
        
        auto enemyCmds = battleLogic->getEnemyCommands();
        std::string enemyCmd = BattleLogic::getCommandName(enemyCmds[params.currentJudgingTurnIndex]);
        SDL_Color enemyCmdColor = {255, 150, 150, 255};
        int cmdTextX = enemyDisplayX - 100;
        int cmdTextY = centerY + 220;
        
        graphics->drawText(enemyCmd, cmdTextX, cmdTextY, "default", enemyCmdColor);
        
        if (params.judgeSubPhase == JudgeSubPhase::SHOW_ENEMY_COMMAND) {
            float glowProgress = std::sin(params.judgeDisplayTimer * 3.14159f * 2.0f) * 0.5f + 0.5f;
            Uint8 glowAlpha = (Uint8)(glowProgress * 100);
            graphics->setDrawColor(255, 100, 100, glowAlpha);
            graphics->drawRect(cmdTextX - 10, cmdTextY - 5, 220, 40, false);
        }
    }
    
    if (params.judgeSubPhase >= JudgeSubPhase::SHOW_ENEMY_COMMAND) {
        SDL_Color vsColor = {255, 255, 255, 255};
        float blinkProgress = std::sin(params.judgeDisplayTimer * 3.14159f * 4.0f) * 0.5f + 0.5f;
        vsColor.a = (Uint8)(blinkProgress * 255);
        graphics->drawText("VS", centerX - 30, centerY - 200, "default", vsColor);
    }
    
    if (params.judgeSubPhase == JudgeSubPhase::SHOW_RESULT) {
        auto playerCmds = battleLogic->getPlayerCommands();
        auto enemyCmds = battleLogic->getEnemyCommands();
        int result = battleLogic->judgeRound(playerCmds[params.currentJudgingTurnIndex], 
                               enemyCmds[params.currentJudgingTurnIndex]);
        
        std::string resultText;
        SDL_Color resultColor;
        
        float scaleProgress = std::min(1.0f, params.judgeDisplayTimer / 0.5f);
        float scale = 0.3f + scaleProgress * 0.7f;
        
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
    
    graphics->setDrawColor(0, 0, 0, 180);
    graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerWidth = 300;
    int playerHeight = 300;
    
    SDL_Texture* playerTex = graphics->getTexture("player");
    if (playerTex) {
        graphics->drawTexture(playerTex, playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight);
    } else {
        graphics->setDrawColor(100, 200, 255, 255);
        graphics->drawRect(playerBaseX - playerWidth / 2, playerBaseY - playerHeight / 2, playerWidth, playerHeight, true);
    }
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyWidth = 300;
    int enemyHeight = 300;
    
    SDL_Texture* enemyTex = graphics->getTexture("enemy_" + enemy->getTypeName());
    if (enemyTex) {
        graphics->drawTexture(enemyTex, enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight);
    } else {
        graphics->setDrawColor(255, 100, 100, 255);
        graphics->drawRect(enemyBaseX - enemyWidth / 2, enemyBaseY - enemyHeight / 2, enemyWidth, enemyHeight, true);
    }
    
    renderTurnNumber(params.currentSelectingTurn + 1, params.commandTurnCount, params.isDesperateMode);
    
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
    
    std::string hintText = "↑↓で選択  Enterで決定  Qで戻る";
    SDL_Color hintColor = {150, 150, 150, 255};
    graphics->drawText(hintText, centerX - 120, screenHeight - 100, "default", hintColor);
    
    renderHP(playerBaseX, playerBaseY, enemyBaseX, enemyBaseY, playerHeight, enemyHeight);
}

void BattleUI::renderResultAnnouncement(const ResultAnnouncementRenderParams& params) {
    int screenWidth = graphics->getScreenWidth();
    int screenHeight = graphics->getScreenHeight();
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    
    graphics->setDrawColor(0, 0, 0, 220);
    graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    
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
    float displayScale = std::max(0.3f, resultState.resultScale);
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
    
    // スコア表示
    std::string scoreText = "自分 " + std::to_string(params.playerWins) + "勝  " + 
                           "敵 " + std::to_string(params.enemyWins) + "勝";
    SDL_Color scoreColor = {255, 255, 255, 255};
    graphics->drawText(scoreText, centerX - 150, centerY + 50, "default", scoreColor);
    
    if (params.hasThreeWinStreak && params.isVictory) {
        std::string bonusText = "✨ ダメージ1.5倍ボーナス！ ✨";
        SDL_Color bonusColor = {255, 255, 100, 255};
        graphics->drawText(bonusText, centerX - 150, centerY + 80, "default", bonusColor);
    }
    
    // 詳細テキスト
    std::string detailText;
    if (params.isVictory) {
        detailText = "自分が" + std::to_string(params.playerWins) + "ターン分の攻撃を実行！";
    } else if (params.isDefeat) {
        detailText = "敵が" + std::to_string(params.enemyWins) + "ターン分の攻撃を実行！";
    } else {
        detailText = "両方がダメージを受ける";
    }
    SDL_Color detailColor = {200, 200, 200, 255};
    graphics->drawText(detailText, centerX - 200, centerY + 100, "default", detailColor);
    
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
    
    if (params.isDefeat) {
        float darkIntensity = std::sin(resultState.resultAnimationTimer * 3.14159f * 2.0f) * 0.2f + 0.3f;
        graphics->setDrawColor(0, 0, 0, (Uint8)(darkIntensity * 100));
        graphics->drawRect(0, 0, screenWidth, screenHeight, true);
    }
    
    auto& charState = animationController->getCharacterState();
    int playerBaseX = screenWidth / 4;
    int playerBaseY = screenHeight / 2;
    int playerX = playerBaseX + (int)charState.playerAttackOffsetX + (int)charState.playerHitOffsetX;
    int playerY = playerBaseY + (int)charState.playerAttackOffsetY + (int)charState.playerHitOffsetY;
    
    int enemyBaseX = screenWidth * 3 / 4;
    int enemyBaseY = screenHeight / 2;
    int enemyX = enemyBaseX + (int)charState.enemyAttackOffsetX + (int)charState.enemyHitOffsetX;
    int enemyY = enemyBaseY + (int)charState.enemyAttackOffsetY + (int)charState.enemyHitOffsetY;
    
    int playerWidth = 300;
    int playerHeight = 300;
    int enemyWidth = 300;
    int enemyHeight = 300;
    
    renderHP(playerX, playerY, enemyX, enemyY, playerHeight, enemyHeight);
    
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
    
    SDL_Texture* enemyTex = graphics->getTexture("enemy_" + enemy->getTypeName());
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
                        int playerHeight, int enemyHeight) {
    std::string playerHpText = "HP: " + std::to_string(player->getHp()) + "/" + std::to_string(player->getMaxHp());
    SDL_Color playerHpColor = {100, 255, 100, 255};
    graphics->drawText(playerHpText, playerX - 100, playerY - playerHeight / 2 - 40, "default", playerHpColor);
    
    std::string enemyHpText = "HP: " + std::to_string(enemy->getHp()) + "/" + std::to_string(enemy->getMaxHp());
    SDL_Color enemyHpColor = {255, 100, 100, 255};
    graphics->drawText(enemyHpText, enemyX - 100, enemyY - enemyHeight / 2 - 40, "default", enemyHpColor);
}

void BattleUI::renderTurnNumber(int turnNumber, int totalTurns, bool isDesperateMode) {
    std::string turnText = "ターン " + std::to_string(turnNumber) + " / " + std::to_string(totalTurns);
    if (isDesperateMode) {
        turnText += "  ⚡ 大勝負 ⚡";
    }
    SDL_Color turnColor = {255, 255, 255, 255};
    graphics->drawText(turnText, 20, 20, "default", turnColor);
}

