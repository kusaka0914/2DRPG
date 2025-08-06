#include "NightState.h"
#include "TownState.h"
#include "GameOverState.h"
#include "Graphics.h"
#include "InputManager.h"
#include <iostream>
#include <random>
#include <algorithm>

// 静的変数の初期化
int NightState::totalResidentsKilled = 0;

NightState::NightState(std::shared_ptr<Player> player)
    : player(player), playerX(12), playerY(15), 
      isStealthMode(true), stealthLevel(1), residentsKilled(0),
      moveTimer(0), playerTexture(nullptr), residentTexture(nullptr), guardTexture(nullptr),
      messageLabel(nullptr), isShowingMessage(false) {
    
    // 住民の位置を初期化（6名分散配置）
    residents = {
        {8, 8},   // 左上
        {22, 8},  // 右上
        {5, 12},  // 左中
        {25, 12}, // 右中
        {10, 16}, // 左下
        {20, 16}  // 右下
    };
    
    // 見張りの位置を初期化（4人分散配置、画面内に収める）
    guards = {
        {7, 7},   // 左上の住民近く
        {21, 7},  // 右上の住民近く
        {7, 17},  // 左下の住民近く
        {21, 17}  // 右下の住民近く
    };
    
    // 衛兵の移動方向を初期化（円運動用）
    guardDirections = {
        {1, 1},   // 右下方向
        {-1, 1},  // 左下方向
        {1, -1},  // 右上方向
        {-1, -1}  // 左上方向
    };
}

void NightState::enter() {
    std::cout << "夜間モードに入りました" << std::endl;
    player->setNightTime(true);
    showMessage("夜の街に潜入しました。住民を襲撃して街を壊滅させましょう。");
    setupUI();
}

void NightState::exit() {
    std::cout << "夜間モードを終了しました" << std::endl;
    player->setNightTime(false);
    
    // 街に戻る時にタイマーを再起動
    TownState::s_nightTimerActive = true;
    TownState::s_nightTimer = 300.0f; // 5分 = 300秒
}

void NightState::update(float deltaTime) {
    moveTimer -= deltaTime;
    ui.update(deltaTime);
    
    // 衛兵の更新
    updateGuards(deltaTime);
}

void NightState::render(Graphics& graphics) {
    // 初回のみテクスチャを読み込み
    if (!playerTexture) {
        loadTextures(graphics);
    }
    
    // 夜の街を描画
    drawNightTown(graphics);
    
    // 住民を描画
    for (auto& resident : residents) {
        if (residentTexture) {
            graphics.drawTexture(residentTexture, resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        } else {
            // テクスチャがない場合は赤い四角で表示
            graphics.setDrawColor(255, 0, 0, 255);
            graphics.drawRect(resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // 見張りを描画
    for (auto& guard : guards) {
        if (guardTexture) {
            graphics.drawTexture(guardTexture, guard.first * TILE_SIZE, guard.second * TILE_SIZE, TILE_SIZE, TILE_SIZE);
        } else {
            // テクスチャがない場合は青い四角で表示
            graphics.setDrawColor(0, 0, 255, 255);
            graphics.drawRect(guard.first * TILE_SIZE, guard.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // プレイヤーを描画
    drawPlayer(graphics);
    
    // メンタルなどのUIを表示（街と同じ見た目）
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
    
    // UI更新
    updateUI();
    
    // UI描画
    ui.render(graphics);
    
    graphics.present();
}

void NightState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // メッセージ表示中の場合はスペースキーでクリア
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
            // 3人倒した後のメッセージの場合は街に戻る
            if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
                if (stateManager) {
                    // 夜の回数を増加
                    TownState::s_nightCount++;
                    // タイマーを再起動して街に戻る
                    TownState::s_nightTimerActive = true;
                    TownState::s_nightTimer = 300.0f; // 5分 = 300秒
                    stateManager->changeState(std::make_unique<TownState>(player));
                }
            }
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    // ESCキーで街に戻る
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            // 夜の回数を増加
            TownState::s_nightCount++;
            // タイマーを再起動して街に戻る
            TownState::s_nightTimerActive = true;
            TownState::s_nightTimer = 300.0f; // 5分 = 300秒
            stateManager->changeState(std::make_unique<TownState>(player));
        }
        return;
    }
    
    // スペースキーで住民を襲撃
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        checkResidentInteraction();
        return;
    }
    
    // プレイヤー移動
    handleMovement(input);
}

void NightState::setupUI() {
    ui.clear();
    
    // メッセージ表示用
    auto messageLabelPtr = std::make_unique<Label>(50, 450, "", "default");
    messageLabelPtr->setColor({255, 255, 255, 255});
    messageLabel = messageLabelPtr.get();
    ui.addElement(std::move(messageLabelPtr));
}

void NightState::handleMovement(const InputManager& input) {
    // 共通の移動処理を使用
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void NightState::checkResidentInteraction() {
    // 近くの住民をチェック
    for (auto& resident : residents) {
        if (GameUtils::isNearPositionEuclidean(playerX, playerY, resident.first, resident.second, 1.5f)) {
            attackResident(resident.first, resident.second);
            return;
        }
    }
    
    showMessage("近くに住民がいません。");
}

void NightState::attackResident(int x, int y) {
    if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
        showMessage("今夜はもう十分です。これ以上は危険です。\n街に戻ります。");
        // メッセージをクリアしたら街に戻る
        return;
    }
    
    // 衛兵の近くで襲撃した場合のチェック
    if (isNearGuard(x, y)) {
        showMessage("衛兵に見つかりました！王様からの信頼度が0になりました。");
        player->setKingTrust(0); // 王様からの信頼度を0に設定
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "衛兵に見つかりました。王様からの信頼度が0になりました。"));
        }
        return;
    }
    
    // メンタルに応じた成功率を計算
    int mental = player->getMental();
    int successRate = 50; // 基本成功率50%
    
    // メンタルが低いほど成功率が下がる
    if (mental < 30) {
        successRate = 20;
    } else if (mental < 50) {
        successRate = 35;
    } else if (mental > 80) {
        successRate = 70; // メンタルが高いと成功率が上がる
    }
    
    // ランダム判定
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    if (dis(gen) <= successRate) {
        // 成功：住民を倒した
        residentsKilled++;
        
        // 信頼度の変更
        player->changeKingTrust(-10); // 王様からの信頼度を10減少
        player->changeDemonTrust(10); // 魔王からの信頼度を10上昇
        
        // メンタルの変更
        totalResidentsKilled++; // 合計倒した人数を増加
        if (totalResidentsKilled >= 6) { // 合計6人以上倒した場合
            player->changeMental(20); // メンタルを20上昇
        } else {
            player->changeMental(-20); // メンタルを20減少
        }
        
        // 住民を削除
        residents.erase(std::remove_if(residents.begin(), residents.end(),
            [x, y](const std::pair<int, int>& pos) {
                return pos.first == x && pos.second == y;
            }), residents.end());
        
        showMessage("住民を倒しました。\n王様からの信頼度-10\n魔王からの信頼度+10\nメンタル" + 
                   std::string(totalResidentsKilled >= 6 ? "+20" : "-20"));
        
        // 3人倒した場合の処理
        if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
            showMessage("今夜はもう十分です。これ以上は危険です。\n街に戻ります。");
        }
    } else {
        // 失敗
        player->changeDemonTrust(-10); // 魔王からの信頼度を10減少
        showMessage("住民を倒すのに失敗しました。\n魔王からの信頼度-10");
    }
}

void NightState::hideEvidence() {
    player->performEvilAction();
    showMessage("証拠を隠滅しました。");
}

void NightState::showMessage(const std::string& message) {
    GameState::showMessage(message, messageLabel, isShowingMessage);
}

void NightState::clearMessage() {
    GameState::clearMessage(messageLabel, isShowingMessage);
}

void NightState::drawNightTown(Graphics& graphics) {
    // 夜の街を描画（暗い背景）
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 30; x++) {
            int drawX = x * TILE_SIZE;
            int drawY = y * TILE_SIZE;
            
            // 暗い街の背景
            graphics.setDrawColor(30, 30, 50, 255);
            graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    // 住民を描画（赤い点）
    graphics.setDrawColor(255, 100, 100, 255);
    for (auto& resident : residents) {
        int drawX = resident.first * TILE_SIZE + 8;
        int drawY = resident.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
    
    // 見張りを描画（黄色い点）
    graphics.setDrawColor(255, 255, 100, 255);
    for (auto& guard : guards) {
        int drawX = guard.first * TILE_SIZE + 8;
        int drawY = guard.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
}

void NightState::drawPlayer(Graphics& graphics) {
    if (playerTexture) {
        graphics.drawTexture(playerTexture, playerX * TILE_SIZE, playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    } else {
        // テクスチャがない場合は緑色の四角で表示
        graphics.setDrawColor(0, 255, 0, 255);
        graphics.drawRect(playerX * TILE_SIZE, playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

bool NightState::isValidPosition(int x, int y) const {
    return x >= 0 && x < 30 && y >= 0 && y < 20;
} 

void NightState::loadTextures(Graphics& graphics) {
    if (!playerTexture) {
        playerTexture = graphics.loadTexture("assets/characters/player.png", "player");
    }
    if (!residentTexture) {
        residentTexture = graphics.loadTexture("assets/characters/resident_1.png", "resident"); // 住民用にresident_1.pngを使用
    }
    if (!guardTexture) {
        guardTexture = graphics.loadTexture("assets/characters/guard.png", "guard"); // 見張り用にguard.pngを使用
    }
}

void NightState::updateGuards(float deltaTime) {
    static float guardMoveTimer = 0;
    guardMoveTimer += deltaTime;
    
    // 衛兵の移動間隔（1.5秒ごと）
    if (guardMoveTimer >= 1.5f) {
        guardMoveTimer = 0;
        
        for (size_t i = 0; i < guards.size(); ++i) {
            // 各衛兵の担当する住民の位置を決定
            int targetResidentIndex = i % residents.size();
            int targetX = residents[targetResidentIndex].first;
            int targetY = residents[targetResidentIndex].second;
            
            // 現在の衛兵の位置
            int currentX = guards[i].first;
            int currentY = guards[i].second;
            
            // 住民の周りを円運動するように移動
            int dx = targetX - currentX;
            int dy = targetY - currentY;
            
            // 住民から2-3マス離れた位置を維持
            int distance = std::max(abs(dx), abs(dy));
            if (distance < 2) {
                // 住民に近すぎる場合は離れる
                if (dx > 0) dx = 2;
                else if (dx < 0) dx = -2;
                if (dy > 0) dy = 2;
                else if (dy < 0) dy = -2;
            } else if (distance > 4) {
                // 住民から遠すぎる場合は近づく
                dx = dx * 3 / 4;
                dy = dy * 3 / 4;
            }
            
            // 新しい位置を計算
            int newX = currentX + guardDirections[i].first;
            int newY = currentY + guardDirections[i].second;
            
            // 境界チェック（画面内に制限）
            if (newX < 1 || newX >= 29 || newY < 1 || newY >= 19) {
                // 方向を反転
                guardDirections[i].first = -guardDirections[i].first;
                guardDirections[i].second = -guardDirections[i].second;
                newX = currentX + guardDirections[i].first;
                newY = currentY + guardDirections[i].second;
                
                // 再度境界チェック
                if (newX < 1 || newX >= 29 || newY < 1 || newY >= 19) {
                    // まだ境界外の場合は現在位置を維持
                    newX = currentX;
                    newY = currentY;
                }
            }
            
            // 住民の近くにいる場合は円運動
            if (distance <= 4) {
                // 時計回りに90度回転
                int tempX = guardDirections[i].first;
                int tempY = guardDirections[i].second;
                guardDirections[i].first = -tempY;
                guardDirections[i].second = tempX;
            }
            
            // 位置を更新
            guards[i].first = newX;
            guards[i].second = newY;
        }
    }
}

bool NightState::isNearGuard(int x, int y) const {
    // 衛兵を中心とした正方形の視界（最大2マス先まで）
    for (const auto& guard : guards) {
        int dx = abs(x - guard.first);
        int dy = abs(y - guard.second);
        
        // 正方形の視界（マンハッタン距離で2マス以内）
        if (dx <= 2 && dy <= 2) {
            return true;
        }
    }
    return false;
}

void NightState::updateUI() {
    // 信頼度チェック
    checkTrustLevels();
}

void NightState::checkTrustLevels() {
    // 信頼度が0になった場合のゲームオーバー
    if (player->getDemonTrust() <= 0) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "魔王からの信頼度が0になりました。処刑されました。"));
        }
    } else if (player->getKingTrust() <= 0) {
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "王様からの信頼度が0になりました。処刑されました。"));
        }
    }
} 