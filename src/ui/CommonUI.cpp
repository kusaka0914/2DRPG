#include "CommonUI.h"
#include "../core/utils/ui_config_manager.h"

void CommonUI::drawNightTimer(Graphics& graphics, float nightTimer, bool nightTimerActive, bool showGameExplanation) {
    if (nightTimerActive && !showGameExplanation) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto commonUIConfig = config.getCommonUIConfig();
        
        int remainingMinutes = static_cast<int>(nightTimer) / 60;
        int remainingSeconds = static_cast<int>(nightTimer) % 60;
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, commonUIConfig.nightTimer.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        SDL_Color bgColor = commonUIConfig.backgroundColor;
        bgColor.a = commonUIConfig.backgroundAlpha;
        graphics.setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        graphics.drawRect(bgX, bgY, commonUIConfig.nightTimer.width, commonUIConfig.nightTimer.height, true);
        graphics.setDrawColor(commonUIConfig.borderColor.r, commonUIConfig.borderColor.g, commonUIConfig.borderColor.b, commonUIConfig.borderColor.a);
        graphics.drawRect(bgX, bgY, commonUIConfig.nightTimer.width, commonUIConfig.nightTimer.height, false);
        
        int textX, textY;
        config.calculatePosition(textX, textY, commonUIConfig.nightTimerText.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        std::string timerText = "夜の街まで: " + std::to_string(remainingMinutes) + ":" + 
                               (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        graphics.drawText(timerText, textX, textY, "default", commonUIConfig.nightTimerText.color);
    }
}

void CommonUI::drawTargetLevel(Graphics& graphics, int targetLevel, bool levelGoalAchieved, int currentLevel) {
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto commonUIConfig = config.getCommonUIConfig();
    
    int bgX, bgY;
    config.calculatePosition(bgX, bgY, commonUIConfig.targetLevel.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    SDL_Color bgColor = commonUIConfig.backgroundColor;
    bgColor.a = commonUIConfig.backgroundAlpha;
    graphics.setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    graphics.drawRect(bgX, bgY, commonUIConfig.targetLevel.width, commonUIConfig.targetLevel.height, true);
    graphics.setDrawColor(commonUIConfig.borderColor.r, commonUIConfig.borderColor.g, commonUIConfig.borderColor.b, commonUIConfig.borderColor.a);
    graphics.drawRect(bgX, bgY, commonUIConfig.targetLevel.width, commonUIConfig.targetLevel.height, false);
    
    if (targetLevel > 0) { // 目標レベルが設定されている場合のみ表示
        int textX, textY;
        config.calculatePosition(textX, textY, commonUIConfig.targetLevelText.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        if (levelGoalAchieved) {
            // 目標達成済み
            std::string goalText = "必要レベル: " + std::to_string(targetLevel);
            graphics.drawText(goalText, textX, textY, "default", commonUIConfig.targetLevelAchievedColor);
            graphics.drawText("夜の街に進出可能", textX, textY + commonUIConfig.targetLevelLineSpacing, "default", commonUIConfig.targetLevelAchievedColor);
        } else {
            // 目標未達成
            std::string goalText = "必要レベル: " + std::to_string(targetLevel);
            graphics.drawText(goalText, textX, textY, "default", commonUIConfig.targetLevelText.color);
            std::string remainingText = "残りレベル: " + std::to_string(targetLevel - currentLevel);
            graphics.drawText(remainingText, textX, textY + commonUIConfig.targetLevelLineSpacing, "default", commonUIConfig.targetLevelRemainingColor);
        }
    }
}

void CommonUI::drawTrustLevels(Graphics& graphics, std::shared_ptr<Player> player, bool nightTimerActive, bool showGameExplanation) {
    if (nightTimerActive && !showGameExplanation) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto commonUIConfig = config.getCommonUIConfig();
        
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, commonUIConfig.trustLevels.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        SDL_Color bgColor = commonUIConfig.backgroundColor;
        bgColor.a = commonUIConfig.backgroundAlpha;
        graphics.setDrawColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        graphics.drawRect(bgX, bgY, commonUIConfig.trustLevels.width, commonUIConfig.trustLevels.height, true);
        graphics.setDrawColor(commonUIConfig.borderColor.r, commonUIConfig.borderColor.g, commonUIConfig.borderColor.b, commonUIConfig.borderColor.a);
        graphics.drawRect(bgX, bgY, commonUIConfig.trustLevels.width, commonUIConfig.trustLevels.height, false);
        
        int textX, textY;
        config.calculatePosition(textX, textY, commonUIConfig.trustLevelsText.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        std::string mentalText = "メンタル: " + std::to_string(player->getMental());
        std::string demonTrustText = "魔王からの信頼: " + std::to_string(player->getDemonTrust());
        std::string kingTrustText = "王様からの信頼: " + std::to_string(player->getKingTrust());
        
        graphics.drawText(mentalText, textX, textY, "default", commonUIConfig.trustLevelsText.color);
        graphics.drawText(demonTrustText, textX, textY + commonUIConfig.trustLevelsLineSpacing1, "default", commonUIConfig.trustLevelsText.color);
        graphics.drawText(kingTrustText, textX, textY + commonUIConfig.trustLevelsLineSpacing2, "default", commonUIConfig.trustLevelsText.color);
    }
}

void CommonUI::drawGameControllerStatus(Graphics& graphics, bool gameControllerConnected) {
    if (gameControllerConnected) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto commonUIConfig = config.getCommonUIConfig();
        
        int x, y;
        config.calculatePosition(x, y, commonUIConfig.gameControllerStatus.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        graphics.setDrawColor(commonUIConfig.gameControllerStatusColor.r, commonUIConfig.gameControllerStatusColor.g, commonUIConfig.gameControllerStatusColor.b, commonUIConfig.gameControllerStatusColor.a);
        graphics.drawRect(x, y, commonUIConfig.gameControllerStatus.width, commonUIConfig.gameControllerStatus.height, true);
        graphics.setDrawColor(commonUIConfig.borderColor.r, commonUIConfig.borderColor.g, commonUIConfig.borderColor.b, commonUIConfig.borderColor.a);
        graphics.drawRect(x, y, commonUIConfig.gameControllerStatus.width, commonUIConfig.gameControllerStatus.height, false);
    }
} 