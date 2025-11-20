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
#include "../core/AudioManager.h"
#include "../utils/TownLayout.h"
#include <nlohmann/json.hpp>
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
      selectedChoice(0), currentTargetX(0), currentTargetY(0), showResidentKilledMessage(false), showReturnToTownMessage(false), shouldReturnToTown(false), showGuardMessage(false), showGuardKilledMessage(false), showAllGuardsKilledMessage(false), currentGuardX(0), currentGuardY(0), castleX(TownLayout::CASTLE_X), castleY(TownLayout::CASTLE_Y),
      showGameExplanation(false), explanationStep(0), explanationMessageBoard(nullptr),
      guardMoveTimer(0), guardTargetHomeIndices(), guardStayTimers(), guardsInitialized(false),
      allResidentsKilled(false), allGuardsKilled(false), canAttackGuards(false), canEnterCastle(false),
      guardHp() {
    
    for (int i = 0; i < 6; ++i) {
        residentTextures[i] = nullptr;
    }
    
    for (const auto& pos : TownLayout::RESIDENTS) {
        if (!TownLayout::isResidentKilled(pos.first, pos.second, killedResidentPositions)) {
            residents.push_back(pos);
        }
    }
    
    buildings = TownLayout::BUILDINGS;
    buildingTypes = TownLayout::BUILDING_TYPES;
    residentHomes = TownLayout::RESIDENT_HOMES;
    guards = TownLayout::GUARDS;
    
    // 衛兵の初期位置を住人の家の周辺に設定（デバッグ用、通常はTownLayout::GUARDSを使用）
    if (residentHomes.size() >= 4) {
        guards[0] = {residentHomes[0].first - 1, residentHomes[0].second};
        guards[1] = {residentHomes[1].first, residentHomes[1].second - 1};
        guards[2] = {residentHomes[2].first + 1, residentHomes[2].second};
        guards[3] = {residentHomes[3].first, residentHomes[3].second + 1};
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
    // 夜の街に入った時は night.ogg を再生
    AudioManager::getInstance().stopMusic();
    AudioManager::getInstance().playMusic("night", -1);
    
    try {
        // 保存された状態を復元（BattleStateから戻ってきた場合など）
        bool restoredFromJson = false;
        const nlohmann::json* savedState = player->getSavedGameState();
        if (savedState && !savedState->is_null() && 
            savedState->contains("stateType") && 
            static_cast<StateType>((*savedState)["stateType"]) == StateType::NIGHT) {
            fromJson(*savedState);
            restoredFromJson = true;
        }
        
        if (player) {
            player->setNightTime(true);
        }
        
        // 夜の街に入る時はタイマーを停止
        TownState::s_nightTimerActive = false;
        
        // 保存された位置がある場合は復元
        if (s_playerPositionSaved) {
            playerX = s_savedPlayerX;
            playerY = s_savedPlayerY;
        }
        
        residents.clear();
    // 昼の街と同じ住民位置を使用（TownLayout::RESIDENTS）
    const auto& killedResidents = player->getKilledResidents();
    for (const auto& pos : TownLayout::RESIDENTS) {
        if (!TownLayout::isResidentKilled(pos.first, pos.second, killedResidents)) {
            residents.push_back(pos);
        }
    }
    
    // 静的変数の現在の状態を保存（住民を倒した処理のチェック用）
    std::vector<std::pair<int, int>> previousKilledPositions = killedResidentPositions;
    int previousTotalKilled = totalResidentsKilled;
        
    // 新しい夜に入る時は、1夜に倒した人数をリセット
    // ただし、fromJson()で復元した場合は、復元された値を保持するためスキップ
    static int lastNight = -1;
    int currentNight = player->getCurrentNight();
    bool isNewNight = (lastNight != currentNight);
    
    if (isNewNight && !restoredFromJson) {
        // 新しい夜の開始時は、residentsKilledを必ず0にリセット
        // （fromJson()で復元した場合は、復元された値を保持するためスキップ）
        residentsKilled = 0;
        // メッセージ表示フラグもリセット
        showResidentKilledMessage = false;
        showReturnToTownMessage = false;
        shouldReturnToTown = false;
        lastNight = currentNight;
    } else if (restoredFromJson) {
        // fromJson()で復元した場合は、lastNightを現在の夜に設定して、
        // 次回のenter()で正しく判定できるようにする
        lastNight = currentNight;
    }
        
        // 現在のStateの状態を保存
        saveCurrentState(player);
        
        setupUI();
        if (nightDisplayLabel) {
            nightDisplayLabel->setText("第" + std::to_string(currentNight) + "夜");
        }
        
        // 戦闘から戻ってきた場合のみ、最新の倒した住民を処理
        // （新しい夜の開始時には実行しない）
        bool residentKilledInBattle = false;
        if (!isNewNight) {
            const auto& playerKilledResidents = player->getKilledResidents();
            if (!playerKilledResidents.empty()) {
                // 最新の倒した住民の位置を取得
                const auto& lastKilled = playerKilledResidents.back();
                int x = lastKilled.first;
                int y = lastKilled.second;
                
                // まだ処理していない住民か確認（previousKilledPositionsに含まれていない場合）
                bool alreadyProcessed = TownLayout::isResidentKilled(x, y, previousKilledPositions);
                
                // まだ処理していない場合、住民を倒した処理を実行
                if (!alreadyProcessed) {
                    handleResidentKilled(x, y);
                    residentKilledInBattle = true;
                }
            }
        }
        
        // 静的変数を更新（新しい夜の開始時は、player->getKilledResidents()で更新）
        // 戦闘から戻ってきた場合は、handleResidentKilledで既に更新されている
        const auto& playerKilledResidents = player->getKilledResidents();
        if (isNewNight) {
            // 新しい夜の開始時は、killedResidentPositionsとtotalResidentsKilledを更新
            killedResidentPositions = playerKilledResidents;
            TownLayout::removeDuplicatePositions(killedResidentPositions);
            totalResidentsKilled = killedResidentPositions.size();
        } else if (residentKilledInBattle) {
            // 戦闘から戻ってきた場合は、handleResidentKilledで既に更新されているので、
            // 念のため同期を取る（重複を除去）
            killedResidentPositions = playerKilledResidents;
            TownLayout::removeDuplicatePositions(killedResidentPositions);
            totalResidentsKilled = killedResidentPositions.size();
        }
        
        // 初回の夜の街説明を表示するかチェック
        // 説明UIを既に見た場合は表示しない
        if (!player->hasSeenNightExplanation) {
            if (!showGameExplanation || gameExplanationTexts.empty()) {
                setupGameExplanation();
            }
            showGameExplanation = true;
            explanationStep = 0;
            // explanationMessageBoardはsetupUI()で初期化されるので、render()で設定する
        }
        
        // 衛兵戦から戻ってきた場合の処理
        bool guardKilledInBattle = false;
        if (!isNewNight && currentGuardX != 0 && currentGuardY != 0) {
            // 衛兵を倒した処理を実行
            guardKilledInBattle = true;
            
            // 衛兵を削除
            for (size_t i = 0; i < guards.size(); ++i) {
                if (guards[i].first == currentGuardX && guards[i].second == currentGuardY) {
                    guards.erase(guards.begin() + i);
                    guardHp.erase(guardHp.begin() + i);
                    break;
                }
            }
            
            // 位置をリセット
            currentGuardX = 0;
            currentGuardY = 0;
        }
        
        // 住民を全て倒した状態を検出（デバッグモードなど）
        // ただし、衛兵を倒した直後はcheckGameProgress()を呼ばない（メッセージが重複しないように）
        if (!guardKilledInBattle) {
            checkGameProgress();
        } else {
            // 衛兵を倒した直後でも、ゲーム進行状態は更新する（メッセージは表示しない）
            // 住民を全て倒したかどうかを確認（メッセージは表示しない）
            const auto& killedResidents = player->getKilledResidents();
            bool allKilled = TownLayout::areAllResidentsKilled(killedResidents);
            
            if (!allResidentsKilled && allKilled) {
                allResidentsKilled = true;
                canAttackGuards = true;
            }
            
            // 全ての衛兵を倒したかどうかを確認（メッセージを表示する前にチェック）
            if (allResidentsKilled && !allGuardsKilled && guards.empty()) {
                allGuardsKilled = true;
                canEnterCastle = true;
                // 全ての衛兵を倒したメッセージを表示（「衛兵を倒しました。」のメッセージの後に表示される）
                // フラグを設定して、handleInput()で処理する
                showAllGuardsKilledMessage = true; // 次のEnterで全衛兵倒したメッセージを表示
            }
            
            // 衛兵を倒したメッセージを表示（全ての衛兵を倒したかどうかのチェックの後）
            showMessage("衛兵を倒しました。");
            showGuardKilledMessage = true;
        }
        
        // メッセージ表示（住民を倒した処理でメッセージが表示されていない場合、かつ説明UIが表示されていない場合のみ）
        if (!residentKilledInBattle && !guardKilledInBattle && !isShowingMessage && !showGameExplanation) {
            // 住民を全て倒した場合は、別のメッセージを表示しない（checkGameProgress()で既に表示されている）
            if (!allResidentsKilled) {
                showMessage("夜の街に潜入しました。衛兵が近くにいない時に住民を襲撃して街を壊滅させましょう。");
            }
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
        
        // ホットリロード対応
        static bool lastReloadState = false;
        auto& config = UIConfig::UIConfigManager::getInstance();
        bool currentReloadState = config.checkAndReloadConfig();
        
        if (!lastReloadState && currentReloadState) {
            setupUI();
        }
        lastReloadState = currentReloadState;
        
        updateGuards(deltaTime);
        
        // ゲーム進行チェック
        checkGameProgress();
    } catch (const std::exception& e) {
    }
}

void NightState::render(Graphics& graphics) {
    try {
        bool uiJustInitialized = false;
        if (!explanationMessageBoard) {
            // setupUI()が呼ばれていない場合は、ここで呼ぶ
            setupUI();
            uiJustInitialized = true;
        }
        
        if (!playerTexture) {
            loadTextures(graphics);
        }
        
        // 画面をクリア（前の画面の描画を消すため）
        graphics.setDrawColor(30, 30, 50, 255); // 夜の街の背景色
        graphics.clear();
        
        drawMap(graphics);
        drawBuildings(graphics);
        
        for (size_t i = 0; i < residents.size(); ++i) {
            const auto& resident = residents[i];
            
            int textureIndex = TownLayout::getResidentTextureIndex(resident.first, resident.second);
            SDL_Texture* texture = residentTextures[textureIndex];
            
            if (texture) {
                // アスペクト比を保持して縦幅に合わせて描画
                int centerX = resident.first * TILE_SIZE + TILE_SIZE / 2;
                int centerY = resident.second * TILE_SIZE + TILE_SIZE / 2;
                graphics.drawTextureAspectRatio(texture, centerX, centerY, TILE_SIZE, true, true);
            } else {
                graphics.setDrawColor(255, 0, 0, 255);
                graphics.drawRect(resident.first * TILE_SIZE, resident.second * TILE_SIZE, TILE_SIZE, TILE_SIZE, true);
            }
        }
        
        for (auto& guard : guards) {
            if (guardTexture) {
                // アスペクト比を保持して縦幅に合わせて描画
                int centerX = guard.first * TILE_SIZE + TILE_SIZE / 2;
                int centerY = guard.second * TILE_SIZE + TILE_SIZE / 2;
                graphics.drawTextureAspectRatio(guardTexture, centerX, centerY, TILE_SIZE, true, true);
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
        
        // 説明UIが設定されている場合は表示
        // uiJustInitializedがtrueの場合（UIが初期化された直後）は確実に1番目のメッセージを表示
        if (uiJustInitialized && showGameExplanation && !gameExplanationTexts.empty() && explanationStep == 0) {
            showExplanationMessage(gameExplanationTexts[0]);
        }
        // uiJustInitializedがfalseの場合でも、showGameExplanationがtrueでメッセージが表示されていない場合は表示する
        else if (showGameExplanation && !gameExplanationTexts.empty() && explanationStep == 0 && explanationMessageBoard && explanationMessageBoard->getText().empty()) {
            showExplanationMessage(gameExplanationTexts[0]);
        }
        
        if (showGameExplanation && explanationMessageBoard && !explanationMessageBoard->getText().empty()) {
            auto mbConfig = config.getMessageBoardConfig();
            
            int bgX, bgY;
            config.calculatePosition(bgX, bgY, mbConfig.background.position, graphics.getScreenWidth(), graphics.getScreenHeight());
            
            graphics.setDrawColor(0, 0, 0, 255); // 黒色
            graphics.drawRect(bgX, bgY, mbConfig.background.width, mbConfig.background.height, true);
            graphics.setDrawColor(255, 255, 255, 255); // 白色でボーダー
            graphics.drawRect(bgX, bgY, mbConfig.background.width, mbConfig.background.height);
        }
        
        // インタラクション可能な時に「ENTER」を表示
        if (!isShowingMessage && !showGameExplanation) {
            bool canInteract = false;
            
            // 住民の近くにいるかチェック
            if (!canAttackGuards) {
                const auto& killedResidents = player->getKilledResidents();
                bool allKilled = TownLayout::areAllResidentsKilled(killedResidents);
                if (!allKilled) {
                    for (auto& resident : residents) {
                        if (GameUtils::isNearPositionEuclidean(playerX, playerY, resident.first, resident.second, 1.5f)) {
                            canInteract = true;
                            break;
                        }
                    }
                }
            }
            
            // 衛兵の近くにいて、攻撃可能な場合
            if (!canInteract && canAttackGuards) {
                for (auto& guard : guards) {
                    if (GameUtils::isNearPositionEuclidean(playerX, playerY, guard.first, guard.second, 1.5f)) {
                        canInteract = true;
                        break;
                    }
                }
            }
            
            // 城の近くにいて、入場可能な場合
            bool nearCastle = false;
            if (!canInteract && canEnterCastle) {
                int distanceToCastle = std::max(abs(playerX - castleX), abs(playerY - castleY));
                if (distanceToCastle <= 2) {
                    canInteract = true;
                    nearCastle = true;
                }
            }
            
            if (canInteract) {
                int textX = playerX * TILE_SIZE + TILE_SIZE / 2;
                int textY = playerY * TILE_SIZE + TILE_SIZE + 5;
                
                // テキストを中央揃えにするため、テキストの幅を取得して調整
                SDL_Color textColor = {255, 255, 255, 255};
                SDL_Texture* enterTexture = graphics.createTextTexture("ENTER", "default", textColor);
                if (enterTexture) {
                    int textWidth, textHeight;
                    SDL_QueryTexture(enterTexture, nullptr, nullptr, &textWidth, &textHeight);
                    
                    // テキストサイズを小さくする（80%に縮小）
                    const float SCALE = 0.8f;
                    int scaledWidth = static_cast<int>(textWidth * SCALE);
                    int scaledHeight = static_cast<int>(textHeight * SCALE);
                    
                    // パディングを追加
                    const int PADDING = 4;
                    int bgWidth = scaledWidth + PADDING * 2;
                    int bgHeight = scaledHeight + PADDING * 2;
                    
                    // 背景の位置を計算（中央揃え）
                    int bgX = textX - bgWidth / 2;
                    int bgY = textY;
                    
                    // 黒背景を描画
                    graphics.setDrawColor(0, 0, 0, 200); // 半透明の黒
                    graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
                    
                    // 白い枠線を描画
                    graphics.setDrawColor(255, 255, 255, 255); // 白
                    graphics.drawRect(bgX, bgY, bgWidth, bgHeight, false);
                    
                    // テキストを小さく描画（中央揃え）
                    int drawX = textX - scaledWidth / 2;
                    int drawY = textY + PADDING;
                    graphics.drawTexture(enterTexture, drawX, drawY, scaledWidth, scaledHeight);
                    SDL_DestroyTexture(enterTexture);
                } else {
                    // フォールバック: drawTextを使用
                    graphics.drawText("ENTER", textX, textY, "default", textColor);
                }
            }
            
            // 城に入れる時、城の周りにも「ENTER」を表示
            if (canEnterCastle && !nearCastle) {
                // 城の周り（距離2以内）の位置に「ENTER」を表示
                for (int dy = -2; dy <= 2; dy++) {
                    for (int dx = -2; dx <= 2; dx++) {
                        int checkX = castleX + dx;
                        int checkY = castleY + dy;
                        int distanceToCastle = std::max(abs(dx), abs(dy));
                        
                        // 距離2以内で、プレイヤーがその位置にいない場合
                        if (distanceToCastle <= 2 && distanceToCastle > 0 && 
                            (checkX != playerX || checkY != playerY) &&
                            isValidPosition(checkX, checkY)) {
                            
                            int textX = checkX * TILE_SIZE + TILE_SIZE / 2;
                            int textY = checkY * TILE_SIZE + TILE_SIZE + 5;
                            
                            // テキストを中央揃えにするため、テキストの幅を取得して調整
                            SDL_Color textColor = {255, 255, 255, 255};
                            SDL_Texture* enterTexture = graphics.createTextTexture("ENTER", "default", textColor);
                            if (enterTexture) {
                                int textWidth, textHeight;
                                SDL_QueryTexture(enterTexture, nullptr, nullptr, &textWidth, &textHeight);
                                
                                // テキストサイズを小さくする（80%に縮小）
                                const float SCALE = 0.8f;
                                int scaledWidth = static_cast<int>(textWidth * SCALE);
                                int scaledHeight = static_cast<int>(textHeight * SCALE);
                                
                                // パディングを追加
                                const int PADDING = 4;
                                int bgWidth = scaledWidth + PADDING * 2;
                                int bgHeight = scaledHeight + PADDING * 2;
                                
                                // 背景の位置を計算（中央揃え）
                                int bgX = textX - bgWidth / 2;
                                int bgY = textY;
                                
                                // 黒背景を描画
                                graphics.setDrawColor(0, 0, 0, 200); // 半透明の黒
                                graphics.drawRect(bgX, bgY, bgWidth, bgHeight, true);
                                
                                // 白い枠線を描画
                                graphics.setDrawColor(255, 255, 255, 255); // 白
                                graphics.drawRect(bgX, bgY, bgWidth, bgHeight, false);
                                
                                // テキストを小さく描画（中央揃え）
                                int drawX = textX - scaledWidth / 2;
                                int drawY = textY + PADDING;
                                graphics.drawTexture(enterTexture, drawX, drawY, scaledWidth, scaledHeight);
                                SDL_DestroyTexture(enterTexture);
                            } else {
                                // フォールバック: drawTextを使用
                                graphics.drawText("ENTER", textX, textY, "default", textColor);
                            }
                        }
                    }
                }
            }
        }
        
        ui.render(graphics);
        
        graphics.present();
    } catch (const std::exception& e) {
    }
}

void NightState::handleInput(const InputManager& input) {
    ui.handleInput(input);
    
    // 説明表示中の処理
    if (showGameExplanation) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            explanationStep++;
            
            if (explanationStep >= gameExplanationTexts.size()) {
                showGameExplanation = false;
                explanationStep = 0;
                clearExplanationMessage();
                
                // 説明UIが完全に終わったことを記録
                player->hasSeenNightExplanation = true;
            } else {
                showExplanationMessage(gameExplanationTexts[explanationStep]);
            }
        }
        return; // 説明中は他の操作を無効化
    }
    
    if (isShowingResidentChoice && !isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::UP) || input.isKeyJustPressed(InputKey::W)) {
            selectedChoice = (selectedChoice - 1 + choiceOptions.size()) % choiceOptions.size();
            updateChoiceDisplay();
        } else if (input.isKeyJustPressed(InputKey::DOWN) || input.isKeyJustPressed(InputKey::S)) {
            selectedChoice = (selectedChoice + 1) % choiceOptions.size();
            updateChoiceDisplay();
        }
        
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            handleResidentChoice();
        }
        return; // 選択肢表示中は他の操作を無効化
    }
    
    if (isShowingMessage) {
        if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
            clearMessage();
            
            if (showGuardKilledMessage) {
                // 衛兵を倒したメッセージを閉じる
                showGuardKilledMessage = false;
                isShowingMessage = false;
                clearMessage();
                
                // 全ての衛兵を倒した場合は、次のメッセージを表示
                if (showAllGuardsKilledMessage) {
                    showAllGuardsKilledMessage = false;
                    showMessage("衛兵も全て倒せたようですね。それでは最後は城に入って王様を倒しましょう。城の近くに行き、Enterで城に入れます。");
                    return;
                }
                return;
            }
            else if (showGuardMessage) {
                // 衛兵との戦闘を開始
                showGuardMessage = false;
                isShowingMessage = false;
                
                // 衛兵を敵として戦闘を開始
                Enemy guardEnemy(EnemyType::GUARD);
                guardEnemy.setLevel(105); // レベル105に設定
                
                if (stateManager) {
                    // 戦闘に入る前にNightStateの状態を保存
                    saveCurrentState(player);
                    
                    stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(guardEnemy)));
                }
                return;
            }
            else if (isShowingResidentChoice) {
                isShowingMessage = false;
                updateChoiceDisplay();
            }
            else if (showResidentKilledMessage) {
                showResidentKilledMessage = false;
                // 魔王の信頼度が上がったメッセージを表示
                if (totalResidentsKilled <= 6) {
                    showMessage("住民を倒しました。\n勇者: ああぁぁぁ！！本当にごめんなさい。私じゃ魔王に勝てないから仕方ないのよ・・・\n\n魔王からの信頼度 +10 メンタル -20");
                } else {
                    showMessage("住民を倒しました。\n勇者: フフッ！！だんだん楽シクなってきたヨォ！！\n\n魔王からの信頼度 +10 メンタル +20");
                }
                // 3人目を倒した場合は、次のメッセージで街に戻る処理を実行するフラグを設定
                // ただし、住民を全て倒した場合はメッセージを表示しない
                const auto& killedResidents = player->getKilledResidents();
                bool allKilled = TownLayout::areAllResidentsKilled(killedResidents);
                if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT && !allKilled) {
                    showReturnToTownMessage = true;
                }
            }
            else if (showReturnToTownMessage) {
                showReturnToTownMessage = false;
                // 街に戻るメッセージを表示
                int currentNight = player->getCurrentNight();
                if (currentNight < 4) {
                    showMessage("住民を3人倒しました。これ以上は危険です。\n街に戻ります。");
                    // 次のEnterで街に戻る処理を実行するフラグを設定
                    shouldReturnToTown = true;
                } else {
                    // 4夜目以降でも、1夜に3人倒した場合は街に戻る
                    showMessage("住民を3人倒しました。これ以上は危険です。\n街に戻ります。");
                    // 次のEnterで街に戻る処理を実行するフラグを設定
                    shouldReturnToTown = true;
                }
            }
            else if (shouldReturnToTown) {
                // 街に戻るメッセージを表示した後、Enterを押した時に街に戻る処理を実行
                shouldReturnToTown = false;
                int currentNight = player->getCurrentNight();
                if (stateManager) {
                    // 1夜に倒した人数をリセット（次の夜のために）
                    residentsKilled = 0;
                    TownState::s_nightTimerActive = true;
                    TownState::s_nightTimer = 900.0f; // 15分 = 900秒
                    stateManager->changeState(std::make_unique<TownState>(player));
                }
            }
            else if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT) {
                // 3人目を倒したが、メッセージ表示フローを経由していない場合のフォールバック
                // （通常はshowReturnToTownMessageフラグで処理されるが、念のため）
                // ただし、住民を全て倒した場合は街に戻らない
                const auto& killedResidents = player->getKilledResidents();
                bool allKilled = TownLayout::areAllResidentsKilled(killedResidents);
                if (stateManager && player->getCurrentNight() < 4 && !allKilled) {
                    // 1夜に倒した人数をリセット（次の夜のために）
                    residentsKilled = 0;
                    TownState::s_nightTimerActive = true;
                    TownState::s_nightTimer = 900.0f; // 20分 = 1200秒
                    stateManager->changeState(std::make_unique<TownState>(player));
                }
            }
        }
        return; // メッセージ表示中は他の操作を無効化
    }
    
    if (input.isKeyJustPressed(InputKey::ESCAPE) || input.isKeyJustPressed(InputKey::GAMEPAD_B)) {
        if (stateManager) {
            TownState::s_nightTimerActive = true;
            TownState::s_nightTimer = 900.0f; // 20分 = 1200秒
            stateManager->changeState(std::make_unique<TownState>(player));
        }
        return;
    }
    
    if (input.isKeyJustPressed(InputKey::ENTER) || input.isKeyJustPressed(InputKey::GAMEPAD_A)) {
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
        
        auto& config = UIConfig::UIConfigManager::getInstance();
        auto nightConfig = config.getNightConfig();
        
        // メッセージボード（画面中央下部）
        int messageX, messageY;
        config.calculatePosition(messageX, messageY, nightConfig.messageBoard.text.position, 1100, 650);
        auto messageBoardLabel = std::make_unique<Label>(messageX, messageY, "", "default");
        messageBoardLabel->setColor(nightConfig.messageBoard.text.color);
        messageBoardLabel->setText("");
        messageBoard = messageBoardLabel.get(); // ポインタを保存
        ui.addElement(std::move(messageBoardLabel));
        
        // 夜の表示ラベル
        int nightDisplayX, nightDisplayY;
        config.calculatePosition(nightDisplayX, nightDisplayY, nightConfig.nightDisplayText.position, 1100, 650);
        auto nightDisplayLabel = std::make_unique<Label>(nightDisplayX, nightDisplayY, "", "default");
        nightDisplayLabel->setColor(nightConfig.nightDisplayText.color);
        nightDisplayLabel->setText("");
        this->nightDisplayLabel = nightDisplayLabel.get(); // ポインタを保存
        ui.addElement(std::move(nightDisplayLabel));

        // 夜の操作ラベル
        int nightOperationX, nightOperationY;
        config.calculatePosition(nightOperationX, nightOperationY, nightConfig.nightOperationText.position, 1100, 650);
        auto nightOperationLabel = std::make_unique<Label>(nightOperationX, nightOperationY, "住民と話す: ENTER", "default");
        nightOperationLabel->setColor(nightConfig.nightOperationText.color);
        this->nightOperationLabel = nightOperationLabel.get(); // ポインタを保存
        ui.addElement(std::move(nightOperationLabel));
        
        // 説明用メッセージボード（TownStateと同じ仕様）
        auto mbConfig = config.getMessageBoardConfig();
        
        int textX, textY;
        config.calculatePosition(textX, textY, mbConfig.text.position, 1100, 650);  // 画面サイズは固定値（実際の画面サイズはrender()で取得可能だが、ここでは固定値を使用）
        
        auto explanationLabelPtr = std::make_unique<Label>(textX, textY, "", "default");
        explanationLabelPtr->setColor(mbConfig.text.color);
        explanationLabelPtr->setText("");
        explanationMessageBoard = explanationLabelPtr.get();
        ui.addElement(std::move(explanationLabelPtr));
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
    
    // 1夜に倒せる住民の最大値は3人まで（ただし、全ての住民を倒した場合は除く）
    // 実際に全ての住民を倒したかどうかを確認
    const auto& killedResidents = player->getKilledResidents();
    bool allKilled = TownLayout::areAllResidentsKilled(killedResidents);
    
    // 1夜に3人倒した場合、かつ全ての住民を倒していない場合は、街に戻る
    if (residentsKilled >= MAX_RESIDENTS_PER_NIGHT && !allKilled) {
        int currentNight = player->getCurrentNight();
        if (currentNight < 4) {
            showMessage("住民を3人倒しました。これ以上は危険です。\n街に戻ります。");
        } else {
            // 4夜目以降でも、全ての住民を倒していない場合は街に戻る
            showMessage("住民を3人倒しました。これ以上は危険です。\n街に戻ります。");
        }
        return;
    }
    
    // 全ての住民を倒した場合は、checkGameProgress()で処理されるため、ここでは何もしない
    // （canAttackGuardsがtrueの場合は、既に全ての住民を倒しているので、住民との対話はできない）
    if (allKilled && canAttackGuards) {
        return;
    }
    
    if (!canAttackGuards && isNearGuard(x, y)) {
        player->setKingTrust(0); // 王様からの信頼度を0に設定
        
        if (stateManager) {
            stateManager->changeState(std::make_unique<GameOverState>(player, "衛兵に見つかりました。"));
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
    
    showMessage("住民：「勇者様！？なぜこのようなことを！？お許しください！\n私には家族がいるんです・・・！どうか・・・」");
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
        std::string residentName = TownLayout::getResidentName(currentTargetX, currentTargetY);
        residentEnemy.setName(residentName);
        
        // 住民の画像インデックスを取得して設定
        int textureIndex = TownLayout::getResidentTextureIndex(currentTargetX, currentTargetY);
        residentEnemy.setResidentTextureIndex(textureIndex);
        
        // 住民の位置情報を設定
        residentEnemy.setResidentPosition(currentTargetX, currentTargetY);
        
        if (stateManager) {
            // 戦闘に入る前にNightStateの状態を保存
            saveCurrentState(player);
            
            stateManager->changeState(std::make_unique<BattleState>(player, std::make_unique<Enemy>(residentEnemy)));
        }
        return;
    } else if (choice == 1) {
        // 「倒さない」を選択した場合の処理（既存の処理）
        isShowingResidentChoice = false;
        
        player->changeDemonTrust(-10);
        player->changeMental(20);
        
        showMessage("住民：「ありがとうございます！本当にありがとうございます！この事は絶対に秘密にしますから！」\n魔王からの信頼度 -10 メンタル +20");
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
        // アスペクト比を保持して縦幅に合わせて描画
        int centerX = playerX * TILE_SIZE + TILE_SIZE / 2;
        int centerY = playerY * TILE_SIZE + TILE_SIZE / 2;
        graphics.drawTextureAspectRatio(playerTexture, centerX, centerY, TILE_SIZE, true, true);
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

void NightState::handleResidentKilled(int x, int y) {
    residentsKilled++;
    
    player->changeDemonTrust(10); // 魔王からの信頼度を10上昇
    
    // 重複チェック
    if (!TownLayout::isResidentKilled(x, y, killedResidentPositions)) {
        killedResidentPositions.push_back({x, y});
    }
    player->addKilledResident(x, y);
    
    // totalResidentsKilledはplayer->getKilledResidents().size()で更新
    totalResidentsKilled = player->getKilledResidents().size();
    if (totalResidentsKilled >= 6) { // 合計6人以上倒した場合
        player->changeMental(20); // メンタルを20上昇
    } else {
        player->changeMental(-20); // メンタルを20減少
    }
    
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
    // 現在のStateの状態を保存
    saveCurrentState(player);
    
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
        
        SDL_Texture* texture = nullptr;
        if (type == "shop") {
            texture = shopTexture;
        } else if (type == "weapon_shop") {
            texture = weaponShopTexture;
        } else if (type == "house") {
            texture = houseTexture;
        } else if (type == "castle") {
            texture = castleTexture;
        }
        
        auto [r, g, b] = TownLayout::getBuildingColor(type);
        GameState::drawBuilding(graphics, x, y, TILE_SIZE, 2, texture, r, g, b);
    }
    
    for (const auto& home : residentHomes) {
        GameState::drawBuilding(graphics, home.first, home.second, TILE_SIZE, 2,
                               residentHomeTexture, 139, 69, 19);
    }
}

void NightState::drawGate(Graphics& graphics) {
    GameState::drawGate(graphics, TownLayout::GATE_X, TownLayout::GATE_Y, TILE_SIZE,
                       stoneTileTexture, toriiTexture);
}

void NightState::checkGameProgress() {
    // 住民を全て倒したかどうかを正確に判定
    const auto& killedResidents = player->getKilledResidents();
    bool allKilled = TownLayout::areAllResidentsKilled(killedResidents);
    
    if (!allResidentsKilled && allKilled) {
        allResidentsKilled = true;
        canAttackGuards = true;
        showMessage("住民を全員倒せたようですね。残るは衛兵と王様だけです。\nまずは外にいる衛兵を全員倒してしまいましょう。衛兵に近づいてEnterで戦闘に入れます。");
    }
    
    if (allResidentsKilled && !allGuardsKilled && guards.empty()) {
        allGuardsKilled = true;
        canEnterCastle = true;
        showMessage("衛兵も全て倒せたようですね。それでは最後は城に入って王様を倒しましょう。城の近くに行き、Enterで城に入れます。");
    }
}

void NightState::checkGuardInteraction() {
    for (auto& guard : guards) {
        if (GameUtils::isNearPositionEuclidean(playerX, playerY, guard.first, guard.second, 1.5f)) {
            // 戦闘に移行する前にプレイヤーの位置を保存
            s_savedPlayerX = playerX;
            s_savedPlayerY = playerY;
            s_playerPositionSaved = true;
            
            currentGuardX = guard.first;
            currentGuardY = guard.second;
            
            // メッセージを表示
            showMessage("衛兵：「勇者！やはりお前だったのか！俺はお前が怪しいと思っていたんだ！これ以上は好きにはさせない！」");
            showGuardMessage = true;
            return;
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

void NightState::setupGameExplanation() {
    gameExplanationTexts.clear();
    gameExplanationTexts.push_back("目標レベル到達おめでとう！素晴らしい戦いだったよ！");
    gameExplanationTexts.push_back("さて、次は夜の街だね。ここからが堕天勇者としての最大の見せ場だよ。");
    gameExplanationTexts.push_back("なんてったってこれから街を滅ぼすんだからね。");
    gameExplanationTexts.push_back("じゃあまず家の前に住民が立っているのは見える？");
    gameExplanationTexts.push_back("合計12人いるみたい。\n警戒されずに倒せるのが一夜にせいぜい3人だから、滅ぼすためには四夜必要だね。");
    gameExplanationTexts.push_back("本題、夜の街で行うことは衛兵に見つからずに住民を倒すことだよ。");
    gameExplanationTexts.push_back("衛兵は各家を巡回しているから、衛兵がちょうどいないタイミングで住民に話しかけるんだ。");
    gameExplanationTexts.push_back("話しかけるのは住民の前でEnterだよ。");
    gameExplanationTexts.push_back("そうすると住民と戦うかどうか選択できるんだ。");
    gameExplanationTexts.push_back("基本的には戦うを選択して住民をどんどん倒していくよ。");
    gameExplanationTexts.push_back("ここでさっき説明を飛ばした、メンタル、魔王からの信頼について説明していくよ。");
    gameExplanationTexts.push_back("メンタルっていうのは君の精神状態だよ。\n住民を倒していくうちにメンタルが下がっていくんだ。\n倒し慣れると楽しくなって逆に上がるって人もいるみたいだけどね。");
    gameExplanationTexts.push_back("メンタルが下がると住民との戦闘で攻撃の命中率が下がってしまうんだ。");
    gameExplanationTexts.push_back("メンタルを回復するには住民と戦わないを選択するといいよ。\nその代わり魔王からの信頼が下がるから注意してね。");
    gameExplanationTexts.push_back("魔王からの信頼は住民を倒すことで上がっていくよ。");
    gameExplanationTexts.push_back("でもこの時王様からの信頼は下がっちゃうから、\n日中にモンスターを倒して王様からの信頼をあげて、\n夜は住民を倒して魔王からの信頼をあげて、というのを繰り返してバランスを保つのが重要だよ。");
    gameExplanationTexts.push_back("一旦説明は以上だよ！とりあえず住民と戦ってみよう！");
}

void NightState::showExplanationMessage(const std::string& message) {
    if (explanationMessageBoard) {
        explanationMessageBoard->setText(message);
    }
}

void NightState::clearExplanationMessage() {
    if (explanationMessageBoard) {
        explanationMessageBoard->setText("");
    }
}

nlohmann::json NightState::toJson() const {
    nlohmann::json j;
    j["stateType"] = static_cast<int>(StateType::NIGHT);
    j["playerX"] = playerX;
    j["playerY"] = playerY;
    j["isStealthMode"] = isStealthMode;
    j["stealthLevel"] = stealthLevel;
    j["residentsKilled"] = residentsKilled;
    j["totalResidentsKilled"] = totalResidentsKilled;
    j["allResidentsKilled"] = allResidentsKilled;
    j["allGuardsKilled"] = allGuardsKilled;
    j["canAttackGuards"] = canAttackGuards;
    j["canEnterCastle"] = canEnterCastle;
    j["showResidentKilledMessage"] = showResidentKilledMessage;
    j["showReturnToTownMessage"] = showReturnToTownMessage;
    j["shouldReturnToTown"] = shouldReturnToTown;
    j["isShowingResidentChoice"] = isShowingResidentChoice;
    j["isShowingMercyChoice"] = isShowingMercyChoice;
    j["selectedChoice"] = selectedChoice;
    j["currentTargetX"] = currentTargetX;
    j["currentTargetY"] = currentTargetY;
    j["showGuardMessage"] = showGuardMessage;
    j["showGuardKilledMessage"] = showGuardKilledMessage;
    j["showAllGuardsKilledMessage"] = showAllGuardsKilledMessage;
    j["currentGuardX"] = currentGuardX;
    j["currentGuardY"] = currentGuardY;
    
    // 住民の位置を保存
    j["residents"] = TownLayout::positionsToJson(residents);
    
    // 衛兵の位置を保存
    j["guards"] = TownLayout::positionsToJson(guards);
    
    // 倒した住民の位置を保存
    j["killedResidentPositions"] = TownLayout::positionsToJson(killedResidentPositions);
    
    // 衛兵のHPを保存
    nlohmann::json guardHpJson = nlohmann::json::array();
    for (int hp : guardHp) {
        guardHpJson.push_back(hp);
    }
    j["guardHp"] = guardHpJson;
    
    return j;
}

void NightState::fromJson(const nlohmann::json& j) {
    if (j.contains("playerX")) playerX = j["playerX"];
    if (j.contains("playerY")) playerY = j["playerY"];
    if (j.contains("isStealthMode")) isStealthMode = j["isStealthMode"];
    if (j.contains("stealthLevel")) stealthLevel = j["stealthLevel"];
    if (j.contains("residentsKilled")) residentsKilled = j["residentsKilled"];
    if (j.contains("totalResidentsKilled")) totalResidentsKilled = j["totalResidentsKilled"];
    if (j.contains("allResidentsKilled")) allResidentsKilled = j["allResidentsKilled"];
    if (j.contains("allGuardsKilled")) allGuardsKilled = j["allGuardsKilled"];
    if (j.contains("canAttackGuards")) canAttackGuards = j["canAttackGuards"];
    if (j.contains("canEnterCastle")) canEnterCastle = j["canEnterCastle"];
    if (j.contains("showResidentKilledMessage")) showResidentKilledMessage = j["showResidentKilledMessage"];
    if (j.contains("showReturnToTownMessage")) showReturnToTownMessage = j["showReturnToTownMessage"];
    if (j.contains("shouldReturnToTown")) shouldReturnToTown = j["shouldReturnToTown"];
    if (j.contains("isShowingResidentChoice")) isShowingResidentChoice = j["isShowingResidentChoice"];
    if (j.contains("isShowingMercyChoice")) isShowingMercyChoice = j["isShowingMercyChoice"];
    if (j.contains("selectedChoice")) selectedChoice = j["selectedChoice"];
    if (j.contains("currentTargetX")) currentTargetX = j["currentTargetX"];
    if (j.contains("currentTargetY")) currentTargetY = j["currentTargetY"];
    if (j.contains("showGuardMessage")) showGuardMessage = j["showGuardMessage"];
    if (j.contains("showGuardKilledMessage")) showGuardKilledMessage = j["showGuardKilledMessage"];
    if (j.contains("showAllGuardsKilledMessage")) showAllGuardsKilledMessage = j["showAllGuardsKilledMessage"];
    if (j.contains("currentGuardX")) currentGuardX = j["currentGuardX"];
    if (j.contains("currentGuardY")) currentGuardY = j["currentGuardY"];
    if (j.contains("showGameExplanation")) showGameExplanation = j["showGameExplanation"];
    if (j.contains("explanationStep")) explanationStep = j["explanationStep"];
    // gameExplanationTextsも復元
    if (j.contains("gameExplanationTexts") && j["gameExplanationTexts"].is_array()) {
        gameExplanationTexts.clear();
        for (const auto& text : j["gameExplanationTexts"]) {
            gameExplanationTexts.push_back(text.get<std::string>());
        }
    }
    
    // 説明UIが完了していない場合は最初から始める（explanationStepをリセット）
    if (!player->hasSeenNightExplanation && showGameExplanation) {
        explanationStep = 0;
    }
    
    // 住民の位置を復元
    if (j.contains("residents") && j["residents"].is_array()) {
        residents = TownLayout::positionsFromJson(j["residents"]);
    }
    
    // 衛兵の位置を復元
    if (j.contains("guards") && j["guards"].is_array()) {
        guards = TownLayout::positionsFromJson(j["guards"]);
    }
    
    // 倒した住民の位置を復元（重複を除去）
    if (j.contains("killedResidentPositions") && j["killedResidentPositions"].is_array()) {
        killedResidentPositions = TownLayout::positionsFromJson(j["killedResidentPositions"]);
        TownLayout::removeDuplicatePositions(killedResidentPositions);
        totalResidentsKilled = killedResidentPositions.size();
    }
    
    // 衛兵のHPを復元
    if (j.contains("guardHp") && j["guardHp"].is_array()) {
        guardHp.clear();
        for (const auto& hpJson : j["guardHp"]) {
            guardHp.push_back(hpJson.get<int>());
        }
    }
}

void NightState::setRemainingGuards(int remainingCount) {
    if (remainingCount < 0 || remainingCount > static_cast<int>(guards.size())) {
        return;
    }
    
    // 指定数だけ残す（最初のremainingCount体を残す）
    guards.erase(guards.begin() + remainingCount, guards.end());
    guardHp.erase(guardHp.begin() + remainingCount, guardHp.end());
    guardTargetHomeIndices.erase(guardTargetHomeIndices.begin() + remainingCount, guardTargetHomeIndices.end());
    guardStayTimers.erase(guardStayTimers.begin() + remainingCount, guardStayTimers.end());
    
    // 住民を全て倒した状態にする（衛兵を攻撃可能にする）
    allResidentsKilled = true;
    canAttackGuards = true;
}
                                                 