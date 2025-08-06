#include "CommonUI.h"

void CommonUI::drawNightTimer(Graphics& graphics, float nightTimer, bool nightTimerActive, bool showGameExplanation) {
    if (nightTimerActive && !showGameExplanation) {
        int remainingMinutes = static_cast<int>(nightTimer) / 60;
        int remainingSeconds = static_cast<int>(nightTimer) % 60;
        
        // タイマー背景
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(10, 10, 200, 40, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(10, 10, 200, 40, false);
        
        // タイマーテキスト
        std::string timerText = "夜の街まで: " + std::to_string(remainingMinutes) + ":" + 
                               (remainingSeconds < 10 ? "0" : "") + std::to_string(remainingSeconds);
        graphics.drawText(timerText, 20, 20, "default", {255, 255, 255, 255});
    }
}

void CommonUI::drawTargetLevel(Graphics& graphics, int targetLevel, bool levelGoalAchieved, int currentLevel) {
    // 目標レベル背景
    graphics.setDrawColor(0, 0, 0, 200);
    graphics.drawRect(10, 60, 250, 50, true);
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(10, 60, 250, 50, false);
    
    if (targetLevel > 0) { // 目標レベルが設定されている場合のみ表示
        if (levelGoalAchieved) {
            // 目標達成済み
            std::string goalText = "目標レベル" + std::to_string(targetLevel) + "達成！";
            graphics.drawText(goalText, 20, 70, "default", {0, 255, 0, 255}); // 緑色
            graphics.drawText("夜の街に進出可能", 20, 90, "default", {0, 255, 0, 255});
        } else {
            // 目標未達成
            std::string goalText = "目標レベル: " + std::to_string(targetLevel);
            graphics.drawText(goalText, 20, 70, "default", {255, 255, 255, 255});
            std::string remainingText = "残りレベル: " + std::to_string(targetLevel - currentLevel);
            graphics.drawText(remainingText, 20, 90, "default", {255, 255, 0, 255}); // 黄色
        }
    }
}

void CommonUI::drawTrustLevels(Graphics& graphics, std::shared_ptr<Player> player, bool nightTimerActive, bool showGameExplanation) {
    if (nightTimerActive && !showGameExplanation) {
        // パラメータ背景
        graphics.setDrawColor(0, 0, 0, 200);
        graphics.drawRect(800, 10, 300, 80, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(800, 10, 300, 80, false);
        
        // パラメータテキスト
        std::string mentalText = "メンタル: " + std::to_string(player->getMental());
        std::string demonTrustText = "魔王からの信頼: " + std::to_string(player->getDemonTrust());
        std::string kingTrustText = "王様からの信頼: " + std::to_string(player->getKingTrust());
        
        graphics.drawText(mentalText, 810, 20, "default", {255, 255, 255, 255});
        graphics.drawText(demonTrustText, 810, 40, "default", {255, 100, 100, 255}); // 赤色
        graphics.drawText(kingTrustText, 810, 60, "default", {100, 100, 255, 255}); // 青色
    }
}

void CommonUI::drawGameControllerStatus(Graphics& graphics, bool gameControllerConnected) {
    if (gameControllerConnected) {
        graphics.setDrawColor(0, 255, 0, 255); // 緑色
        graphics.drawRect(700, 10, 80, 20, true);
        graphics.setDrawColor(255, 255, 255, 255);
        graphics.drawRect(700, 10, 80, 20, false);
    }
} 