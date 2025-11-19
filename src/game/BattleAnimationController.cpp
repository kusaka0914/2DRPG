#include "BattleAnimationController.h"
#include <cmath>

BattleAnimationController::BattleAnimationController() {
    resetAll();
}

void BattleAnimationController::updateResultCharacterAnimation(float deltaTime, bool isVictory, bool isDefeat,
                                                                float resultTimer, bool& damageAppliedInAnimation,
                                                                bool hasThreeWinStreak,
                                                                float animStartTime, float animDuration) {
    float animTime = resultTimer - animStartTime;
    
    if (animTime < 0.0f) {
        characterState = CharacterAnimationState{};
        return;
    }
    
    if (animTime > animDuration) {
        characterState = CharacterAnimationState{};
        return;
    }
    
    float progress = animTime / animDuration;
    
    if (isVictory) {
        if (progress < 0.2f) {
            float punchProgress = progress / 0.2f;
            float eased = punchProgress * punchProgress * (3.0f - 2.0f * punchProgress);
            characterState.playerAttackOffsetX = 200.0f * eased;
            characterState.playerAttackOffsetY = -40.0f * std::sin(punchProgress * 3.14159f);
        } else if (progress < 0.4f) {
            float returnProgress = (progress - 0.2f) / 0.2f;
            float bounce = std::sin(returnProgress * 3.14159f) * 0.3f;
            characterState.playerAttackOffsetX = 200.0f * (1.0f - returnProgress) + bounce * 20.0f;
            characterState.playerAttackOffsetY = -10.0f * std::sin(returnProgress * 3.14159f);
        } else {
            characterState.playerAttackOffsetX = 0.0f;
            characterState.playerAttackOffsetY = 0.0f;
        }
        
        if (progress >= 0.15f && progress < 0.4f) {
            float hitProgress = (progress - 0.15f) / 0.25f;
            float impact = std::sin(hitProgress * 3.14159f);
            characterState.enemyHitOffsetX = 80.0f * (1.0f - hitProgress * 0.7f);
            characterState.enemyHitOffsetY = 30.0f * impact * (1.0f - hitProgress * 0.5f);
        } else if (progress >= 0.4f && progress < 0.7f) {
            float shakeProgress = (progress - 0.4f) / 0.3f;
            characterState.enemyHitOffsetX = 15.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            characterState.enemyHitOffsetY = 15.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
        } else {
            characterState.enemyHitOffsetX = 0.0f;
            characterState.enemyHitOffsetY = 0.0f;
        }
        
        characterState.attackAnimationFrame = (int)(progress * 10.0f) % 3;
        
    } else if (isDefeat) {
        if (progress < 0.2f) {
            float punchProgress = progress / 0.2f;
            float eased = punchProgress * punchProgress * (3.0f - 2.0f * punchProgress);
            characterState.enemyAttackOffsetX = -200.0f * eased;
            characterState.enemyAttackOffsetY = -40.0f * std::sin(punchProgress * 3.14159f);
        } else if (progress < 0.4f) {
            float returnProgress = (progress - 0.2f) / 0.2f;
            float bounce = std::sin(returnProgress * 3.14159f) * 0.3f;
            characterState.enemyAttackOffsetX = -200.0f * (1.0f - returnProgress) - bounce * 20.0f;
            characterState.enemyAttackOffsetY = -10.0f * std::sin(returnProgress * 3.14159f);
        } else {
            characterState.enemyAttackOffsetX = 0.0f;
            characterState.enemyAttackOffsetY = 0.0f;
        }
        
        if (progress >= 0.15f && progress < 0.4f) {
            float hitProgress = (progress - 0.15f) / 0.25f;
            float impact = std::sin(hitProgress * 3.14159f);
            characterState.playerHitOffsetX = -80.0f * (1.0f - hitProgress * 0.7f);
            characterState.playerHitOffsetY = 30.0f * impact * (1.0f - hitProgress * 0.5f);
        } else if (progress >= 0.4f && progress < 0.7f) {
            float shakeProgress = (progress - 0.4f) / 0.3f;
            characterState.playerHitOffsetX = -15.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            characterState.playerHitOffsetY = 15.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
        } else {
            characterState.playerHitOffsetX = 0.0f;
            characterState.playerHitOffsetY = 0.0f;
        }
        
        characterState.attackAnimationFrame = (int)(progress * 10.0f) % 3;
        
    } else {
        if (progress < 0.2f) {
            float punchProgress = progress / 0.2f;
            float eased = punchProgress * punchProgress;
            characterState.playerAttackOffsetX = 120.0f * eased;
            characterState.playerAttackOffsetY = -30.0f * std::sin(punchProgress * 3.14159f);
            characterState.enemyAttackOffsetX = -120.0f * eased;
            characterState.enemyAttackOffsetY = -30.0f * std::sin(punchProgress * 3.14159f);
        } else if (progress < 0.4f) {
            float returnProgress = (progress - 0.2f) / 0.2f;
            characterState.playerAttackOffsetX = 120.0f * (1.0f - returnProgress);
            characterState.playerAttackOffsetY = 0.0f;
            characterState.enemyAttackOffsetX = -120.0f * (1.0f - returnProgress);
            characterState.enemyAttackOffsetY = 0.0f;
        } else {
            characterState.playerAttackOffsetX = 0.0f;
            characterState.playerAttackOffsetY = 0.0f;
            characterState.enemyAttackOffsetX = 0.0f;
            characterState.enemyAttackOffsetY = 0.0f;
        }
        
        if (progress >= 0.15f && progress < 0.4f) {
            float hitProgress = (progress - 0.15f) / 0.25f;
            float impact = std::sin(hitProgress * 3.14159f);
            characterState.playerHitOffsetX = -50.0f * (1.0f - hitProgress * 0.7f);
            characterState.playerHitOffsetY = 20.0f * impact * (1.0f - hitProgress * 0.5f);
            characterState.enemyHitOffsetX = 50.0f * (1.0f - hitProgress * 0.7f);
            characterState.enemyHitOffsetY = 20.0f * impact * (1.0f - hitProgress * 0.5f);
        } else if (progress >= 0.4f && progress < 0.7f) {
            float shakeProgress = (progress - 0.4f) / 0.3f;
            characterState.playerHitOffsetX = -10.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            characterState.playerHitOffsetY = 10.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
            characterState.enemyHitOffsetX = 10.0f * std::sin(shakeProgress * 3.14159f * 6.0f);
            characterState.enemyHitOffsetY = 10.0f * std::cos(shakeProgress * 3.14159f * 6.0f);
        } else {
            characterState.playerHitOffsetX = 0.0f;
            characterState.playerHitOffsetY = 0.0f;
            characterState.enemyHitOffsetX = 0.0f;
            characterState.enemyHitOffsetY = 0.0f;
        }
    }
}

void BattleAnimationController::updateResultAnimation(float deltaTime, bool isDesperateMode) {
    resultState.resultAnimationTimer += deltaTime;
    
    // 拡大アニメーション
    if (resultState.resultAnimationTimer < BattleConstants::RESULT_SCALE_ANIMATION_DURATION) {
        resultState.resultScale = resultState.resultAnimationTimer / BattleConstants::RESULT_SCALE_ANIMATION_DURATION;
    } else {
        // 脈動効果
        float pulseFreq = isDesperateMode ? 
            BattleConstants::DESPERATE_RESULT_PULSE_FREQUENCY : 
            BattleConstants::RESULT_PULSE_FREQUENCY;
        float pulseAmp = isDesperateMode ? 
            BattleConstants::DESPERATE_RESULT_PULSE_AMPLITUDE : 
            BattleConstants::RESULT_PULSE_AMPLITUDE;
        float pulse = std::sin((resultState.resultAnimationTimer - BattleConstants::RESULT_SCALE_ANIMATION_DURATION) * pulseFreq) * pulseAmp;
        resultState.resultScale = 1.0f + pulse;
    }
    
    // 回転アニメーション
    if (resultState.resultAnimationTimer < BattleConstants::RESULT_ROTATION_ANIMATION_DURATION) {
        float rotationDegrees = isDesperateMode ? 
            BattleConstants::DESPERATE_RESULT_ROTATION_DEGREES : 
            BattleConstants::RESULT_ROTATION_DEGREES;
        resultState.resultRotation = resultState.resultAnimationTimer * rotationDegrees;
    } else {
        resultState.resultRotation = 0.0f;
    }
}

void BattleAnimationController::resetResultAnimation() {
    resultState = ResultAnimationState{};
}

void BattleAnimationController::updateCommandSelectAnimation(float deltaTime) {
    commandSelectState.commandSelectAnimationTimer += deltaTime;
    commandSelectState.commandSelectSlideProgress = 
        std::min(1.0f, commandSelectState.commandSelectAnimationTimer / BattleConstants::COMMAND_SELECT_ANIMATION_DURATION);
}

void BattleAnimationController::resetCommandSelectAnimation() {
    commandSelectState = CommandSelectAnimationState{};
}

void BattleAnimationController::resetAll() {
    characterState = CharacterAnimationState{};
    resultState = ResultAnimationState{};
    commandSelectState = CommandSelectAnimationState{};
}

