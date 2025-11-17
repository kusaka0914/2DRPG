#include "MainMenuState.h"
#include "FieldState.h"
#include "RoomState.h"
#include "BattleState.h"
#include "../core/utils/ui_config_manager.h"
#include <sstream>
#include <cstdlib>

MainMenuState::MainMenuState(std::shared_ptr<Player> player) : player(player) {
    isFadingOut = false;
    isFadingIn = false;
    fadeTimer = 0.0f;
    fadeDuration = 0.5f;
}

void MainMenuState::enter() {
    setupUI();
    // updatePlayerInfo();
    isFadingOut = false;
    isFadingIn = false;
    fadeTimer = 0.0f;
}

void MainMenuState::exit() {
    ui.clear();
    isFadingOut = false; // 状態遷移時にフラグをリセット
    isFadingIn = false;
}

void MainMenuState::update(float deltaTime) {
    ui.update(deltaTime);
    
    // フェード更新処理
    updateFade(deltaTime);
    
    // フェードアウト完了後、状態遷移
    if (isFadingOut && fadeTimer >= fadeDuration) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<RoomState>(player));
        }
    }
    
    static bool lastReloadState = false;
    auto& config = UIConfig::UIConfigManager::getInstance();
    bool currentReloadState = config.checkAndReloadConfig();
    
    if (!lastReloadState && currentReloadState) {
        setupUI();
    }
    lastReloadState = currentReloadState;
}

void MainMenuState::render(Graphics& graphics) {
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.clear();
    
    // タイトル背景画像を描画
    SDL_Texture* titleBg = graphics.getTexture("title_bg");
    if (titleBg) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        // 背景画像を画面全体に描画（アスペクト比は無視）
        graphics.drawTexture(titleBg, 0, 0, screenWidth, screenHeight);
    }
    
    // タイトルロゴ画像を描画
    SDL_Texture* titleLogo = graphics.getTexture("title_logo");
    if (titleLogo) {
        int screenWidth = graphics.getScreenWidth();
        int screenHeight = graphics.getScreenHeight();
        
        // 画像サイズを取得
        int textureWidth, textureHeight;
        SDL_QueryTexture(titleLogo, nullptr, nullptr, &textureWidth, &textureHeight);
        
        // 画面中央に配置（アスペクト比を保持）
        float aspectRatio = static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
        int displayWidth, displayHeight;
        
        // 画面幅の60%を基準にサイズを計算
        int baseWidth = static_cast<int>(screenWidth * 0.4f);
        if (textureWidth > textureHeight) {
            displayWidth = baseWidth;
            displayHeight = static_cast<int>(baseWidth / aspectRatio);
        } else {
            displayHeight = static_cast<int>(baseWidth / aspectRatio);
            displayWidth = baseWidth;
        }
        
        int logoX = (screenWidth - displayWidth) / 2;
        int logoY = (screenHeight - displayHeight) / 2 - 50; // 少し上に配置
        
        graphics.drawTexture(titleLogo, logoX, logoY, displayWidth, displayHeight);
    }
    
    // "START GAME : PRESS ENTER" テキストを表示
    int screenWidth = graphics.getScreenWidth();
    int screenHeight = graphics.getScreenHeight();
    std::string startText = "START GAME : PRESS ENTER";
    SDL_Color textColor = {255, 255, 255, 255}; // 白
    SDL_Texture* startTexture = graphics.createTextTexture(startText, "default", textColor);
    if (startTexture) {
        int textWidth, textHeight;
        SDL_QueryTexture(startTexture, nullptr, nullptr, &textWidth, &textHeight);
        int textX = (screenWidth - textWidth) / 2; // 中央
        int textY = screenHeight - 100; // 画面下部から100px上
        graphics.drawTexture(startTexture, textX, textY, textWidth, textHeight);
        SDL_DestroyTexture(startTexture);
    } else {
        // フォールバック：通常のテキスト描画
        graphics.drawText(startText, screenWidth / 2, screenHeight - 100, "default", textColor);
    }
    
    ui.render(graphics);
    
    // フェードオーバーレイを描画
    renderFade(graphics);
    
    graphics.present();
}

void MainMenuState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (input.isKeyJustPressed(InputKey::ENTER) && !isFading()) {
        // Enterキーでフェードアウト開始
        startFadeOut(0.5f, [this]() {
            // フェードアウト完了時のコールバック（必要に応じて使用）
        });
    }
    
    if (input.isKeyJustPressed(InputKey::ESCAPE)) {
        // ゲーム終了処理
    }
}

void MainMenuState::setupUI() {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto mainMenuConfig = config.getMainMenuConfig();
    
    // タイトルテキストは画像に置き換えたため、ラベルは作成しない
    
    int playerInfoX, playerInfoY;
    config.calculatePosition(playerInfoX, playerInfoY, mainMenuConfig.playerInfo.position, 1100, 650);
    playerInfoLabel = std::make_unique<Label>(playerInfoX, playerInfoY, "", "default");
    playerInfoLabel->setColor(mainMenuConfig.playerInfo.color);
    ui.addElement(std::move(playerInfoLabel));
    
    // // ステータス確認ボタン
    // auto statusBtn = std::make_unique<Button>(300, 270, 200, 50, "ステータス確認");
    // statusBtn->setColors({0, 0, 100, 255}, {0, 0, 150, 255}, {0, 0, 50, 255});
    // statusBtn->setOnClick([this]() {
    //     updatePlayerInfo();
    // });
    // ui.addElement(std::move(statusBtn));
    
    // innBtn->setColors({100, 0, 100, 255}, {150, 0, 150, 255}, {50, 0, 50, 255});
    // innBtn->setOnClick([this]() {
    //     const int innCost = 10;
    //     if (player->getGold() >= innCost) {
    //         if (player->getHp() < player->getMaxHp() || player->getMp() < player->getMaxMp()) {
    //             player->gainGold(-innCost);
    //             player->heal(player->getMaxHp());
    //             player->restoreMp(player->getMaxMp());
    //             updatePlayerInfo();
    //         }
    //     }
    // });
    // ui.addElement(std::move(innBtn));
    // auto skeletonLordBtn = std::make_unique<Button>(300, 480, 200, 50, "スケルトンと戦う");
    // skeletonLordBtn->setColors({100, 0, 0, 255}, {150, 0, 0, 255}, {50, 0, 0, 255});
    // skeletonLordBtn->setOnClick([this]() {
    //     if (stateManager) {
    //         for (int i = 0; i < 99; i++) {
    //             player->levelUp();
    //         }
    //         auto skeleton = std::make_unique<Enemy>(EnemyType::DEMON_LORD);
    //         stateManager->changeState(std::make_unique<BattleState>(player, std::move(skeleton)));
    //     }
    // });
    // ui.addElement(std::move(skeletonLordBtn));

    // auto demonLordBtn = std::make_unique<Button>(300, 480, 200, 50, "魔王と戦う");
    // demonLordBtn->setColors({100, 0, 0, 255}, {150, 0, 0, 255}, {50, 0, 0, 255});
    // demonLordBtn->setOnClick([this]() {
    //     if (stateManager) {
    //         for (int i = 0; i < 100; i++) {
    //             player->levelUp();
    //         }
    //         auto demon = std::make_unique<Enemy>(EnemyType::DEMON_LORD);
    //         stateManager->changeState(std::make_unique<BattleState>(player, std::move(demon)));
    //     }
    // });
    // ui.addElement(std::move(demonLordBtn));
    
    // // ゲーム終了ボタン
    // auto quitBtn = std::make_unique<Button>(300, 410, 200, 50, "ゲーム終了");
    // quitBtn->setColors({100, 0, 0, 255}, {150, 0, 0, 255}, {50, 0, 0, 255});
    // quitBtn->setOnClick([this]() {
    //     std::exit(0);
    // });
    // ui.addElement(std::move(quitBtn));
}

void MainMenuState::updatePlayerInfo() {
    if (!player) return;
    
    std::ostringstream info;
    info << "【" << player->getName() << "】\n";
    info << "レベル: " << player->getLevel() << "\n";
    info << "HP: " << player->getHp() << "/" << player->getMaxHp() << "\n";
    info << "MP: " << player->getMp() << "/" << player->getMaxMp() << "\n";
    info << "ゴールド: " << player->getGold() << "\n";
    info << "経験値: " << player->getExp();
    
    if (ui.getElements().size() > 1) {
        Label* playerInfoLabel = dynamic_cast<Label*>(ui.getElements()[1].get());
        if (playerInfoLabel) {
            playerInfoLabel->setText(info.str());
        }
    }
} 