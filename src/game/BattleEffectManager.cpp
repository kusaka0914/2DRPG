#include "BattleEffectManager.h"
#include <random>
#include <cmath>
#include <algorithm>

BattleEffectManager::BattleEffectManager() {
    resetAll();
}

void BattleEffectManager::triggerHitEffect(int damage, float x, float y, bool isPlayerHit) {
    HitEffect effect;
    effect.timer = 1.5f;
    effect.x = x;
    effect.y = y;
    effect.damage = damage;
    effect.isPlayerHit = isPlayerHit;
    effect.scale = 0.0f;
    effect.rotation = 0.0f;
    effect.alpha = 255.0f;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDis(0.0f, 3.14159f * 2.0f);
    std::uniform_real_distribution<float> speedDis(50.0f, 200.0f);
    std::uniform_real_distribution<float> lifeDis(0.3f, 1.0f);
    
    for (int i = 0; i < 30; i++) {
        float angle = angleDis(gen);
        float speed = speedDis(gen);
        effect.particlesX.push_back(x);
        effect.particlesY.push_back(y);
        effect.particlesVX.push_back(std::cos(angle) * speed);
        effect.particlesVY.push_back(std::sin(angle) * speed);
        effect.particlesLife.push_back(lifeDis(gen));
    }
    
    hitEffects.push_back(effect);
}

void BattleEffectManager::updateHitEffects(float deltaTime) {
    for (auto it = hitEffects.begin(); it != hitEffects.end();) {
        it->timer -= deltaTime;
        
        for (size_t i = 0; i < it->particlesX.size(); i++) {
            it->particlesX[i] += it->particlesVX[i] * deltaTime;
            it->particlesY[i] += it->particlesVY[i] * deltaTime;
            it->particlesLife[i] -= deltaTime;
            it->particlesVX[i] *= 0.95f;
            it->particlesVY[i] *= 0.95f;
        }
        
        // スケールアニメーション
        if (it->timer > 1.2f) {
            float progress = (1.5f - it->timer) / 0.3f;
            it->scale = progress * 1.5f;
        } else {
            float progress = (1.2f - it->timer) / 1.2f;
            it->scale = 1.5f - progress * 0.5f;
        }
        
        // 回転アニメーション
        it->rotation += deltaTime * 360.0f;
        
        // 透明度アニメーション
        if (it->timer < 0.5f) {
            it->alpha = (it->timer / 0.5f) * 255.0f;
        }
        
        // タイマーが切れたら削除
        if (it->timer <= 0.0f) {
            it = hitEffects.erase(it);
        } else {
            ++it;
        }
    }
}

void BattleEffectManager::renderHitEffects(Graphics& graphics) {
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    
    for (const auto& effect : hitEffects) {
        // 画面フラッシュ（ダメージを受けた時に一瞬真っ赤になる）
        // 複数のレイヤーで派手なエフェクトを実現
        if (effect.timer > 1.4f) {
            float flashProgress = (1.5f - effect.timer) / 0.1f;
            
            // 第1レイヤー：強い赤色フラッシュ
            Uint8 flashAlpha1 = (Uint8)((1.0f - flashProgress) * 200);
            graphics.setDrawColor(255, 0, 0, flashAlpha1);
            graphics.drawRect(0, 0, screenWidth, screenHeight, true);
            
            // 第2レイヤー：白色のパルス（一瞬の閃光）
            float pulse = std::sin(flashProgress * 3.14159f * 4.0f) * 0.5f + 0.5f;
            Uint8 flashAlpha2 = (Uint8)((1.0f - flashProgress) * 100 * pulse);
            graphics.setDrawColor(255, 255, 255, flashAlpha2);
            graphics.drawRect(0, 0, screenWidth, screenHeight, true);
        }
        
        // パーティクルエフェクト（赤色と黄色を混ぜる）
        for (size_t i = 0; i < effect.particlesX.size(); i++) {
            if (effect.particlesLife[i] > 0.0f) {
                float lifeProgress = effect.particlesLife[i] / 1.0f;
                Uint8 particleAlpha = (Uint8)(lifeProgress * 255.0f);
                
                // パーティクルを赤色と黄色で交互に
                if (i % 2 == 0) {
                    graphics.setDrawColor(255, 0, 0, particleAlpha);
                } else {
                    graphics.setDrawColor(255, 215, 0, particleAlpha);
                }
                
                int particleSize = (int)(lifeProgress * 10.0f); // サイズを大きく
                graphics.drawRect((int)effect.particlesX[i] - particleSize / 2, 
                                 (int)effect.particlesY[i] - particleSize / 2, 
                                 particleSize, particleSize, true);
            }
        }
        
        std::string damageText = std::to_string(effect.damage);
        
        int textX = (int)effect.x;
        int textY = (int)(effect.y - 50.0f * effect.scale);
        
        // ダメージテキスト（大きく、回転、スケールアニメーション）
        float textScale = 1.0f + std::sin(effect.rotation * 3.14159f / 180.0f * 2.0f) * 0.3f; // パルス効果
        SDL_Color damageColor = {255, 255, 255, (Uint8)effect.alpha}; // 白色で目立つ
        
        // テキストの影（赤色）
        SDL_Color shadowColor = {255, 0, 0, (Uint8)(effect.alpha * 0.8f)};
        graphics.drawText(damageText, textX - 50 + 2, textY + 2, "default", shadowColor);
        
        // メインテキスト
        graphics.drawText(damageText, textX - 50, textY, "default", damageColor);
        
        // 画面全体のパルスエフェクト（ダメージの瞬間）
        if (effect.timer > 1.45f) {
            float pulseProgress = (1.5f - effect.timer) / 0.05f;
            float pulse = std::sin(pulseProgress * 3.14159f * 8.0f) * 0.5f + 0.5f;
            Uint8 pulseAlpha = (Uint8)((1.0f - pulseProgress) * 80.0f * pulse);
            graphics.setDrawColor(255, 100, 100, pulseAlpha);
            graphics.drawRect(0, 0, screenWidth, screenHeight, true);
        }
    }
}

void BattleEffectManager::triggerScreenShake(float intensity, float duration, bool victoryShake, bool targetPlayer) {
    shakeState.shakeIntensity = intensity;
    shakeState.shakeTimer = duration;
    shakeState.isVictoryShake = victoryShake;
    shakeState.shakeTargetPlayer = targetPlayer;
}

void BattleEffectManager::updateScreenShake(float deltaTime) {
    if (shakeState.shakeTimer > 0.0f) {
        shakeState.shakeTimer -= deltaTime;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        
        float currentIntensity;
        if (shakeState.isVictoryShake) {
            float progress = 1.0f - (shakeState.shakeTimer / (shakeState.shakeTimer + deltaTime));
            float pulse = std::sin(progress * 3.14159f * 4.0f);
            currentIntensity = shakeState.shakeIntensity * (1.0f - progress * 0.5f) * (0.5f + 0.5f * std::abs(pulse));
        } else {
            currentIntensity = shakeState.shakeIntensity * (shakeState.shakeTimer / (shakeState.shakeTimer + deltaTime));
        }
        
        shakeState.shakeOffsetX = dis(gen) * currentIntensity;
        shakeState.shakeOffsetY = dis(gen) * currentIntensity;
        
        if (shakeState.shakeTimer <= 0.0f) {
            shakeState.shakeTimer = 0.0f;
            shakeState.shakeOffsetX = 0.0f;
            shakeState.shakeOffsetY = 0.0f;
            shakeState.shakeIntensity = 0.0f;
            shakeState.isVictoryShake = false;
            shakeState.shakeTargetPlayer = false;
        }
    } else {
        shakeState.shakeOffsetX = 0.0f;
        shakeState.shakeOffsetY = 0.0f;
        shakeState.isVictoryShake = false;
        shakeState.shakeTargetPlayer = false;
    }
}

void BattleEffectManager::clearHitEffects() {
    hitEffects.clear();
}

void BattleEffectManager::resetAll() {
    hitEffects.clear();
    shakeState = ScreenShakeState{};
}

