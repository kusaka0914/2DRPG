#include "NightState.h"
#include "FieldState.h"
#include "TownState.h"
#include "GameOverState.h"
#include "CastleState.h"
#include "DemonCastleState.h"
#include "BattleState.h"
#include "../entities/Enemy.h"
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include "../ui/CommonUI.h"
#include "../core/utils/ui_config_manager.h"
#include <iostream>
#include <random>
#include <algorithm>

int NightState::residentsKilled = 0;
int NightState::totalResidentsKilled = 0;
std::vector<std::pair<int, int>> NightState::killedResidentPositions = {};
int NightState::s_savedPlayerX = TownLayout::PLAYER_START_X;
int NightState::s_savedPlayerY = TownLayout::PLAYER_START_Y;
bool NightState::s_playerPositionSaved = false;

NightState::NightState(std::shared_ptr<Player> player)
    : player(player), playerX(s_playerPositionSaved ? s_savedPlayerX : TownLayout::PLAYER_START_X), 
      playerY(s_playerPositionSaved ? s_savedPlayerY : TownLayout::PLAYER_START_Y), 
      isStealthMode(true), stealthLevel(1),
      moveTimer(0), playerTexture(nullptr), guardTexture(nullptr),
      shopTexture(nullptr), weaponShopTexture(nullptr), houseTexture(nullptr), castleTexture(nullptr),
      stoneTileTexture(nullptr), residentHomeTexture(nullptr), toriiTexture(nullptr),
      messageBoard(nullptr), nightDisplayLabel(nullptr), nightOperationLabel(nullptr), isShowingMessage(false), isShowingResidentChoice(false), isShowingMercyChoice(false),
      selectedChoice(0), currentTargetX(0), currentTargetY(0), showResidentKilledMessage(false), showReturnToTownMessage(false), shouldReturnToTown(false), castleX(TownLayout::CASTLE_X), castleY(TownLayout::CASTLE_Y),
      guardMoveTimer(0), guardTargetHomeIndices(), guardStayTimers(), guardsInitialized(false),
      allResidentsKilled(false), allGuardsKilled(false), canAttackGuards(false), canEnterCastle(false),
      guardHp() {
    
    for (int i = 0; i < 6; ++i) {
        residentTextures[i] = nullptr;
    }
    
    std::vector<std::pair<int, int>> allResidentPositions = {
        {3, 7},   // 町の住人1
        {7, 6},   // 町の住人2
        {11, 8},  // 町の住人3
        {5, 10},  // 町の住人4
        {19, 8},  // 町の住人5
        {23, 6},  // 町の住人6
        {27, 7},  // 町の住人7
        {6, 13},  // 町の住人8
        {24, 13},  // 町の住人9
        {3, 14},  // 町の住人10
        {19, 14},  // 町の住人11,
        {20, 4}  // 町の住人12
    };
    
    for (const auto& pos : allResidentPositions) {
        bool isKilled = false;
        for (const auto& killedPos : killedResidentPositions) {
            if (pos.first == killedPos.first && pos.second == killedPos.second) {
                isKilled = true;
                break;
            }
        }
        if (!isKilled) {
            residents.push_back(pos);
        }
    }
    
    buildings = {
        {8, 10},   // 道具屋（2x2）
        {18, 10},  // 武器屋（2x2）
        {24, 10},  // 自室（2x2）
        {13, 2}    // 城（画面中央上部、2x2）
    };
    
    buildingTypes = {
        "shop", "weapon_shop", "house", "castle"
    };
    
    residentHomes = {
        {1, 6}, {5, 5}, {9, 7}, {3, 9}, {17, 7}, {21, 5}, {25, 6}, {4, 12}, {22, 12},{1,13},{17,13},{18,3}
    };
    
    guards = {
        {12, 2},  // 衛兵1
        {12, 3},  // 衛兵2
        {15, 2},   // 衛兵3
        {15, 3}   // 衛兵4
    };
    
    if (residentHomes.size() >= 4) {
        guards[0] = {residentHomes[0].first - 1, residentHomes[0].second}; // 1番目の家の左
        guards[1] = {residentHomes[1].first, residentHomes[1].second - 1}; // 2番目の家の上
        guards[2] = {residentHomes[2].first + 1, residentHomes[2].second}; // 3番目の家の右
        guards[3] = {residentHomes[3].first, residentHomes[3].second + 1}; // 4番目の家の下
    }
    
    guardDirections = {
        {1, 1},   // 右下方向
        {-1, 1},  // 左下方向
        {1, -1},  // 右上方向
        {-1, -1}  // 左上方向
    };
    
    guardTargetHomeIndices.resize(4, 0);
    guardStayTimers.resize(4, 0.0f);
    
    guardHp.resize(4, 2);
    
    setupUI();
}

NightState::~NightState() {
    ui.clear();
    
    playerTexture = nullptr;
    guardTexture = nullptr;
    shopTexture = nullptr;
    weaponShopTexture = nullptr;
    houseTexture = nullptr;
    castleTexture = nullptr;
    stoneTileTexture = nullptr;
    residentHomeTexture = nullptr;
    toriiTexture = nullptr;
    
    for (int i = 0; i < 6; ++i) {
        residentTextures[i] = nullptr;
    }
    
    messageBoard = nullptr;
    
    residents.clear();
    guards.clear();
    guardDirections.clear();
    buildings.clear();
    buildingTypes.clear();
    residentHomes.clear();
    guardTargetHomeIndices.clear();
    guardStayTimers.clear();
}

void NightState::enter() {
    try {        
        if (player) {
            player->setNightTime(true);
        }
        
        // 保存された位置がある場合は復元
        if (s_playerPositionSaved) {
            playerX = s_savedPlayerX;
            playerY = s_savedPlayerY;
        }
        
        residents.clear();
    std::vector<std::pair<int, int>> allResidentPositions = {
        {3, 7},   // 町の住人1
        {7, 6},   // 町の住人2
        {11, 8},  // 町の住人3
        {5, 10},  // 町の住人4
        {19, 8},  // 町の住人5
        {23, 6},  // 町の住人6
        {27, 7},  // 町の住人7
        {6, 13},  // 町の住人8
        {24, 13},  // 町の住人9
        {3, 14},  // 町の住人10
        {19, 14},  // 町の住人11,
        {20, 4}  // 町の住人12
    };
    
    const auto& killedResidents = player->getKilledResidents();
    for (const auto& pos : allResidentPositions) {
        bool isKilled = false;
        for (const auto& killedPos : killedResidents) {
            if (pos.first == killedPos.first && pos.second == killedPos.second) {
                isKilled = true;
                break;
            }
        }
        if (!isKilled) {
            residents.push_back(pos);
        }
    }
    
    // 静的変数の現在の状態を保存（住民を倒した処理のチェック用）
    std::vector<std::pair<int, int>> previousKilledPositions = killedResidentPositions;
    int previousTotalKilled = totalResidentsKilled;
    
    // 新しい夜に入る時は、1夜に倒した人数をリセット
    // （前回の夜から戻ってきた場合はリセットしない）
    static int lastNight = -1;
    int currentNight = player->getCurrentNight();
    if (lastNight != currentNight) {
        residentsKilled = 0;
        lastNight = currentNight;
    }
        
        player->autoSave();
        
        setupUI();
        if (nightDisplayLabel) {
            nightDisplayLabel->setText("第" + std::to_string(currentNight) + "夜");
        }
        
        // 戦闘から戻ってきた場合、最新の倒した住民を処理
        bool residentKilledInBattle = false;
        const auto& playerKilledResidents = player->getKilledResidents();
        if (!playerKilledResidents.empty()) {
            // 最新の倒した住民の位置を取得
            const auto& lastKilled = playerKilledResidents.back();
            int x = lastKilled.first;
            int y = lastKilled.second;
            
            // まだ処理していない住民か確認（previousKilledPositionsに含まれていない場合）
            bool alreadyProcessed = false;
            for (const auto& pos : previousKilledPositions) {
                if (pos.first == x && pos.second == y) {
                    alreadyProcessed = true;
                    break;
                }
            }
            
            // まだ処理していない場合、住民を倒した処理を実行
            if (!alreadyProcessed) {
                handleResidentKilled(x, y);
                residentKilledInBattle = true;
            }
        }
        
        // 静的変数を更新（住民を倒した処理の後）
        killedResidentPositions = playerKilledResidents;
        totalResidentsKilled = playerKilledResidents.size();
        
        // メッセージ表示（住民を倒した処理でメッセージが表示されていない場合のみ）
        if (!residentKilledInBattle && !isShowingMessage) {
            showMessage("夜の街に潜入しました。衛兵が近くにいない時に住民を襲撃して街を壊滅させましょう。");
        }
    } catch (const std::exception& e) {
    }
}

void NightState::exit() {
    player->setNightTime(false);
    
    TownState::s_nightTimerActive = true;
    TownState::saved = false;
}

void NightState::update(float deltaTime) {
    try {
        moveTimer -= deltaTime;
        ui.update(deltaTime);
        
        updateGuards(deltaTime);
        
        // ゲーム進行チェック
        checkGameProgress();
    } catch (const std::exception& e) {
    }
}

void NightState::render(Graphics& graphics) {
    try {
        if (!playerTexture) {
            loadTextures(graphics);
        }
        
        drawMap(graphics);
        drawBuildings(graphics);
        
        for (size_t i = 0; i < residents.size(); ++i) {
            const auto& resident = residents[i];
            
            int textureIndex = getResidentTextureIndex(resident.first, resident.second);
            SDL_Texture* texture = residentTextures[textureIndex];
            
            if (texture) {
                graphics.drawTexture(texture, resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            } else {
                graphics.setDrawColor(255, 0, 0, 255);
                graphics.drawRect(resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            }
        }
        
        for (auto& guard : guards) {
            if (guardTexture) {
                graphics.drawTexture(guardTexture, guard.first * TILE_SIZE, guard.second * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            } else {
                graphics.setDrawColor(0, 0, 255, 255);
                graphics.drawRect(guard.first * TILE_SIZE, guard.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            }
        }
        
        drawPlayer(graphics);
        
        CommonUI::drawTrustLevels(graphics, player, true, false);
        
        // UI更新
        updateUI();
        
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto nightConfig = config.getNightConfig();
        
        if (nightDisplayLabel && !nightDisplayLabel->getText().empty()) {
            int bgX, bgY;
            config.calculatePosition(bgX, bgY, nightConfig.nightDisplayBackground.position, graphics.getScreenWidth(), graphics.getScreenHeight());
            graphics.setDrawColor(0, 0, 0, 255); // 黒色
            graphics.drawRect(bgX, bgY, nightConfig.nightDisplayBackground.width, nightConfig.nightDisplayBackground.height, true);
            graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
            graphics.drawRect(bgX, bgY, nightConfig.nightDisplayBackground.width, nightConfig.nightDisplayBackground.height);
        }

        if (nightOperationLabel && !nightOperationLabel->getText().empty()) {
            int bgX, bgY;
            config.calculatePosition(bgX, bgY, nightConfig.nightOperationBackground.position, graphics.getScreenWidth(), graphics.getScreenHeight());
            graphics.setDrawColor(0, 0, 0, 255); // 黒色
            graphics.drawRect(bgX, bgY, nightConfig.nightOperationBackground.width, nightConfig.nightOperationBackground.height, true);
            graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
            graphics.drawRect(bgX, bgY, nightConfig.nightOperationBackground.width, nightConfig.nightOperationBackground.height);
        }
        

        if (messageBoard && !messageBoard->getText().empty()) {
            int bgX, bgY;
            config.calculatePosition(bgX, bgY, nightConfig.messageBoard.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
            graphics.setDrawColor(0, 0, 0, 255); // 黒色
            graphics.drawRect(bgX, bgY, nightConfig.messageBoard.background.width, nightConfig.messageBoard.background.height, true);
            graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
            graphics.drawRect(bgX, bgY, nightConfig.messageBoard.background.width, nightConfig.messageBoard.background.height);
        }
        
        
        
        ui.render(graphics);
        
        graphics.present();
    } catch (const std::exception& e) {
    }
}

void NightState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    if (isShowingResidentChoice && !isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            selectedChoice = (selectedChoice - 1 + choiceOptions.size()) % choiceOptions.size();
            updateChoiceDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            selectedChoice = (selectedChoice + 1) % choiceOptions.size();
            updateChoiceDisplay();
        }
        
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            handleResidentChoice();
        }
        return; // 選択肢表示中は他の操作を無効化
    }
    
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
            
            if (isShowingResidentChoice) {
                isShowingMessage = false;
                updateChoiceDisplay();
            }
            else if (showResidentKilledMessage) {
                showResidentKilledMessage = false;
                // 魔王の信頼度が上がったメッセージを表示
                if (totalResidentsKilled <= 6) {
                    showMessage("住民を倒しました。\n勇者: ああぁぁぁぁ、ごめんなさい。倒さないと私が魔王にやられちゃうんだ、、\n\n魔王からの信頼度 +10 メンタル -20");
                } else {
                    showMessage("住民を倒しました。\n勇者: ふふっ、だんだん楽しくなってきたぁ、、！\n\n魔王からの信頼度 +10 メンタル +20");
                }
                // 3人目を倒した場合は、次のメッセージで街に戻る処理を実行するフラグを設定
                if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
                    showReturnToTownMessage = true;
                }
            }
            else if (showReturnToTownMessage) {
                showReturnToTownMessage = false;
                // 街に戻るメッセージを表示
                int currentNight = player->getCurrentNight();
                if (currentNight < 4) {
                    showMessage("住人を3人倒しました。これ以上は危険です。\n街に戻ります。");
                    // 次のSpaceで街に戻る処理を実行するフラグを設定
                    shouldReturnToTown = true;
                } else {
                    // 4夜目以降の場合はメッセージを表示（衛兵を攻撃可能にする）
                    showMessage("住民を全て倒しました。次は衛兵を倒しましょう。\n衛兵は2回攻撃しないと倒すことができません。");
                }
            }
            else if (shouldReturnToTown) {
                // 街に戻るメッセージを表示した後、Spaceを押した時に街に戻る処理を実行
                shouldReturnToTown = false;
                int currentNight = player->getCurrentNight();
                if (stateManager && currentNight < 4) {
                    // 1夜に倒した人数をリセット（次の夜のために）
                    residentsKilled = 0;
                    TownState::s_nightTimerActive = true;
                    if (currentNight == 1) {
                        TownState::s_nightTimer = 240.0f;
                    } else if (currentNight == 2) {
                        TownState::s_nightTimer = 180.0f;
                    } else {
                        TownState::s_nightTimer = 180.0f;
                    }
                    stateManager->changeState(std::make_unique<TownState>(player));
                }
            }
            else if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
                // 3人目を倒したが、メッセージ表示フローを経由していない場合のフォールバック
                // （通常はshowReturnToTownMessageフラグで処理されるが、念のため）
                if (stateManager && player->getCurrentNight() < 4) {
                    // 1夜に倒した人数をリセット（次の夜のために）
                    residentsKilled = 0;
                    TownState::s_nightTimerActive = true;
                    if (player->getCurrentNight() == 1) {
                        TownState::s_nightTimer = 240.0f;
                    } else if (player->getCurrentNight() == 2) {
                        TownState::s_nightTimer = 180.0f;
                    } else {
                        TownState::s_nightTimer = 180.0f;
                    }
                    stateManager->changeState(std::make_unique<TownState>(player));
                }
            }
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            TownState::s_nightTimerActive = true;
            TownState::s_nightTimer = 300.0f; // 5分 = 300秒
            stateManager->changeState(std::make_unique<TownState>(player));
        }
        return;
    }
    
    if (input.isKeyJustPressed(InputKey::SPACE) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
        if (canEnterCastle) {
            checkCastleEntrance();
            return;
        }
        
        if (canAttackGuards) {
            checkGuardInteraction();
            return;
        }
        
        checkResidentInteraction();
        return;
    }
    
    handleMovement(input);
}

void NightState::setupUI() {
    try {
        ui.clear();
        
        // メッセージボード（画面中央下部）- TownStateと同じ仕様
        auto messageBoardLabel = std::make_unique<Label>(210, 500, "", "default");
        messageBoardLabel->setColor({255, 255, 255, 255}); // 白文字
        messageBoardLabel->setText("");
        messageBoard = messageBoardLabel.get(); // ポインタを保存
        ui.addElement(std::move(messageBoardLabel));
        
        auto nightDisplayLabel = std::make_unique<Label>(20, 15, "", "default");
        nightDisplayLabel->setColor({255, 255, 255, 255}); // 白文字
        nightDisplayLabel->setText("");
        this->nightDisplayLabel = nightDisplayLabel.get(); // ポインタを保存
        ui.addElement(std::move(nightDisplayLabel));

        auto nightOperationLabel = std::make_unique<Label>(95, 15, "", "default");
        nightOperationLabel->setColor({255, 255, 255, 255}); // 白文字
        nightOperationLabel->setText("住人を倒す: スペースキー");
        this->nightOperationLabel = nightOperationLabel.get(); // ポインタを保存
        ui.addElement(std::move(nightOperationLabel));
    } catch (const std::exception& e) {
    }
}

void NightState::handleMovement(const InputManager& input) {
    GameState::handleMovement(input, playerX, playerY, moveTimer, MOVE_DELAY,
        [this](int x, int y) { return isValidPosition(x, y); },
        [this](int x, int y) { /* 移動後の処理（必要に応じて） */ });
}

void NightState::checkResidentInteraction() {
    for (auto& resident : residents) {
        if (GameUtils::isNearPositionEuclidean(playerX, playerY, resident.first, resident.second, 1.5f)) {
            attackResident(resident.first, resident.second);
            return;
        }
    }
}

void NightState::attackResident(int x, int y) {
    
    if (isShowingResidentChoice) {
        return;
    }
    
    // 1夜に倒せる住民の最大値は3人まで
    if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
        int currentNight = player->getCurrentNight();
        if (currentNight < 4) {
            showMessage("住人を3人倒しました。これ以上は危険です。\n街に戻ります。");
        } else {
            showMessage("住民を全て倒しました。次は衛兵を倒しましょう。\n衛兵は2回攻撃しないと倒すことができません。");
        }
        return;
    }
    
    if (!canAttackGuards && isNearGuard(x, y)) {
        player->setKingTrust(0); // 王様からの信頼度を0に設定
        
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "衛兵に見つかりました。王様からの信頼度が0になりました。"));
        }
        return;
    }
    
    // 戦闘に移行する前にプレイヤーの位置を保存
    s_savedPlayerX = playerX;
    s_savedPlayerY = playerY;
    s_playerPositionSaved = true;
    
    currentTargetX = x;
    currentTargetY = y;
    
    choiceOptions.clear();
    choiceOptions.push_back("倒す");
    choiceOptions.push_back("倒さない");
    
    showMessage("住民：「お願いします！助けてください！\n私には家族がいるんです、、！」");
    isShowingResidentChoice = true;
    selectedChoice = 0;
}

void NightState::hideEvidence() {
    player->performEvilAction();
    showMessage("証拠を隠滅しました。");
}

void NightState::handleResidentChoice() {
    if (isShowingResidentChoice && selectedChoice >= 0 && selectedChoice < static_cast<int>(choiceOptions.size())) {
        executeResidentChoice(selectedChoice);
    }
}

void NightState::updateChoiceDisplay() {
    std::string message = "選択してください：\n";
    if (selectedChoice == 0) {
        message += "> 1. 倒す\n";
        message += "  2. 倒さない";
    } else {
        message += "  1. 倒す\n";
        message += "> 2. 倒さない";
    }
    
    if (messageBoard) {
        messageBoard->setText(message);
    }
}

void NightState::executeResidentChoice(int choice) {
    if (choice == 0) {
        // 「倒す」を選択した場合、戦闘画面に遷移
        isShowingResidentChoice = false;
        
        // 住民を敵として戦闘を開始
        // 住民は弱い敵として扱う（SLIMEタイプ、レベル1）
        Enemy residentEnemy(EnemyType::SLIME);
        
        // 住民の名前を取得して設定
        std::string residentName = getResidentName(currentTargetX, currentTargetY);
        residentEnemy.setName(residentName);
        
        // 住民の画像インデックスを取得して設定
        int textureIndex = getResidentTextureIndex(currentTargetX, currentTargetY);
        residentEnemy.setResidentTextureIndex(textureIndex);
        
        // 住民の位置情報を設定
        residentEnemy.setResidentPosition(currentTargetX, currentTargetY);
        
        if (stateManager) {
            stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(residentEnemy)));
        }
        return;
    } else if (choice == 1) {
        // 「倒さない」を選択した場合の処理（既存の処理）
        isShowingResidentChoice = false;
        
        player->changeDemonTrust(-10);
        player->changeMental(20);
        
        showMessage("住民：「ありがとうございます！本当にありがとうございます！」\n魔王からの信頼度 -10 メンタル +20");
        return;
    }
    
    // 以下は既存の処理（確率判定による住民撃破）は一旦コメントアウト
    /*
    if (choice == 0) {
        isShowingResidentChoice = false;
        
        int mental = player->getMental();
        int successRate = 80; // 基本成功率80%
        
        // メンタルが低いほど成功率が下がる
        if (mental < 30) {
            successRate = 40;
        } else if (mental < 50) {
            successRate = 60;
        } else if (mental > 80) {
            successRate = 90; // メンタルが高いと成功率が上がる
        }
        
        // ランダム判定
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);
        
        if (dis(gen) <= successRate) {
            residentsKilled++;
            
            player->changeDemonTrust(10); // 魔王からの信頼度を10上昇
            
            totalResidentsKilled++; // 合計倒した人数を増加
            if (totalResidentsKilled >= 6) { // 合計6人以上倒した場合
                player->changeMental(20); // メンタルを20上昇
            } else {
                player->changeMental(-20); // メンタルを20減少
            }
            
            killedResidentPositions.push_back({currentTargetX, currentTargetY});
            player->addKilledResident(currentTargetX, currentTargetY);
            
            residents.erase(std::remove_if(residents.begin(), residents.end(),
                [this](const std::pair<int, int>& pos) {
                    return pos.first == currentTargetX && pos.second == currentTargetY;
                }), residents.end());
            
            showMessage("住民：「みんなお前を信じてたんだぞ。\nほんっと、最低な勇者だな。」");
            
            isShowingResidentChoice = false; // 選択肢表示を終了
            showResidentKilledMessage = true; // 倒したメッセージ表示フラグを設定

            player->changeKingTrust(-10);
            
            if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
                int currentNight = player->getCurrentNight();
                if (currentNight < 4) {
                    showMessage("住人を3人倒しました。これ以上は危険です。\n街に戻ります。");
                } else {
                    showMessage("住民を全て倒しました。次は衛兵を倒しましょう。\n衛兵は2回攻撃しないと倒すことができません。");
                    canAttackGuards = true; // 衛兵を攻撃可能にする
                }
            }
        } else {
            player->changeDemonTrust(-10); // 魔王からの信頼度を10減少
            showMessage("住人を倒すのをためらいました。\n勇者: ああぁぁぁぁぁ、私には出来ないよ、、\n\n魔王からの信頼度 -10");
        }
    */
}

void NightState::showMessage(const std::string& message) {
    GameState::showMessage(message, messageBoard, isShowingMessage);
}

void NightState::clearMessage() {
    GameState::clearMessage(messageBoard, isShowingMessage);
}

void NightState::drawNightTown(Graphics& graphics) {
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 30; x++) {
            int drawX = x * TILE_SIZE;
            int drawY = y * TILE_SIZE;
            
            graphics.setDrawColor(30, 30, 50, 255);
            graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
        }
    }
    
    graphics.setDrawColor(255, 100, 100, 255);
    for (auto& resident : residents) {
        int drawX = resident.first * TILE_SIZE + 8;
        int drawY = resident.second * TILE_SIZE + 8;
        graphics.drawRect(drawX, drawY, 16, 16, true);
    }
    
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
        graphics.setDrawColor(0, 255, 0, 255);
        graphics.drawRect(playerX * TILE_SIZE, playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
    }
}

bool NightState::isValidPosition(int x, int y) const {
    // 基本的な境界チェック
    if (x < 0 || x >= 28 || y < 0 || y >= 16) {
        return false;
    }
    
    if (isCollidingWithBuilding(x, y)) {
        return false;
    }
    
    if (isCollidingWithResident(x, y)) {
        return false;
    }
    
    if (isCollidingWithGuard(x, y)) {
        return false;
    }
    
    return true;
}

bool NightState::isCollidingWithBuilding(int x, int y) const {
    for (const auto& building : buildings) {
        int buildingX = building.first;
        int buildingY = building.second;
        
        if (GameUtils::isColliding(x, y, 1, 1, buildingX, buildingY, 2, 2)) {
            return true;
        }
    }
    
    for (const auto& home : residentHomes) {
        int homeX = home.first;
        int homeY = home.second;
        
        if (GameUtils::isColliding(x, y, 1, 1, homeX, homeY, 2, 2)) {
            return true;
        }
    }
    
    return false;
}

bool NightState::isCollidingWithResident(int x, int y) const {
    for (const auto& resident : residents) {
        if (x == resident.first && y == resident.second) {
            return true;
        }
    }
    return false;
}

bool NightState::isCollidingWithGuard(int x, int y) const {
    for (const auto& guard : guards) {
        if (x == guard.first && y == guard.second) {
            return true;
        }
    }
    return false;
}

int NightState::getResidentTextureIndex(int x, int y) const {
    std::vector<std::pair<int, int>> allPositions = {
        {3, 7},   // 町の住人1 -> resident_1
        {7, 6},   // 町の住人2 -> resident_2
        {11, 8},  // 町の住人3 -> resident_3
        {5, 10},  // 町の住人4 -> resident_4
        {19, 8},  // 町の住人5 -> resident_5
        {23, 6},  // 町の住人6 -> resident_6
        {27, 7},  // 町の住人7 -> resident_1（循環）
        {6, 13},  // 町の住人8 -> resident_2
        {24, 13}, // 町の住人9 -> resident_3
        {3, 14},  // 町の住人10 -> resident_4
        {19, 14}, // 町の住人11 -> resident_5
        {20, 4}   // 町の住人12 -> resident_6
    };
    
    for (size_t i = 0; i < allPositions.size(); ++i) {
        if (allPositions[i].first == x && allPositions[i].second == y) {
            return i % 6; // 6つの画像を循環使用
        }
    }
    
    return 0; // デフォルト
}

std::string NightState::getResidentName(int x, int y) const {
    std::vector<std::pair<int, int>> allPositions = {
        {3, 7},   // 町の住人1
        {7, 6},   // 町の住人2
        {11, 8},  // 町の住人3
        {5, 10},  // 町の住人4
        {19, 8},  // 町の住人5
        {23, 6},  // 町の住人6
        {27, 7},  // 町の住人7
        {6, 13},  // 町の住人8
        {24, 13}, // 町の住人9
        {3, 14},  // 町の住人10
        {19, 14}, // 町の住人11
        {20, 4}   // 町の住人12
    };
    
    std::vector<std::string> residentNames = {
        "町の住人1", "町の住人2", "町の住人3", "町の住人4", "町の住人5", "町の住人6",
        "町の住人7", "町の住人8", "町の住人9", "町の住人10", "町の住人11", "町の住人12"
    };
    
    for (size_t i = 0; i < allPositions.size() && i < residentNames.size(); ++i) {
        if (allPositions[i].first == x && allPositions[i].second == y) {
            return residentNames[i];
        }
    }
    
    return "住民"; // デフォルト
}

void NightState::handleResidentKilled(int x, int y) {
    residentsKilled++;
    
    player->changeDemonTrust(10); // 魔王からの信頼度を10上昇
    
    totalResidentsKilled++; // 合計倒した人数を増加
    if (totalResidentsKilled >= 6) { // 合計6人以上倒した場合
        player->changeMental(20); // メンタルを20上昇
    } else {
        player->changeMental(-20); // メンタルを20減少
    }
    
    killedResidentPositions.push_back({x, y});
    player->addKilledResident(x, y);
    
    residents.erase(std::remove_if(residents.begin(), residents.end(),
        [x, y](const std::pair<int, int>& pos) {
            return pos.first == x && pos.second == y;
        }), residents.end());
    
    // 最初のメッセージを表示
    showMessage("住民：「みんなお前を信じてたんだぞ。\nほんっと、最低な勇者だな。」");
    
    // 追加のメッセージ表示フラグを設定（handleInput()で処理される）
    showResidentKilledMessage = true;
    
    player->changeKingTrust(-10);
    
    // 3人目を倒した場合は、メッセージ表示フローで処理されるため、ここでは何もしない
    // （handleInput()でshowReturnToTownMessageフラグが設定され、街に戻る処理が実行される）
    
    // メンタルや信頼度の変動をセーブ
    player->autoSave();
    
    checkGameProgress();
} 

void NightState::loadTextures(Graphics& graphics) {
    try {
        if (!playerTexture) {
            playerTexture = graphics.loadTexture("assets/textures/characters/player.png", "player");
        }
        
        for (int i = 0; i < 6; ++i) {
            if (!residentTextures[i]) {
                std::string textureName = "resident_" + std::to_string(i + 1);
                residentTextures[i] = graphics.getTexture(textureName);
            }
        }
        
        if (!guardTexture) {
            guardTexture = graphics.loadTexture("assets/textures/characters/guard.png", "guard");
        }
        
        shopTexture = graphics.getTexture("shop");
        weaponShopTexture = graphics.getTexture("weapon_shop");
        houseTexture = graphics.getTexture("house");
        castleTexture = graphics.getTexture("castle");
        stoneTileTexture = graphics.loadTexture("assets/textures/tiles/nightstonetile.png", "night_stone_tile");
        residentHomeTexture = graphics.getTexture("resident_home");
        toriiTexture = graphics.getTexture("torii");
    } catch (const std::exception& e) {
    }
}

void NightState::updateGuards(float deltaTime) {
    try {
        if (!guardsInitialized) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, residentHomes.size() - 1);
            
            for (size_t i = 0; i < guards.size(); ++i) {
                guardTargetHomeIndices[i] = dis(gen);
            }
            guardsInitialized = true;
        }
        
        guardMoveTimer += deltaTime;
        
        if (guardMoveTimer >= 2.0f) {
            guardMoveTimer = 0;
            
            for (size_t i = 0; i < guards.size(); ++i) {
                int currentX = guards[i].first;
                int currentY = guards[i].second;
                
                int targetHomeIndex = guardTargetHomeIndices[i];
                if (targetHomeIndex >= 0 && targetHomeIndex < static_cast<int>(residentHomes.size())) {
                    int targetX = residentHomes[targetHomeIndex].first;
                    int targetY = residentHomes[targetHomeIndex].second;
                    
                    int distanceToHome = std::max(abs(currentX - targetX), abs(currentY - targetY));
                    
                    if (distanceToHome <= 1) {
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<> dis(0, residentHomes.size() - 1);
                        
                        int newTargetIndex;
                        do {
                            newTargetIndex = dis(gen);
                        } while (newTargetIndex == targetHomeIndex);
                        guardTargetHomeIndices[i] = newTargetIndex;
                    } else {
                        guards[i].first = targetX;
                        guards[i].second = targetY;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
    }
}

bool NightState::isNearGuard(int x, int y) const {
    for (const auto& guard : guards) {
        int dx = abs(x - guard.first);
        int dy = abs(y - guard.second);
        
        if (dx <= 2 && dy <= 2) {
            return true;
        }
    }
    return false;
}

void NightState::updateUI() {
    checkTrustLevels();
}

void NightState::checkTrustLevels() {
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

void NightState::drawMap(Graphics& graphics) {
    if (stoneTileTexture) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 28; x++) {
                int drawX = x * TILE_SIZE;
                int drawY = y * TILE_SIZE;
                
                graphics.drawTexture(stoneTileTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
            }
        }
    } else {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 28; x++) {
                int drawX = x * TILE_SIZE;
                int drawY = y * TILE_SIZE;
                
                // 石畳タイル（グレー）
                graphics.setDrawColor(160, 160, 160, 255);
                graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, true);
                
                graphics.setDrawColor(100, 100, 100, 255);
                graphics.drawRect(drawX, drawY, TILE_SIZE, TILE_SIZE, false);
            }
        }
    }
    
    drawGate(graphics);
}

void NightState::drawBuildings(Graphics& graphics) {
    for (size_t i = 0; i < buildings.size(); ++i) {
        int x = buildings[i].first;
        int y = buildings[i].second;
        std::string type = buildingTypes[i];
        
        if (type == "shop" && shopTexture) {
            graphics.drawTexture(shopTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else if (type == "weapon_shop" && weaponShopTexture) {
            graphics.drawTexture(weaponShopTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else if (type == "house" && houseTexture) {
            graphics.drawTexture(houseTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else if (type == "castle" && castleTexture) {
            graphics.drawTexture(castleTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else {
            Uint8 r, g, b;
            if (type == "shop") {
                r = 139; g = 69; b = 19; // 茶色
            } else if (type == "weapon_shop") {
                r = 192; g = 192; b = 192; // 銀色
            } else if (type == "house") {
                r = 34; g = 139; b = 34; // 緑色
            } else if (type == "castle") {
                r = 255; g = 215; b = 0; // 金色
            } else {
                r = 128; g = 128; b = 128; // グレー
            }
            
            graphics.setDrawColor(r, g, b, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2);
        }
    }
    
    for (const auto& home : residentHomes) {
        int x = home.first;
        int y = home.second;
        
        if (residentHomeTexture) {
            graphics.drawTexture(residentHomeTexture, x * TILE_SIZE, y * TILE_SIZE, 
                               TILE_SIZE * 2, TILE_SIZE * 2);
        } else {
            graphics.setDrawColor(139, 69, 19, 255); // 茶色
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2, true);
            graphics.setDrawColor(0, 0, 0, 255);
            graphics.drawRect(x * TILE_SIZE, y * TILE_SIZE, 
                            TILE_SIZE * 2, TILE_SIZE * 2);
        }
    }
}

void NightState::drawGate(Graphics& graphics) {
    int gateX = TownLayout::GATE_X;
    int gateY = TownLayout::GATE_Y;
    
    if (stoneTileTexture) {
        int drawX = gateX * TILE_SIZE;
        int drawY = gateY * TILE_SIZE;
        graphics.drawTexture(stoneTileTexture, drawX, drawY, TILE_SIZE, TILE_SIZE);
    }
    
    if (toriiTexture) {
        int drawX = gateX * TILE_SIZE;
        int drawY = gateY * TILE_SIZE;
        // 鳥居は少し大きく描画（2タイルサイズ）
        graphics.drawTexture(toriiTexture, drawX - TILE_SIZE/2, drawY - TILE_SIZE, 
                           TILE_SIZE * 2, TILE_SIZE * 2);
    } else {
        int drawX = gateX * TILE_SIZE + TILE_SIZE/4;
        int drawY = gateY * TILE_SIZE - TILE_SIZE/2;
        graphics.setDrawColor(255, 0, 0, 255); // 赤色
        graphics.drawRect(drawX, drawY, TILE_SIZE * 1.5, TILE_SIZE * 1.5, true);
        graphics.setDrawColor(0, 0, 0, 255);
        graphics.drawRect(drawX, drawY, TILE_SIZE * 1.5, TILE_SIZE * 1.5, false);
    }
}

void NightState::checkGameProgress() {
    if (!allResidentsKilled && residents.empty()) {
        allResidentsKilled = true;
        canAttackGuards = true;
        showMessage("住人を全員倒せたようですね。残るは衛兵と王様だけです。\nまずは外にいる衛兵を全員倒してしまいましょう。");
    }
    
    if (allResidentsKilled && !allGuardsKilled && guards.empty()) {
        allGuardsKilled = true;
        canEnterCastle = true;
        showMessage("衛兵も全て倒せたようですね。それでは最後は城に入って王様を倒しましょう。");
    }
}

void NightState::checkGuardInteraction() {
    for (auto& guard : guards) {
        if (GameUtils::isNearPositionEuclidean(playerX, playerY, guard.first, guard.second, 1.5f)) {
            attackGuard(guard.first, guard.second);
            return;
        }
    }
}

void NightState::attackGuard(int x, int y) {
    for (size_t i = 0; i < guards.size(); ++i) {
        if (guards[i].first == x && guards[i].second == y) {
            guardHp[i]--;
            
            if (guardHp[i] <= 0) {
                guards.erase(guards.begin() + i);
                guardHp.erase(guardHp.begin() + i);
            }
            break;
        }
    }
}

void NightState::checkCastleEntrance() {
    int distanceToCastle = std::max(abs(playerX - castleX), abs(playerY - castleY));
    if (distanceToCastle <= 2 && canEnterCastle) {
        enterCastle();
    }
}

void NightState::enterCastle() {
    if (stateManager) {
        stateManager->changeState(std::make_unique<CastleState>(player, true));
    }
} 
