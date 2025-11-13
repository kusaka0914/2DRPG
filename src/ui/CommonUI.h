#pragma once
#include "../gfx/Graphics.h"
#include "../entities/Player.h"
#include <memory>

class CommonUI {
public:
    static void drawNightTimer(Graphics& graphics, float nightTimer, bool nightTimerActive, bool showGameExplanation);
    static void drawTargetLevel(Graphics& graphics, int targetLevel, bool levelGoalAchieved, int currentLevel);
    static void drawTrustLevels(Graphics& graphics, std::shared_ptr<Player> player, bool nightTimerActive, bool showGameExplanation);
    static void drawGameControllerStatus(Graphics& graphics, bool gameControllerConnected);
}; 