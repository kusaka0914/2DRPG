#include "MainMenuState.h"
#include "FieldState.h"
#include "RoomState.h"
#include "BattleState.h"
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
}

void MainMenuState::render(Graphics& graphics) {
    // 背景色を設定
    graphics.setDrawColor(0, 0, 0, 255);
    graphics.clear();
    
    ui.render(graphics);
    
    graphics.present();
}

void MainMenuState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // ESCキーでゲーム終了（実装は後で）
    if (input.isKeyJustPressed(InputKey::ESCAPE)) {
        // ゲーム終了処理
    }
}

void MainMenuState::setupUI() {
    ui.clear();
    
    // タイトルラベル
    titleLabel = std::make_unique<Label>(350, 250, "勇者だって強者に逆らえない。", "title");
    titleLabel->setColor({255, 215, 0, 255}); // ゴールド色
    ui.addElement(std::move(titleLabel));
    
    // プレイヤー情報ラベル
    playerInfoLabel = std::make_unique<Label>(50, 120, "", "default");
    ui.addElement(std::move(playerInfoLabel));
    
    // 冒険に出るボタン
    auto adventureBtn = std::make_unique<Button>(450, 350, 200, 50, "冒険に出る");
    adventureBtn->setColors({0, 100, 0, 255}, {0, 150, 0, 255}, {0, 50, 0, 255});
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
    
    // 宿屋で休むボタン
    // auto innBtn = std::make_unique<Button>(300, 340, 200, 50, "宿屋で休む (10G)");
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

    // 魔王と戦うボタン(プレイヤーレベル100にする)
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
    //     // ゲーム終了処理（後で実装）
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
    
    // UIマネージャーからプレイヤー情報ラベルを取得して更新
    if (ui.getElements().size() > 1) {
        Label* playerInfoLabel = dynamic_cast<Label*>(ui.getElements()[1].get());
        if (playerInfoLabel) {
            playerInfoLabel->setText(info.str());
        }
    }
} 