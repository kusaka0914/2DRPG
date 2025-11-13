#include "CommonUI.h"
#include "../core/utils/ui_config_manager.h"

void CommonUI::drawNightTimer(Graphics& graphics, float nightTimer, bool nightTimerActive, bool showGameExplanation) {
    if (nightTimerActive && !showGameExplanation) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto commonUIConfig = config.getCommonUIConfig();
        
        int remainingMinutes = static_cast<int>(nightTimer) / 60;
        int remainingSeconds = static_cast<int>(nightTimer) % 60;
        
        // タイマー背景（JSONから座標を取得）
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, commonUIConfig.nightTimer.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(bgX, bgY, commonUIConfig.nightTimer.width, commonUIConfig.nightTimer.height, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(bgX, bgY, commonUIConfig.nightTimer.width, commonUIConfig.nightTimer.height, false);
        
        // タイマーテキスト（JSONから座標を取得）
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
    
    // 目標レベル背景（JSONから座標を取得）
    int bgX, bgY;
    config.calculatePosition(bgX, bgY, commonUIConfig.targetLevel.position, graphics.getScreenWidth(), graphics.getScreenHeight());
    graphics.setDrawColor(0, 0, 0, 200);
    graphics.drawRect(bgX, bgY, commonUIConfig.targetLevel.width, commonUIConfig.targetLevel.height, true);
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(bgX, bgY, commonUIConfig.targetLevel.width, commonUIConfig.targetLevel.height, false);
    
    if (targetLevel > 0) { // 目標レベルが設定されている場合のみ表示
        int textX, textY;
        config.calculatePosition(textX, textY, commonUIConfig.targetLevelText.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        
        if (levelGoalAchieved) {
            // 目標達成済み
            std::string goalText = "必要レベル: " + std::to_string(targetLevel);
            SDL_Color greenColor = {0, 255, 0, 255};
            graphics.drawText(goalText, textX, textY, "default", greenColor); // 緑色
            graphics.drawText("夜の街に進出可能", textX, textY + 20, "default", greenColor);
        } else {
            // 目標未達成
            std::string goalText = "必要レベル: " + std::to_string(targetLevel);
            graphics.drawText(goalText, textX, textY, "default", commonUIConfig.targetLevelText.color);
            std::string remainingText = "残りレベル: " + std::to_string(targetLevel - currentLevel);
            SDL_Color yellowColor = {255, 255, 0, 255};
            graphics.drawText(remainingText, textX, textY + 20, "default", yellowColor); // 黄色
        }
    }
}

void CommonUI::drawTrustLevels(Graphics& graphics, std::shared_ptr<Player> player, bool nightTimerActive, bool showGameExplanation) {
    if (nightTimerActive && !showGameExplanation) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto commonUIConfig = config.getCommonUIConfig();
        
        // パラメータ背景（JSONから座標を取得）
        int bgX, bgY;
        config.calculatePosition(bgX, bgY, commonUIConfig.trustLevels.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(bgX, bgY, commonUIConfig.trustLevels.width, commonUIConfig.trustLevels.height, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(bgX, bgY, commonUIConfig.trustLevels.width, commonUIConfig.trustLevels.height, false);
        
        // パラメータテキスト（JSONから座標を取得）
        int textX, textY;
        config.calculatePosition(textX, textY, commonUIConfig.trustLevelsText.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        std::string mentalText = "メンタル: " + std::to_string(player->getMental());
        std::string demonTrustText = "魔王からの信頼: " + std::to_string(player->getDemonTrust());
        std::string kingTrustText = "王様からの信頼: " + std::to_string(player->getKingTrust());
        
        graphics.drawText(mentalText, textX, textY, "default", commonUIConfig.trustLevelsText.color);
        graphics.drawText(demonTrustText, textX, textY + 20, "default", commonUIConfig.trustLevelsText.color);
        graphics.drawText(kingTrustText, textX, textY + 40, "default", commonUIConfig.trustLevelsText.color);
    }
}

void CommonUI::drawGameControllerStatus(Graphics& graphics, bool gameControllerConnected) {
    if (gameControllerConnected) {
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto commonUIConfig = config.getCommonUIConfig();
        
        // ゲームコントローラー状態表示（JSONから座標を取得）
        int x, y;
        config.calculatePosition(x, y, commonUIConfig.gameControllerStatus.position, graphics.getScreenWidth(), graphics.getScreenHeight());
        graphics.setDrawColor(0, 255, 0, 255); // 緑色
        graphics.drawRect(x, y, commonUIConfig.gameControllerStatus.width, commonUIConfig.gameControllerStatus.height, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(x, y, commonUIConfig.gameControllerStatus.width, commonUIConfig.gameControllerStatus.height, false);
    }
} 