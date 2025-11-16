#include "MainMenuState.h"
#include "FieldState.h"
#include "RoomState.h"
#include "BattleState.h"
#include "../core/utils/ui_config_manager.h"
#include <sstream>
#include <cstdlib>

MainMenuState::MainMenuState(std::shared_ptr<Player> player) : player(player) {
}

void MainMenuState::enter() {
    setupUI();
    // updatePlayerInfo();
}

void MainMenuState::exit() {
    ui.clear();
}

void MainMenuState::update(float deltaTime) {
    ui.update(deltaTime);
    
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
    
    ui.render(graphics);
    
    graphics.present();
}

void MainMenuState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (input.isKeyJustPressed(InputKey::ESCAPE)) {
        // ゲーム終了処理
    }
}

void MainMenuState::setupUI() {
    ui.clear();
    
    auto& config = UIConfig::UIConfigManager::getInstance();
    auto mainMenuConfig = config.getMainMenuConfig();
    
    int titleX, titleY;
    config.calculatePosition(titleX, titleY, mainMenuConfig.title.position, 1100, 650);
    titleLabel = std::make_unique<Label>(titleX, titleY, "勇者だって強者に逆らえない。", "title");
    titleLabel->setColor(mainMenuConfig.title.color);
    ui.addElement(std::move(titleLabel));
    
    int playerInfoX, playerInfoY;
    config.calculatePosition(playerInfoX, playerInfoY, mainMenuConfig.playerInfo.position, 1100, 650);
    playerInfoLabel = std::make_unique<Label>(playerInfoX, playerInfoY, "", "default");
    playerInfoLabel->setColor(mainMenuConfig.playerInfo.color);
    ui.addElement(std::move(playerInfoLabel));
    
    int btnX, btnY;
    config.calculatePosition(btnX, btnY, mainMenuConfig.adventureButton.position, 1100, 650);
    auto adventureBtn = std::make_unique<Button>(btnX, btnY, mainMenuConfig.adventureButton.width, mainMenuConfig.adventureButton.height, "冒険に出る");
    adventureBtn->setColors(mainMenuConfig.adventureButton.normalColor, mainMenuConfig.adventureButton.hoverColor, mainMenuConfig.adventureButton.pressedColor);
    adventureBtn->setOnClick([this]() {
        if (stateManager) {
            stateManager->changeState(std::make_unique<RoomState>(player));
        }
    });
    ui.addElement(std::move(adventureBtn));
    
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