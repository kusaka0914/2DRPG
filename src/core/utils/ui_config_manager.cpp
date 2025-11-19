#include "ui_config_manager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <ctime>
#ifdef _WIN32
    #include <sys/stat.h>
    #include <io.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace UIConfig {
    UIConfigManager& UIConfigManager::getInstance() {
        static UIConfigManager instance;
        return instance;
    }
    
    void UIConfigManager::setDefaultValues() {
        // メッセージボード（デフォルト値）
        messageBoardConfig.background.position.useRelative = false;
        messageBoardConfig.background.position.absoluteX = 190.0f;
        messageBoardConfig.background.position.absoluteY = 480.0f;
        messageBoardConfig.background.width = 720;
        messageBoardConfig.background.height = 100;
        
        messageBoardConfig.text.position.useRelative = false;
        messageBoardConfig.text.position.absoluteX = 210.0f;
        messageBoardConfig.text.position.absoluteY = 500.0f;
        messageBoardConfig.text.color = {255, 255, 255, 255};
        messageBoardConfig.backgroundColor = {0, 0, 0, 255};
        messageBoardConfig.borderColor = {255, 255, 255, 255};
        
        commonUIConfig.nightTimer.position.useRelative = false;
        commonUIConfig.nightTimer.position.absoluteX = 10.0f;
        commonUIConfig.nightTimer.position.absoluteY = 10.0f;
        commonUIConfig.nightTimer.width = 160;
        commonUIConfig.nightTimer.height = 40;
        
        commonUIConfig.nightTimerText.position.useRelative = false;
        commonUIConfig.nightTimerText.position.absoluteX = 25.0f;
        commonUIConfig.nightTimerText.position.absoluteY = 20.0f;
        commonUIConfig.nightTimerText.color = {255, 255, 255, 255};
        
        // CommonUI - 目標レベル
        commonUIConfig.targetLevel.position.useRelative = false;
        commonUIConfig.targetLevel.position.absoluteX = 10.0f;
        commonUIConfig.targetLevel.position.absoluteY = 60.0f;
        commonUIConfig.targetLevel.width = 160;
        commonUIConfig.targetLevel.height = 60;
        
        commonUIConfig.targetLevelText.position.useRelative = false;
        commonUIConfig.targetLevelText.position.absoluteX = 25.0f;
        commonUIConfig.targetLevelText.position.absoluteY = 70.0f;
        commonUIConfig.targetLevelText.color = {255, 255, 255, 255};
        
        // CommonUI - 信頼度表示
        commonUIConfig.trustLevels.position.useRelative = false;
        commonUIConfig.trustLevels.position.absoluteX = 900.0f;
        commonUIConfig.trustLevels.position.absoluteY = 10.0f;
        commonUIConfig.trustLevels.width = 180;
        commonUIConfig.trustLevels.height = 80;
        
        commonUIConfig.trustLevelsText.position.useRelative = false;
        commonUIConfig.trustLevelsText.position.absoluteX = 915.0f;
        commonUIConfig.trustLevelsText.position.absoluteY = 20.0f;
        commonUIConfig.trustLevelsText.color = {255, 255, 255, 255};
        
        // CommonUI - ゲームコントローラー状態
        commonUIConfig.gameControllerStatus.position.useRelative = false;
        commonUIConfig.gameControllerStatus.position.absoluteX = 700.0f;
        commonUIConfig.gameControllerStatus.position.absoluteY = 10.0f;
        commonUIConfig.gameControllerStatus.width = 80;
        commonUIConfig.gameControllerStatus.height = 20;
        commonUIConfig.gameControllerStatusColor = {0, 255, 0, 255};
        
        // 目標レベルの色と行間
        commonUIConfig.targetLevelAchievedColor = {0, 255, 0, 255};
        commonUIConfig.targetLevelRemainingColor = {255, 255, 0, 255};
        commonUIConfig.targetLevelLineSpacing = 20;
        
        // 信頼度の行間
        commonUIConfig.trustLevelsLineSpacing1 = 20;
        commonUIConfig.trustLevelsLineSpacing2 = 40;
        
        // 背景色とボーダー色
        commonUIConfig.backgroundColor = {0, 0, 0, 255};
        commonUIConfig.borderColor = {255, 255, 255, 255};
        commonUIConfig.backgroundAlpha = 200;
        
        // MainMenuState
        mainMenuConfig.title.position.useRelative = false;
        mainMenuConfig.title.position.absoluteX = 350.0f;
        mainMenuConfig.title.position.absoluteY = 250.0f;
        mainMenuConfig.title.color = {255, 215, 0, 255};
        
        mainMenuConfig.playerInfo.position.useRelative = false;
        mainMenuConfig.playerInfo.position.absoluteX = 50.0f;
        mainMenuConfig.playerInfo.position.absoluteY = 120.0f;
        mainMenuConfig.playerInfo.color = {255, 255, 255, 255};
        
        mainMenuConfig.adventureButton.position.useRelative = false;
        mainMenuConfig.adventureButton.position.absoluteX = 450.0f;
        mainMenuConfig.adventureButton.position.absoluteY = 350.0f;
        mainMenuConfig.adventureButton.width = 200;
        mainMenuConfig.adventureButton.height = 50;
        mainMenuConfig.adventureButton.normalColor = {0, 100, 0, 255};
        mainMenuConfig.adventureButton.hoverColor = {0, 150, 0, 255};
        mainMenuConfig.adventureButton.pressedColor = {0, 50, 0, 255};
        mainMenuConfig.adventureButton.textColor = {255, 255, 255, 255};
        
        // "START GAME : PRESS ENTER"テキスト
        mainMenuConfig.startGameText.position.useRelative = true;
        mainMenuConfig.startGameText.position.offsetX = 0.0f;
        mainMenuConfig.startGameText.position.offsetY = 100.0f;  // 画面下部から100px上（相対位置）
        mainMenuConfig.startGameText.color = {255, 255, 255, 255};
        
        // BattleState
        battleConfig.battleLog.position.useRelative = false;
        battleConfig.battleLog.position.absoluteX = 150.0f;
        battleConfig.battleLog.position.absoluteY = 400.0f;
        battleConfig.battleLog.color = {255, 255, 255, 255};
        
        battleConfig.playerStatus.position.useRelative = false;
        battleConfig.playerStatus.position.absoluteX = 50.0f;
        battleConfig.playerStatus.position.absoluteY = 50.0f;
        battleConfig.playerStatus.color = {255, 255, 255, 255};
        
        battleConfig.enemyStatus.position.useRelative = false;
        battleConfig.enemyStatus.position.absoluteX = 400.0f;
        battleConfig.enemyStatus.position.absoluteY = 50.0f;
        battleConfig.enemyStatus.color = {255, 255, 255, 255};
        
        battleConfig.message.position.useRelative = false;
        battleConfig.message.position.absoluteX = 50.0f;
        battleConfig.message.position.absoluteY = 450.0f;
        battleConfig.message.color = {255, 255, 255, 255};
        
        battleConfig.playerHp.position.useRelative = false;
        battleConfig.playerHp.position.absoluteX = 500.0f;
        battleConfig.playerHp.position.absoluteY = 260.0f;
        battleConfig.playerHp.color = {255, 255, 255, 255};
        
        battleConfig.playerMp.position.useRelative = false;
        battleConfig.playerMp.position.absoluteX = 500.0f;
        battleConfig.playerMp.position.absoluteY = 280.0f;
        battleConfig.playerMp.color = {255, 255, 255, 255};
        
        battleConfig.enemyHp.position.useRelative = false;
        battleConfig.enemyHp.position.absoluteX = 500.0f;
        battleConfig.enemyHp.position.absoluteY = 120.0f;
        battleConfig.enemyHp.color = {255, 255, 255, 255};
        
        battleConfig.enemyPosition.useRelative = false;
        battleConfig.enemyPosition.absoluteX = 500.0f;
        battleConfig.enemyPosition.absoluteY = 150.0f;
        battleConfig.enemyWidth = 100;
        battleConfig.enemyHeight = 100;
        
        battleConfig.playerPosition.useRelative = false;
        battleConfig.playerPosition.absoluteX = 500.0f;
        battleConfig.playerPosition.absoluteY = 300.0f;
        battleConfig.playerWidth = 60;
        battleConfig.playerHeight = 60;
        
        // 説明用メッセージボード
        battleConfig.explanationMessageBoard.background.position.useRelative = false;
        battleConfig.explanationMessageBoard.background.position.absoluteX = 190.0f;
        battleConfig.explanationMessageBoard.background.position.absoluteY = 480.0f;
        battleConfig.explanationMessageBoard.background.width = 720;
        battleConfig.explanationMessageBoard.background.height = 100;
        battleConfig.explanationMessageBoard.text.position.useRelative = false;
        battleConfig.explanationMessageBoard.text.position.absoluteX = 30.0f;  // 左下に配置
        battleConfig.explanationMessageBoard.text.position.absoluteY = 580.0f;  // 画面下部から背景の内側に配置
        battleConfig.explanationMessageBoard.text.color = {255, 255, 255, 255};
        
        // プレイヤー名の設定（相対位置オフセット）
        battleConfig.playerName.offsetX = -80.0f;
        battleConfig.playerName.offsetY = -40.0f;
        battleConfig.playerName.color = {255, 255, 255, 255};
        
        // 体力バーの設定（相対位置オフセット）
        battleConfig.healthBar.offsetX = 10.0f;
        battleConfig.healthBar.offsetY = -40.0f;
        battleConfig.healthBar.width = 150;
        battleConfig.healthBar.height = 20;
        battleConfig.healthBar.barColor = {150, 255, 150, 255};      // 黄緑色
        battleConfig.healthBar.bgColor = {50, 50, 50, 255};          // 暗いグレー
        battleConfig.healthBar.borderColor = {255, 255, 255, 255};   // 白色
        
        // コマンド選択UIの設定
        battleConfig.commandSelection.selectedCommandOffsetY = -150.0f;
        battleConfig.commandSelection.selectedCommandImageSize = 50;
        battleConfig.commandSelection.imageSpacing = 60;
        battleConfig.commandSelection.arrowSpacing = 20;
        battleConfig.commandSelection.buttonBaseOffsetY = -50.0f;
        battleConfig.commandSelection.buttonWidth = 200;
        battleConfig.commandSelection.buttonHeight = 60;
        battleConfig.commandSelection.buttonSpacing = 80;
        battleConfig.commandSelection.buttonImageSize = 60;
        battleConfig.commandSelection.selectedBgColor = {100, 200, 255, 200};
        battleConfig.commandSelection.unselectedBgColor = {50, 50, 50, 150};
        battleConfig.commandSelection.selectedBorderColor = {255, 215, 0, 255};
        battleConfig.commandSelection.unselectedBorderColor = {150, 150, 150, 255};
        battleConfig.commandSelection.selectedTextColor = {255, 255, 255, 255};
        battleConfig.commandSelection.unselectedTextColor = {200, 200, 200, 255};
        battleConfig.commandSelection.arrowColor = {255, 255, 255, 255};
        
        // 三すくみ画像の設定
        battleConfig.rockPaperScissors.position.useRelative = false;
        battleConfig.rockPaperScissors.position.absoluteX = 0.0f;  // 中央に配置（計算時に調整）
        battleConfig.rockPaperScissors.position.absoluteY = 20.0f;  // 上部から20px下
        battleConfig.rockPaperScissors.width = 200;
        
        // ジャッジ結果フェーズのUI設定
        battleConfig.judgeResult.resultText.position.useRelative = true;
        battleConfig.judgeResult.resultText.position.offsetY = -100.0f;
        battleConfig.judgeResult.resultText.baseWidth = 400;
        battleConfig.judgeResult.resultText.baseHeight = 100;
        battleConfig.judgeResult.resultText.victoryColor = {255, 215, 0, 255};
        battleConfig.judgeResult.resultText.defeatColor = {255, 0, 0, 255};
        battleConfig.judgeResult.resultText.drawColor = {200, 200, 200, 255};
        battleConfig.judgeResult.resultText.desperateVictoryColor = {255, 215, 0, 255};
        battleConfig.judgeResult.resultText.desperateDefeatColor = {255, 0, 0, 255};
        
        battleConfig.judgeResult.threeWinStreak.position.useRelative = true;
        battleConfig.judgeResult.threeWinStreak.position.offsetY = -150.0f;
        battleConfig.judgeResult.threeWinStreak.baseWidth = 300;
        battleConfig.judgeResult.threeWinStreak.baseHeight = 80;
        battleConfig.judgeResult.threeWinStreak.color = {255, 215, 0, 255};
        battleConfig.judgeResult.threeWinStreak.format = "3連勝！ダメージ{multiplier}倍ボーナス！";
        
        battleConfig.judgeResult.damageBonus.position.useRelative = true;
        battleConfig.judgeResult.damageBonus.position.offsetY = 80.0f;
        battleConfig.judgeResult.damageBonus.position.offsetX = -150.0f;
        battleConfig.judgeResult.damageBonus.color = {255, 255, 100, 255};
        battleConfig.judgeResult.damageBonus.format = "✨ ダメージ{multiplier}倍ボーナス！ ✨";
        
        // 勝敗UIの設定
        battleConfig.winLossUI.winLossText.position.useRelative = false;
        battleConfig.winLossUI.winLossText.position.offsetY = 20.0f;
        battleConfig.winLossUI.winLossText.color = {255, 255, 255, 255};
        battleConfig.winLossUI.winLossText.padding = 8;
        battleConfig.winLossUI.winLossText.format = "自分 {playerWins}勝  敵 {enemyWins}勝";
        
        battleConfig.winLossUI.totalAttackText.position.useRelative = false;
        battleConfig.winLossUI.totalAttackText.position.offsetY = 20.0f;
        battleConfig.winLossUI.totalAttackText.color = {255, 255, 255, 255};
        battleConfig.winLossUI.totalAttackText.padding = 8;
        battleConfig.winLossUI.totalAttackText.playerWinFormat = "{playerName}が{turns}ターン分の攻撃を実行！";
        battleConfig.winLossUI.totalAttackText.enemyWinFormat = "敵が{turns}ターン分の攻撃を実行！";
        battleConfig.winLossUI.totalAttackText.drawFormat = "両方がダメージを受ける";
        battleConfig.winLossUI.totalAttackText.hesitateFormat = "{playerName}はメンタルの影響で攻撃をためらいました。";
        
        battleConfig.winLossUI.attackText.position.useRelative = false;
        battleConfig.winLossUI.attackText.position.offsetY = 20.0f;
        battleConfig.winLossUI.attackText.color = {255, 255, 255, 255};
        battleConfig.winLossUI.attackText.padding = 8;
        battleConfig.winLossUI.attackText.attackFormat = "{playerName}のアタック！";
        battleConfig.winLossUI.attackText.rushFormat = "{playerName}のラッシュアタック！";
        battleConfig.winLossUI.attackText.statusUpSpellFormat = "{playerName}が強化呪文発動！";
        battleConfig.winLossUI.attackText.healSpellFormat = "{playerName}が回復呪文発動！";
        battleConfig.winLossUI.attackText.attackSpellFormat = "{playerName}が攻撃呪文発動！";
        battleConfig.winLossUI.attackText.defaultSpellFormat = "{playerName}が呪文発動！";
        battleConfig.winLossUI.attackText.defaultAttackFormat = "{playerName}の攻撃！";
        
        // コマンド選択ヒントテキストの設定
        battleConfig.commandHint.position.useRelative = true;
        battleConfig.commandHint.position.offsetY = -100.0f;
        battleConfig.commandHint.position.offsetX = -120.0f;
        battleConfig.commandHint.color = {255, 255, 255, 255};
        battleConfig.commandHint.padding = 8;
        battleConfig.commandHint.normalText = "選択: W/S 決定: Enter 戻る: Q";
        battleConfig.commandHint.residentText = "選択: W/S 決定: Enter";
        
        // ジャッジフェーズの結果表示UI設定
        battleConfig.judgePhase.position.useRelative = true;  // デフォルトは相対位置（画面中央）
        battleConfig.judgePhase.position.offsetX = 0.0f;
        battleConfig.judgePhase.position.offsetY = 0.0f;
        battleConfig.judgePhase.win.text = "勝ち！";
        battleConfig.judgePhase.win.color = {255, 215, 0, 255};
        battleConfig.judgePhase.lose.text = "負け...";
        battleConfig.judgePhase.lose.color = {255, 0, 0, 255};
        battleConfig.judgePhase.draw.text = "引き分け";
        battleConfig.judgePhase.draw.color = {200, 200, 200, 255};
        battleConfig.judgePhase.baseWidth = 200;
        battleConfig.judgePhase.baseHeight = 60;
        battleConfig.judgePhase.backgroundPadding = 20;
        battleConfig.judgePhase.glowColor = {255, 215, 0, 255};
        
        // ステータス上昇呪文の攻撃倍率表示UI設定
        battleConfig.attackMultiplier.position.useRelative = false;
        battleConfig.attackMultiplier.position.absoluteX = 0.0f;  // プレイヤーHPバーのXからの相対位置として使用
        battleConfig.attackMultiplier.position.absoluteY = 0.0f;  // プレイヤーHPバーのYからの相対位置として使用
        battleConfig.attackMultiplier.offsetX = -100.0f;
        battleConfig.attackMultiplier.offsetY = 0.0f;
        battleConfig.attackMultiplier.textColor = {255, 255, 100, 255};
        battleConfig.attackMultiplier.bgColor = {0, 0, 0, 255};
        battleConfig.attackMultiplier.borderColor = {255, 255, 100, 255};
        battleConfig.attackMultiplier.padding = 8;
        battleConfig.attackMultiplier.format = "攻撃倍率: {multiplier}倍 (残り{turns}ターン)";
        
        // RoomState
        roomConfig.messageBoard.background.position.useRelative = false;
        roomConfig.messageBoard.background.position.absoluteX = 190.0f;
        roomConfig.messageBoard.background.position.absoluteY = 480.0f;
        roomConfig.messageBoard.background.width = 720;
        roomConfig.messageBoard.background.height = 100;
        
        roomConfig.messageBoard.text.position.useRelative = false;
        roomConfig.messageBoard.text.position.absoluteX = 210.0f;
        roomConfig.messageBoard.text.position.absoluteY = 500.0f;
        roomConfig.messageBoard.text.color = {255, 255, 255, 255};
        
        roomConfig.howToOperateBackground.position.useRelative = false;
        roomConfig.howToOperateBackground.position.absoluteX = 190.0f;
        roomConfig.howToOperateBackground.position.absoluteY = 10.0f;
        roomConfig.howToOperateBackground.width = 395;
        roomConfig.howToOperateBackground.height = 70;
        
        roomConfig.howToOperateText.position.useRelative = false;
        roomConfig.howToOperateText.position.absoluteX = 205.0f;
        roomConfig.howToOperateText.position.absoluteY = 25.0f;
        roomConfig.howToOperateText.color = {255, 255, 255, 255};
        
        // TownState
        townConfig.playerInfo.position.useRelative = false;
        townConfig.playerInfo.position.absoluteX = 10.0f;
        townConfig.playerInfo.position.absoluteY = 10.0f;
        townConfig.playerInfo.color = {255, 255, 255, 255};
        
        townConfig.controls.position.useRelative = false;
        townConfig.controls.position.absoluteX = 10.0f;
        townConfig.controls.position.absoluteY = 550.0f;
        townConfig.controls.color = {255, 255, 255, 255};
        
        // CastleState
        castleConfig.messageBoard.background.position.useRelative = false;
        castleConfig.messageBoard.background.position.absoluteX = 190.0f;
        castleConfig.messageBoard.background.position.absoluteY = 480.0f;
        castleConfig.messageBoard.background.width = 720;
        castleConfig.messageBoard.background.height = 100;
        
        castleConfig.messageBoard.text.position.useRelative = false;
        castleConfig.messageBoard.text.position.absoluteX = 210.0f;
        castleConfig.messageBoard.text.position.absoluteY = 500.0f;
        castleConfig.messageBoard.text.color = {255, 255, 255, 255};
        
        // DemonCastleState
        demonCastleConfig.messageBoard.background.position.useRelative = false;
        demonCastleConfig.messageBoard.background.position.absoluteX = 190.0f;
        demonCastleConfig.messageBoard.background.position.absoluteY = 480.0f;
        demonCastleConfig.messageBoard.background.width = 720;
        demonCastleConfig.messageBoard.background.height = 100;
        
        demonCastleConfig.messageBoard.text.position.useRelative = false;
        demonCastleConfig.messageBoard.text.position.absoluteX = 210.0f;
        demonCastleConfig.messageBoard.text.position.absoluteY = 500.0f;
        demonCastleConfig.messageBoard.text.color = {255, 255, 255, 255};
        
        // GameOverState
        gameOverConfig.title.text.position.useRelative = false;
        gameOverConfig.title.text.position.absoluteX = 550.0f;
        gameOverConfig.title.text.position.absoluteY = 200.0f;
        gameOverConfig.title.text.color = {255, 0, 0, 255};
        
        gameOverConfig.reason.text.position.useRelative = false;
        gameOverConfig.reason.text.position.absoluteX = 550.0f;
        gameOverConfig.reason.text.position.absoluteY = 250.0f;
        gameOverConfig.reason.text.color = {255, 255, 255, 255};
        
        gameOverConfig.instruction.text.position.useRelative = false;
        gameOverConfig.instruction.text.position.absoluteX = 550.0f;
        gameOverConfig.instruction.text.position.absoluteY = 400.0f;
        gameOverConfig.instruction.text.color = {200, 200, 200, 255};
        
        // EndingState
        endingConfig.message.position.useRelative = false;
        endingConfig.message.position.absoluteX = 450.0f;
        endingConfig.message.position.absoluteY = 325.0f;
        endingConfig.message.color = {255, 255, 255, 255};
        
        endingConfig.staffRoll.position.useRelative = false;
        endingConfig.staffRoll.position.absoluteX = 450.0f;
        endingConfig.staffRoll.position.absoluteY = 325.0f;
        endingConfig.staffRoll.color = {255, 255, 255, 255};
        
        endingConfig.theEnd.position.useRelative = false;
        endingConfig.theEnd.position.absoluteX = 550.0f;
        endingConfig.theEnd.position.absoluteY = 300.0f;
        endingConfig.theEnd.color = {255, 255, 255, 255};
        
        endingConfig.returnToMenu.position.useRelative = false;
        endingConfig.returnToMenu.position.absoluteX = 550.0f;
        endingConfig.returnToMenu.position.absoluteY = 350.0f;
        endingConfig.returnToMenu.color = {200, 200, 200, 255};
        
        // NightState
        nightConfig.nightDisplayBackground.position.useRelative = false;
        nightConfig.nightDisplayBackground.position.absoluteX = 10.0f;
        nightConfig.nightDisplayBackground.position.absoluteY = 5.0f;
        nightConfig.nightDisplayBackground.width = 65;
        nightConfig.nightDisplayBackground.height = 40;
        
        nightConfig.nightDisplayText.position.useRelative = false;
        nightConfig.nightDisplayText.position.absoluteX = 20.0f;
        nightConfig.nightDisplayText.position.absoluteY = 15.0f;
        nightConfig.nightDisplayText.color = {255, 255, 255, 255};
        
        nightConfig.nightOperationBackground.position.useRelative = false;
        nightConfig.nightOperationBackground.position.absoluteX = 85.0f;
        nightConfig.nightOperationBackground.position.absoluteY = 5.0f;
        nightConfig.nightOperationBackground.width = 205;
        nightConfig.nightOperationBackground.height = 40;
        
        nightConfig.nightOperationText.position.useRelative = false;
        nightConfig.nightOperationText.position.absoluteX = 95.0f;
        nightConfig.nightOperationText.position.absoluteY = 15.0f;
        nightConfig.nightOperationText.color = {255, 255, 255, 255};
        
        nightConfig.messageBoard.background.position.useRelative = false;
        nightConfig.messageBoard.background.position.absoluteX = 190.0f;
        nightConfig.messageBoard.background.position.absoluteY = 480.0f;
        nightConfig.messageBoard.background.width = 720;
        nightConfig.messageBoard.background.height = 100;
        
        nightConfig.messageBoard.text.position.useRelative = false;
        nightConfig.messageBoard.text.position.absoluteX = 210.0f;
        nightConfig.messageBoard.text.position.absoluteY = 500.0f;
        nightConfig.messageBoard.text.color = {255, 255, 255, 255};
        
        // FieldState
        fieldConfig.monsterLevel.position.useRelative = false;
        fieldConfig.monsterLevel.position.absoluteX = 0.0f;  // 相対位置で計算される
        fieldConfig.monsterLevel.position.absoluteY = -10.0f;  // 相対位置で計算される
        fieldConfig.monsterLevel.color = {255, 255, 255, 255};
    }
    
    bool UIConfigManager::loadConfig(const std::string& filepath) {
        setDefaultValues();  // デフォルト値を設定
        
        std::vector<std::string> candidatePaths;
        candidatePaths.push_back(filepath);
        
        if (filepath.find("../") == 0) {
            candidatePaths.push_back(filepath.substr(3));  // "../"を削除
        } else if (filepath.find("assets/") == 0) {
            candidatePaths.push_back("../" + filepath);  // "../"を追加
        }
        
        std::ifstream file;
        bool fileFound = false;
        std::string foundPath;
        for (const auto& path : candidatePaths) {
            file.open(path);
            if (file.is_open()) {
                // ファイルが空でないかチェック
                file.seekg(0, std::ios::end);
                size_t fileSize = file.tellg();
                file.seekg(0, std::ios::beg);
                
                if (fileSize > 0) {
                    configFilePath = path;
                    foundPath = path;
                    fileFound = true;
                    break;
                } else {
                    file.close();
                    printf("UI Config: File is empty: %s\n", path.c_str());
                }
            }
        }
        
        if (!fileFound) {
            printf("UI Config: File not found or empty, using default values: %s\n", filepath.c_str());
            configLoaded = false;
            return false;
        }
        
        // ファイルを再度開く（seekgで位置が変わっているため）
        file.close();
        file.open(foundPath);
        
        if (!file.is_open()) {
            printf("UI Config: Failed to reopen file: %s\n", foundPath.c_str());
            configLoaded = false;
            return false;
        }
        
        try {
            nlohmann::json jsonData;
            file >> jsonData;
            file.close();
            
            // メッセージボード設定
            if (jsonData.contains("messageBoard")) {
                auto& mb = jsonData["messageBoard"];
                
                // 背景
                if (mb.contains("background")) {
                    auto& bg = mb["background"];
                    if (bg.contains("position")) {
                        auto& pos = bg["position"];
                        if (pos.contains("absoluteX")) messageBoardConfig.background.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) messageBoardConfig.background.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) messageBoardConfig.background.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) messageBoardConfig.background.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) messageBoardConfig.background.position.useRelative = pos["useRelative"];
                    }
                    if (bg.contains("width")) messageBoardConfig.background.width = bg["width"];
                    if (bg.contains("height")) messageBoardConfig.background.height = bg["height"];
                }
                
                // テキスト
                if (mb.contains("text")) {
                    auto& txt = mb["text"];
                    if (txt.contains("position")) {
                        auto& pos = txt["position"];
                        if (pos.contains("absoluteX")) messageBoardConfig.text.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) messageBoardConfig.text.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) messageBoardConfig.text.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) messageBoardConfig.text.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) messageBoardConfig.text.position.useRelative = pos["useRelative"];
                    }
                    if (txt.contains("color")) {
                        auto& col = txt["color"];
                        if (col.is_array() && col.size() >= 4) {
                            messageBoardConfig.text.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            messageBoardConfig.text.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
                // メッセージボードの背景色とボーダー色
                if (mb.contains("backgroundColor")) {
                    auto& col = mb["backgroundColor"];
                    if (col.is_array() && col.size() >= 4) {
                        messageBoardConfig.backgroundColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        messageBoardConfig.backgroundColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
                if (mb.contains("borderColor")) {
                    auto& col = mb["borderColor"];
                    if (col.is_array() && col.size() >= 4) {
                        messageBoardConfig.borderColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        messageBoardConfig.borderColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
            }
            
            // CommonUI設定
            if (jsonData.contains("commonUI")) {
                auto& cui = jsonData["commonUI"];
                
                if (cui.contains("nightTimer")) {
                    auto& nt = cui["nightTimer"];
                    if (nt.contains("position")) {
                        auto& pos = nt["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.nightTimer.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.nightTimer.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.nightTimer.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.nightTimer.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.nightTimer.position.useRelative = pos["useRelative"];
                    }
                    if (nt.contains("width")) commonUIConfig.nightTimer.width = nt["width"];
                    if (nt.contains("height")) commonUIConfig.nightTimer.height = nt["height"];
                }
                
                if (cui.contains("nightTimerText")) {
                    auto& ntt = cui["nightTimerText"];
                    if (ntt.contains("position")) {
                        auto& pos = ntt["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.nightTimerText.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.nightTimerText.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.nightTimerText.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.nightTimerText.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.nightTimerText.position.useRelative = pos["useRelative"];
                    }
                    if (ntt.contains("color")) {
                        auto& col = ntt["color"];
                        if (col.is_array() && col.size() >= 4) {
                            commonUIConfig.nightTimerText.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            commonUIConfig.nightTimerText.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
                
                // 目標レベル
                if (cui.contains("targetLevel")) {
                    auto& tl = cui["targetLevel"];
                    if (tl.contains("position")) {
                        auto& pos = tl["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.targetLevel.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.targetLevel.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.targetLevel.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.targetLevel.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.targetLevel.position.useRelative = pos["useRelative"];
                    }
                    if (tl.contains("width")) commonUIConfig.targetLevel.width = tl["width"];
                    if (tl.contains("height")) commonUIConfig.targetLevel.height = tl["height"];
                }
                
                if (cui.contains("targetLevelText")) {
                    auto& tlt = cui["targetLevelText"];
                    if (tlt.contains("position")) {
                        auto& pos = tlt["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.targetLevelText.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.targetLevelText.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.targetLevelText.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.targetLevelText.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.targetLevelText.position.useRelative = pos["useRelative"];
                    }
                    if (tlt.contains("color")) {
                        auto& col = tlt["color"];
                        if (col.is_array() && col.size() >= 4) {
                            commonUIConfig.targetLevelText.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            commonUIConfig.targetLevelText.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
                
                if (cui.contains("trustLevels")) {
                    auto& tr = cui["trustLevels"];
                    if (tr.contains("position")) {
                        auto& pos = tr["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.trustLevels.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.trustLevels.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.trustLevels.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.trustLevels.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.trustLevels.position.useRelative = pos["useRelative"];
                    }
                    if (tr.contains("width")) commonUIConfig.trustLevels.width = tr["width"];
                    if (tr.contains("height")) commonUIConfig.trustLevels.height = tr["height"];
                }
                
                if (cui.contains("trustLevelsText")) {
                    auto& trt = cui["trustLevelsText"];
                    if (trt.contains("position")) {
                        auto& pos = trt["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.trustLevelsText.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.trustLevelsText.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.trustLevelsText.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.trustLevelsText.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.trustLevelsText.position.useRelative = pos["useRelative"];
                    }
                    if (trt.contains("color")) {
                        auto& col = trt["color"];
                        if (col.is_array() && col.size() >= 4) {
                            commonUIConfig.trustLevelsText.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            commonUIConfig.trustLevelsText.color = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
                
                // ゲームコントローラー状態
                if (cui.contains("gameControllerStatus")) {
                    auto& gcs = cui["gameControllerStatus"];
                    if (gcs.contains("position")) {
                        auto& pos = gcs["position"];
                        if (pos.contains("absoluteX")) commonUIConfig.gameControllerStatus.position.absoluteX = pos["absoluteX"];
                        if (pos.contains("absoluteY")) commonUIConfig.gameControllerStatus.position.absoluteY = pos["absoluteY"];
                        if (pos.contains("offsetX")) commonUIConfig.gameControllerStatus.position.offsetX = pos["offsetX"];
                        if (pos.contains("offsetY")) commonUIConfig.gameControllerStatus.position.offsetY = pos["offsetY"];
                        if (pos.contains("useRelative")) commonUIConfig.gameControllerStatus.position.useRelative = pos["useRelative"];
                    }
                    if (gcs.contains("width")) commonUIConfig.gameControllerStatus.width = gcs["width"];
                    if (gcs.contains("height")) commonUIConfig.gameControllerStatus.height = gcs["height"];
                }
                
                // 目標レベルの色と行間
                if (cui.contains("targetLevelAchievedColor")) {
                    auto& col = cui["targetLevelAchievedColor"];
                    if (col.is_array() && col.size() >= 4) {
                        commonUIConfig.targetLevelAchievedColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        commonUIConfig.targetLevelAchievedColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
                if (cui.contains("targetLevelRemainingColor")) {
                    auto& col = cui["targetLevelRemainingColor"];
                    if (col.is_array() && col.size() >= 4) {
                        commonUIConfig.targetLevelRemainingColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        commonUIConfig.targetLevelRemainingColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
                if (cui.contains("targetLevelLineSpacing")) {
                    commonUIConfig.targetLevelLineSpacing = cui["targetLevelLineSpacing"];
                }
                
                // 信頼度の行間
                if (cui.contains("trustLevelsLineSpacing1")) {
                    commonUIConfig.trustLevelsLineSpacing1 = cui["trustLevelsLineSpacing1"];
                }
                if (cui.contains("trustLevelsLineSpacing2")) {
                    commonUIConfig.trustLevelsLineSpacing2 = cui["trustLevelsLineSpacing2"];
                }
                
                // ゲームコントローラー状態の色
                if (cui.contains("gameControllerStatusColor")) {
                    auto& col = cui["gameControllerStatusColor"];
                    if (col.is_array() && col.size() >= 4) {
                        commonUIConfig.gameControllerStatusColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        commonUIConfig.gameControllerStatusColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
                
                // 背景色とボーダー色
                if (cui.contains("backgroundColor")) {
                    auto& col = cui["backgroundColor"];
                    if (col.is_array() && col.size() >= 4) {
                        commonUIConfig.backgroundColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        commonUIConfig.backgroundColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
                if (cui.contains("borderColor")) {
                    auto& col = cui["borderColor"];
                    if (col.is_array() && col.size() >= 4) {
                        commonUIConfig.borderColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            static_cast<Uint8>(col[3])
                        };
                    } else if (col.is_array() && col.size() >= 3) {
                        commonUIConfig.borderColor = {
                            static_cast<Uint8>(col[0]),
                            static_cast<Uint8>(col[1]),
                            static_cast<Uint8>(col[2]),
                            255
                        };
                    }
                }
                if (cui.contains("backgroundAlpha")) {
                    commonUIConfig.backgroundAlpha = static_cast<Uint8>(cui["backgroundAlpha"]);
                }
            }
            
            auto loadPosition = [](nlohmann::json& pos, UIPosition& uiPos) {
                if (pos.contains("absoluteX")) uiPos.absoluteX = pos["absoluteX"];
                if (pos.contains("absoluteY")) uiPos.absoluteY = pos["absoluteY"];
                if (pos.contains("offsetX")) uiPos.offsetX = pos["offsetX"];
                if (pos.contains("offsetY")) uiPos.offsetY = pos["offsetY"];
                if (pos.contains("useRelative")) uiPos.useRelative = pos["useRelative"];
            };
            
            auto loadColor = [](nlohmann::json& col, SDL_Color& color) {
                if (col.is_array() && col.size() >= 4) {
                    color = {
                        static_cast<Uint8>(col[0]),
                        static_cast<Uint8>(col[1]),
                        static_cast<Uint8>(col[2]),
                        static_cast<Uint8>(col[3])
                    };
                } else if (col.is_array() && col.size() >= 3) {
                    color = {
                        static_cast<Uint8>(col[0]),
                        static_cast<Uint8>(col[1]),
                        static_cast<Uint8>(col[2]),
                        255
                    };
                }
            };
            
            // MainMenuState設定
            if (jsonData.contains("mainMenu")) {
                auto& mm = jsonData["mainMenu"];
                if (mm.contains("title")) {
                    auto& cfg = mm["title"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], mainMenuConfig.title.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], mainMenuConfig.title.color);
                }
                if (mm.contains("playerInfo")) {
                    auto& cfg = mm["playerInfo"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], mainMenuConfig.playerInfo.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], mainMenuConfig.playerInfo.color);
                }
                if (mm.contains("adventureButton")) {
                    auto& cfg = mm["adventureButton"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], mainMenuConfig.adventureButton.position);
                    if (cfg.contains("width")) mainMenuConfig.adventureButton.width = cfg["width"];
                    if (cfg.contains("height")) mainMenuConfig.adventureButton.height = cfg["height"];
                    if (cfg.contains("normalColor")) loadColor(cfg["normalColor"], mainMenuConfig.adventureButton.normalColor);
                    if (cfg.contains("hoverColor")) loadColor(cfg["hoverColor"], mainMenuConfig.adventureButton.hoverColor);
                    if (cfg.contains("pressedColor")) loadColor(cfg["pressedColor"], mainMenuConfig.adventureButton.pressedColor);
                    if (cfg.contains("textColor")) loadColor(cfg["textColor"], mainMenuConfig.adventureButton.textColor);
                }
                if (mm.contains("startGameText")) {
                    auto& cfg = mm["startGameText"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], mainMenuConfig.startGameText.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], mainMenuConfig.startGameText.color);
                }
            }
            
            // BattleState設定
            if (jsonData.contains("battle")) {
                auto& bt = jsonData["battle"];
                if (bt.contains("battleLog")) {
                    auto& cfg = bt["battleLog"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.battleLog.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.battleLog.color);
                }
                if (bt.contains("playerStatus")) {
                    auto& cfg = bt["playerStatus"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.playerStatus.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.playerStatus.color);
                }
                if (bt.contains("enemyStatus")) {
                    auto& cfg = bt["enemyStatus"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.enemyStatus.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.enemyStatus.color);
                }
                if (bt.contains("message")) {
                    auto& cfg = bt["message"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.message.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.message.color);
                }
                if (bt.contains("playerHp")) {
                    auto& cfg = bt["playerHp"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.playerHp.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.playerHp.color);
                }
                if (bt.contains("playerMp")) {
                    auto& cfg = bt["playerMp"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.playerMp.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.playerMp.color);
                }
                if (bt.contains("enemyHp")) {
                    auto& cfg = bt["enemyHp"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.enemyHp.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.enemyHp.color);
                }
                if (bt.contains("enemyPosition")) {
                    auto& cfg = bt["enemyPosition"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.enemyPosition);
                    if (cfg.contains("width")) battleConfig.enemyWidth = cfg["width"];
                    if (cfg.contains("height")) battleConfig.enemyHeight = cfg["height"];
                }
                if (bt.contains("playerPosition")) {
                    auto& cfg = bt["playerPosition"];
                    if (cfg.contains("position")) {
                        loadPosition(cfg["position"], battleConfig.playerPosition);
                        printf("UI Config: Loaded playerPosition: absoluteX=%.0f, absoluteY=%.0f, useRelative=%s\n",
                               battleConfig.playerPosition.absoluteX,
                               battleConfig.playerPosition.absoluteY,
                               battleConfig.playerPosition.useRelative ? "true" : "false");
                    }
                    if (cfg.contains("width")) battleConfig.playerWidth = cfg["width"];
                    if (cfg.contains("height")) battleConfig.playerHeight = cfg["height"];
                }
                if (bt.contains("explanationMessageBoard")) {
                    auto& mb = bt["explanationMessageBoard"];
                    if (mb.contains("background")) {
                        auto& bg = mb["background"];
                        if (bg.contains("position")) loadPosition(bg["position"], battleConfig.explanationMessageBoard.background.position);
                        if (bg.contains("width")) battleConfig.explanationMessageBoard.background.width = bg["width"];
                        if (bg.contains("height")) battleConfig.explanationMessageBoard.background.height = bg["height"];
                    }
                    if (mb.contains("text")) {
                        auto& txt = mb["text"];
                        if (txt.contains("position")) loadPosition(txt["position"], battleConfig.explanationMessageBoard.text.position);
                        if (txt.contains("color")) loadColor(txt["color"], battleConfig.explanationMessageBoard.text.color);
                    }
                    // メッセージボードの背景色とボーダー色
                    if (mb.contains("backgroundColor")) {
                        auto& col = mb["backgroundColor"];
                        if (col.is_array() && col.size() >= 4) {
                            battleConfig.explanationMessageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            battleConfig.explanationMessageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                    if (mb.contains("borderColor")) {
                        auto& col = mb["borderColor"];
                        if (col.is_array() && col.size() >= 4) {
                            battleConfig.explanationMessageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            battleConfig.explanationMessageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
                
                // プレイヤー名の設定
                if (bt.contains("playerName")) {
                    auto& cfg = bt["playerName"];
                    if (cfg.contains("offsetX")) battleConfig.playerName.offsetX = cfg["offsetX"];
                    if (cfg.contains("offsetY")) battleConfig.playerName.offsetY = cfg["offsetY"];
                    if (cfg.contains("color")) loadColor(cfg["color"], battleConfig.playerName.color);
                }
                
                // 体力バーの設定
                if (bt.contains("healthBar")) {
                    auto& cfg = bt["healthBar"];
                    if (cfg.contains("offsetX")) battleConfig.healthBar.offsetX = cfg["offsetX"];
                    if (cfg.contains("offsetY")) battleConfig.healthBar.offsetY = cfg["offsetY"];
                    if (cfg.contains("width")) battleConfig.healthBar.width = cfg["width"];
                    if (cfg.contains("height")) battleConfig.healthBar.height = cfg["height"];
                    if (cfg.contains("barColor")) loadColor(cfg["barColor"], battleConfig.healthBar.barColor);
                    if (cfg.contains("bgColor")) loadColor(cfg["bgColor"], battleConfig.healthBar.bgColor);
                    if (cfg.contains("borderColor")) loadColor(cfg["borderColor"], battleConfig.healthBar.borderColor);
                }
                
                // コマンド選択UIの設定
                if (bt.contains("commandSelection")) {
                    auto& cfg = bt["commandSelection"];
                    if (cfg.contains("selectedCommandOffsetY")) battleConfig.commandSelection.selectedCommandOffsetY = cfg["selectedCommandOffsetY"];
                    if (cfg.contains("selectedCommandImageSize")) battleConfig.commandSelection.selectedCommandImageSize = cfg["selectedCommandImageSize"];
                    if (cfg.contains("imageSpacing")) battleConfig.commandSelection.imageSpacing = cfg["imageSpacing"];
                    if (cfg.contains("arrowSpacing")) battleConfig.commandSelection.arrowSpacing = cfg["arrowSpacing"];
                    if (cfg.contains("buttonBaseOffsetY")) battleConfig.commandSelection.buttonBaseOffsetY = cfg["buttonBaseOffsetY"];
                    if (cfg.contains("buttonWidth")) battleConfig.commandSelection.buttonWidth = cfg["buttonWidth"];
                    if (cfg.contains("buttonHeight")) battleConfig.commandSelection.buttonHeight = cfg["buttonHeight"];
                    if (cfg.contains("buttonSpacing")) battleConfig.commandSelection.buttonSpacing = cfg["buttonSpacing"];
                    if (cfg.contains("buttonImageSize")) battleConfig.commandSelection.buttonImageSize = cfg["buttonImageSize"];
                    if (cfg.contains("selectedBgColor")) loadColor(cfg["selectedBgColor"], battleConfig.commandSelection.selectedBgColor);
                    if (cfg.contains("unselectedBgColor")) loadColor(cfg["unselectedBgColor"], battleConfig.commandSelection.unselectedBgColor);
                    if (cfg.contains("selectedBorderColor")) loadColor(cfg["selectedBorderColor"], battleConfig.commandSelection.selectedBorderColor);
                    if (cfg.contains("unselectedBorderColor")) loadColor(cfg["unselectedBorderColor"], battleConfig.commandSelection.unselectedBorderColor);
                    if (cfg.contains("selectedTextColor")) loadColor(cfg["selectedTextColor"], battleConfig.commandSelection.selectedTextColor);
                    if (cfg.contains("unselectedTextColor")) loadColor(cfg["unselectedTextColor"], battleConfig.commandSelection.unselectedTextColor);
                    if (cfg.contains("arrowColor")) loadColor(cfg["arrowColor"], battleConfig.commandSelection.arrowColor);
                }
                
                // 三すくみ画像の設定
                if (bt.contains("rockPaperScissors")) {
                    auto& cfg = bt["rockPaperScissors"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], battleConfig.rockPaperScissors.position);
                    if (cfg.contains("width")) battleConfig.rockPaperScissors.width = cfg["width"];
                }
                
                // ジャッジ結果フェーズのUI設定
                if (bt.contains("judgeResult")) {
                    auto& jr = bt["judgeResult"];
                    
                    // メイン結果テキスト
                    if (jr.contains("resultText")) {
                        auto& rt = jr["resultText"];
                        if (rt.contains("position")) loadPosition(rt["position"], battleConfig.judgeResult.resultText.position);
                        if (rt.contains("offsetY")) battleConfig.judgeResult.resultText.offsetY = rt["offsetY"];
                        if (rt.contains("baseWidth")) battleConfig.judgeResult.resultText.baseWidth = rt["baseWidth"];
                        if (rt.contains("baseHeight")) battleConfig.judgeResult.resultText.baseHeight = rt["baseHeight"];
                        if (rt.contains("victoryColor")) loadColor(rt["victoryColor"], battleConfig.judgeResult.resultText.victoryColor);
                        if (rt.contains("defeatColor")) loadColor(rt["defeatColor"], battleConfig.judgeResult.resultText.defeatColor);
                        if (rt.contains("drawColor")) loadColor(rt["drawColor"], battleConfig.judgeResult.resultText.drawColor);
                        if (rt.contains("desperateVictoryColor")) loadColor(rt["desperateVictoryColor"], battleConfig.judgeResult.resultText.desperateVictoryColor);
                        if (rt.contains("desperateDefeatColor")) loadColor(rt["desperateDefeatColor"], battleConfig.judgeResult.resultText.desperateDefeatColor);
                    }
                    
                    // 3連勝テキスト
                    if (jr.contains("threeWinStreak")) {
                        auto& tws = jr["threeWinStreak"];
                        if (tws.contains("position")) loadPosition(tws["position"], battleConfig.judgeResult.threeWinStreak.position);
                        if (tws.contains("offsetY")) battleConfig.judgeResult.threeWinStreak.offsetY = tws["offsetY"];
                        if (tws.contains("baseWidth")) battleConfig.judgeResult.threeWinStreak.baseWidth = tws["baseWidth"];
                        if (tws.contains("baseHeight")) battleConfig.judgeResult.threeWinStreak.baseHeight = tws["baseHeight"];
                        if (tws.contains("color")) loadColor(tws["color"], battleConfig.judgeResult.threeWinStreak.color);
                        if (tws.contains("format")) battleConfig.judgeResult.threeWinStreak.format = tws["format"];
                    }
                    
                    // ダメージボーナステキスト
                    if (jr.contains("damageBonus")) {
                        auto& db = jr["damageBonus"];
                        if (db.contains("position")) loadPosition(db["position"], battleConfig.judgeResult.damageBonus.position);
                        if (db.contains("offsetY")) battleConfig.judgeResult.damageBonus.offsetY = db["offsetY"];
                        if (db.contains("offsetX")) battleConfig.judgeResult.damageBonus.offsetX = db["offsetX"];
                        if (db.contains("color")) loadColor(db["color"], battleConfig.judgeResult.damageBonus.color);
                        if (db.contains("format")) battleConfig.judgeResult.damageBonus.format = db["format"];
                    }
                }
                
                // 勝敗UIの設定
                if (bt.contains("winLossUI")) {
                    auto& wl = bt["winLossUI"];
                    
                    // 「自分 X勝 敵 Y勝」テキスト
                    if (wl.contains("winLossText")) {
                        auto& wlt = wl["winLossText"];
                        if (wlt.contains("position")) loadPosition(wlt["position"], battleConfig.winLossUI.winLossText.position);
                        // offsetYはposition.offsetYとして読み込まれるため、直接読み込みは不要
                        if (wlt.contains("color")) loadColor(wlt["color"], battleConfig.winLossUI.winLossText.color);
                        if (wlt.contains("padding")) battleConfig.winLossUI.winLossText.padding = wlt["padding"];
                        if (wlt.contains("format")) battleConfig.winLossUI.winLossText.format = wlt["format"];
                    }
                    
                    // 「〜ターン分の攻撃を実行」テキスト
                    if (wl.contains("totalAttackText")) {
                        auto& tat = wl["totalAttackText"];
                        if (tat.contains("position")) loadPosition(tat["position"], battleConfig.winLossUI.totalAttackText.position);
                        // offsetYはposition.offsetYとして読み込まれるため、直接読み込みは不要
                        if (tat.contains("color")) loadColor(tat["color"], battleConfig.winLossUI.totalAttackText.color);
                        if (tat.contains("padding")) battleConfig.winLossUI.totalAttackText.padding = tat["padding"];
                        if (tat.contains("playerWinFormat")) battleConfig.winLossUI.totalAttackText.playerWinFormat = tat["playerWinFormat"];
                        if (tat.contains("enemyWinFormat")) battleConfig.winLossUI.totalAttackText.enemyWinFormat = tat["enemyWinFormat"];
                        if (tat.contains("drawFormat")) battleConfig.winLossUI.totalAttackText.drawFormat = tat["drawFormat"];
                        if (tat.contains("hesitateFormat")) battleConfig.winLossUI.totalAttackText.hesitateFormat = tat["hesitateFormat"];
                    }
                    
                    // 「〜のアタック！」などのテキスト
                    if (wl.contains("attackText")) {
                        auto& at = wl["attackText"];
                        if (at.contains("position")) loadPosition(at["position"], battleConfig.winLossUI.attackText.position);
                        // offsetYはposition.offsetYとして読み込まれるため、直接読み込みは不要
                        if (at.contains("color")) loadColor(at["color"], battleConfig.winLossUI.attackText.color);
                        if (at.contains("padding")) battleConfig.winLossUI.attackText.padding = at["padding"];
                        if (at.contains("attackFormat")) battleConfig.winLossUI.attackText.attackFormat = at["attackFormat"];
                        if (at.contains("rushFormat")) battleConfig.winLossUI.attackText.rushFormat = at["rushFormat"];
                        if (at.contains("statusUpSpellFormat")) battleConfig.winLossUI.attackText.statusUpSpellFormat = at["statusUpSpellFormat"];
                        if (at.contains("healSpellFormat")) battleConfig.winLossUI.attackText.healSpellFormat = at["healSpellFormat"];
                        if (at.contains("attackSpellFormat")) battleConfig.winLossUI.attackText.attackSpellFormat = at["attackSpellFormat"];
                        if (at.contains("defaultSpellFormat")) battleConfig.winLossUI.attackText.defaultSpellFormat = at["defaultSpellFormat"];
                        if (at.contains("defaultAttackFormat")) battleConfig.winLossUI.attackText.defaultAttackFormat = at["defaultAttackFormat"];
                    }
                }
                
                // コマンド選択ヒントテキストの設定
                if (bt.contains("commandHint")) {
                    auto& ch = bt["commandHint"];
                    if (ch.contains("position")) loadPosition(ch["position"], battleConfig.commandHint.position);
                    if (ch.contains("offsetY")) battleConfig.commandHint.offsetY = ch["offsetY"];
                    if (ch.contains("offsetX")) battleConfig.commandHint.offsetX = ch["offsetX"];
                    if (ch.contains("color")) loadColor(ch["color"], battleConfig.commandHint.color);
                    if (ch.contains("padding")) battleConfig.commandHint.padding = ch["padding"];
                    if (ch.contains("normalText")) battleConfig.commandHint.normalText = ch["normalText"];
                    if (ch.contains("residentText")) battleConfig.commandHint.residentText = ch["residentText"];
                }
                
                // ジャッジフェーズの結果表示UI設定
                if (bt.contains("judgePhase")) {
                    auto& jp = bt["judgePhase"];
                    
                    // 位置設定
                    if (jp.contains("position")) {
                        loadPosition(jp["position"], battleConfig.judgePhase.position);
                    }
                    
                    // 勝利時の設定
                    if (jp.contains("win")) {
                        auto& win = jp["win"];
                        if (win.contains("text")) battleConfig.judgePhase.win.text = win["text"];
                        if (win.contains("color")) loadColor(win["color"], battleConfig.judgePhase.win.color);
                    }
                    
                    // 敗北時の設定
                    if (jp.contains("lose")) {
                        auto& lose = jp["lose"];
                        if (lose.contains("text")) battleConfig.judgePhase.lose.text = lose["text"];
                        if (lose.contains("color")) loadColor(lose["color"], battleConfig.judgePhase.lose.color);
                    }
                    
                    // 引き分け時の設定
                    if (jp.contains("draw")) {
                        auto& draw = jp["draw"];
                        if (draw.contains("text")) battleConfig.judgePhase.draw.text = draw["text"];
                        if (draw.contains("color")) loadColor(draw["color"], battleConfig.judgePhase.draw.color);
                    }
                    
                    // その他の設定
                    if (jp.contains("baseWidth")) battleConfig.judgePhase.baseWidth = jp["baseWidth"];
                    if (jp.contains("baseHeight")) battleConfig.judgePhase.baseHeight = jp["baseHeight"];
                    if (jp.contains("backgroundPadding")) battleConfig.judgePhase.backgroundPadding = jp["backgroundPadding"];
                    if (jp.contains("glowColor")) loadColor(jp["glowColor"], battleConfig.judgePhase.glowColor);
                }
                
                // ステータス上昇呪文の攻撃倍率表示UI設定
                if (bt.contains("attackMultiplier")) {
                    auto& am = bt["attackMultiplier"];
                    if (am.contains("position")) loadPosition(am["position"], battleConfig.attackMultiplier.position);
                    if (am.contains("offsetX")) battleConfig.attackMultiplier.offsetX = am["offsetX"];
                    if (am.contains("offsetY")) battleConfig.attackMultiplier.offsetY = am["offsetY"];
                    if (am.contains("textColor")) loadColor(am["textColor"], battleConfig.attackMultiplier.textColor);
                    if (am.contains("bgColor")) loadColor(am["bgColor"], battleConfig.attackMultiplier.bgColor);
                    if (am.contains("borderColor")) loadColor(am["borderColor"], battleConfig.attackMultiplier.borderColor);
                    if (am.contains("padding")) battleConfig.attackMultiplier.padding = am["padding"];
                    if (am.contains("format")) battleConfig.attackMultiplier.format = am["format"];
                }
            }
            
            // RoomState設定
            if (jsonData.contains("room")) {
                auto& rm = jsonData["room"];
                if (rm.contains("messageBoard")) {
                    auto& mb = rm["messageBoard"];
                    if (mb.contains("background")) {
                        auto& bg = mb["background"];
                        if (bg.contains("position")) loadPosition(bg["position"], roomConfig.messageBoard.background.position);
                        if (bg.contains("width")) roomConfig.messageBoard.background.width = bg["width"];
                        if (bg.contains("height")) roomConfig.messageBoard.background.height = bg["height"];
                    }
                    if (mb.contains("text")) {
                        auto& txt = mb["text"];
                        if (txt.contains("position")) loadPosition(txt["position"], roomConfig.messageBoard.text.position);
                        if (txt.contains("color")) loadColor(txt["color"], roomConfig.messageBoard.text.color);
                    }
                    // メッセージボードの背景色とボーダー色
                    if (mb.contains("backgroundColor")) {
                        auto& col = mb["backgroundColor"];
                        if (col.is_array() && col.size() >= 4) {
                            roomConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            roomConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                    if (mb.contains("borderColor")) {
                        auto& col = mb["borderColor"];
                        if (col.is_array() && col.size() >= 4) {
                            roomConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            roomConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
                if (rm.contains("howToOperateBackground")) {
                    auto& bg = rm["howToOperateBackground"];
                    if (bg.contains("position")) loadPosition(bg["position"], roomConfig.howToOperateBackground.position);
                    if (bg.contains("width")) roomConfig.howToOperateBackground.width = bg["width"];
                    if (bg.contains("height")) roomConfig.howToOperateBackground.height = bg["height"];
                }
                if (rm.contains("howToOperateText")) {
                    auto& txt = rm["howToOperateText"];
                    if (txt.contains("position")) loadPosition(txt["position"], roomConfig.howToOperateText.position);
                    if (txt.contains("color")) loadColor(txt["color"], roomConfig.howToOperateText.color);
                }
            }
            
            // TownState設定
            if (jsonData.contains("town")) {
                auto& tn = jsonData["town"];
                if (tn.contains("playerInfo")) {
                    auto& cfg = tn["playerInfo"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], townConfig.playerInfo.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], townConfig.playerInfo.color);
                }
                if (tn.contains("controls")) {
                    auto& cfg = tn["controls"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], townConfig.controls.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], townConfig.controls.color);
                }
            }
            
            // CastleState設定
            if (jsonData.contains("castle")) {
                auto& cs = jsonData["castle"];
                if (cs.contains("messageBoard")) {
                    auto& mb = cs["messageBoard"];
                    if (mb.contains("background")) {
                        auto& bg = mb["background"];
                        if (bg.contains("position")) loadPosition(bg["position"], castleConfig.messageBoard.background.position);
                        if (bg.contains("width")) castleConfig.messageBoard.background.width = bg["width"];
                        if (bg.contains("height")) castleConfig.messageBoard.background.height = bg["height"];
                    }
                    if (mb.contains("text")) {
                        auto& txt = mb["text"];
                        if (txt.contains("position")) loadPosition(txt["position"], castleConfig.messageBoard.text.position);
                        if (txt.contains("color")) loadColor(txt["color"], castleConfig.messageBoard.text.color);
                    }
                    // メッセージボードの背景色とボーダー色
                    if (mb.contains("backgroundColor")) {
                        auto& col = mb["backgroundColor"];
                        if (col.is_array() && col.size() >= 4) {
                            castleConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            castleConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                    if (mb.contains("borderColor")) {
                        auto& col = mb["borderColor"];
                        if (col.is_array() && col.size() >= 4) {
                            castleConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            castleConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
            }
            
            // DemonCastleState設定
            if (jsonData.contains("demonCastle")) {
                auto& dc = jsonData["demonCastle"];
                if (dc.contains("messageBoard")) {
                    auto& mb = dc["messageBoard"];
                    if (mb.contains("background")) {
                        auto& bg = mb["background"];
                        if (bg.contains("position")) loadPosition(bg["position"], demonCastleConfig.messageBoard.background.position);
                        if (bg.contains("width")) demonCastleConfig.messageBoard.background.width = bg["width"];
                        if (bg.contains("height")) demonCastleConfig.messageBoard.background.height = bg["height"];
                    }
                    if (mb.contains("text")) {
                        auto& txt = mb["text"];
                        if (txt.contains("position")) loadPosition(txt["position"], demonCastleConfig.messageBoard.text.position);
                        if (txt.contains("color")) loadColor(txt["color"], demonCastleConfig.messageBoard.text.color);
                    }
                    // メッセージボードの背景色とボーダー色
                    if (mb.contains("backgroundColor")) {
                        auto& col = mb["backgroundColor"];
                        if (col.is_array() && col.size() >= 4) {
                            demonCastleConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            demonCastleConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                    if (mb.contains("borderColor")) {
                        auto& col = mb["borderColor"];
                        if (col.is_array() && col.size() >= 4) {
                            demonCastleConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            demonCastleConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
            }
            
            // GameOverState設定
            if (jsonData.contains("gameOver")) {
                auto& go = jsonData["gameOver"];
                if (go.contains("title")) {
                    auto& cfg = go["title"];
                    if (cfg.contains("text")) {
                        auto& textCfg = cfg["text"];
                        if (textCfg.contains("position")) loadPosition(textCfg["position"], gameOverConfig.title.text.position);
                        if (textCfg.contains("color")) loadColor(textCfg["color"], gameOverConfig.title.text.color);
                    }
                    if (cfg.contains("background")) {
                        auto& bgCfg = cfg["background"];
                        if (bgCfg.contains("position")) loadPosition(bgCfg["position"], gameOverConfig.title.background.position);
                        if (bgCfg.contains("width")) gameOverConfig.title.background.width = bgCfg["width"].get<int>();
                        if (bgCfg.contains("height")) gameOverConfig.title.background.height = bgCfg["height"].get<int>();
                    }
                    if (cfg.contains("backgroundColor")) loadColor(cfg["backgroundColor"], gameOverConfig.title.backgroundColor);
                    if (cfg.contains("borderColor")) loadColor(cfg["borderColor"], gameOverConfig.title.borderColor);
                }
                if (go.contains("reason")) {
                    auto& cfg = go["reason"];
                    if (cfg.contains("text")) {
                        auto& textCfg = cfg["text"];
                        if (textCfg.contains("position")) loadPosition(textCfg["position"], gameOverConfig.reason.text.position);
                        if (textCfg.contains("color")) loadColor(textCfg["color"], gameOverConfig.reason.text.color);
                    }
                    if (cfg.contains("background")) {
                        auto& bgCfg = cfg["background"];
                        if (bgCfg.contains("position")) loadPosition(bgCfg["position"], gameOverConfig.reason.background.position);
                        if (bgCfg.contains("width")) gameOverConfig.reason.background.width = bgCfg["width"].get<int>();
                        if (bgCfg.contains("height")) gameOverConfig.reason.background.height = bgCfg["height"].get<int>();
                    }
                    if (cfg.contains("backgroundColor")) loadColor(cfg["backgroundColor"], gameOverConfig.reason.backgroundColor);
                    if (cfg.contains("borderColor")) loadColor(cfg["borderColor"], gameOverConfig.reason.borderColor);
                }
                if (go.contains("instruction")) {
                    auto& cfg = go["instruction"];
                    if (cfg.contains("text")) {
                        auto& textCfg = cfg["text"];
                        if (textCfg.contains("position")) loadPosition(textCfg["position"], gameOverConfig.instruction.text.position);
                        if (textCfg.contains("color")) loadColor(textCfg["color"], gameOverConfig.instruction.text.color);
                    }
                    if (cfg.contains("background")) {
                        auto& bgCfg = cfg["background"];
                        if (bgCfg.contains("position")) loadPosition(bgCfg["position"], gameOverConfig.instruction.background.position);
                        if (bgCfg.contains("width")) gameOverConfig.instruction.background.width = bgCfg["width"].get<int>();
                        if (bgCfg.contains("height")) gameOverConfig.instruction.background.height = bgCfg["height"].get<int>();
                    }
                    if (cfg.contains("backgroundColor")) loadColor(cfg["backgroundColor"], gameOverConfig.instruction.backgroundColor);
                    if (cfg.contains("borderColor")) loadColor(cfg["borderColor"], gameOverConfig.instruction.borderColor);
                }
                if (go.contains("image")) {
                    auto& cfg = go["image"];
                    if (cfg.contains("baseSize")) gameOverConfig.image.baseSize = cfg["baseSize"].get<int>();
                }
            }
            
            // EndingState設定
            if (jsonData.contains("ending")) {
                auto& en = jsonData["ending"];
                if (en.contains("message")) {
                    auto& cfg = en["message"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], endingConfig.message.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], endingConfig.message.color);
                }
                if (en.contains("staffRoll")) {
                    auto& cfg = en["staffRoll"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], endingConfig.staffRoll.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], endingConfig.staffRoll.color);
                }
                if (en.contains("theEnd")) {
                    auto& cfg = en["theEnd"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], endingConfig.theEnd.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], endingConfig.theEnd.color);
                }
                if (en.contains("returnToMenu")) {
                    auto& cfg = en["returnToMenu"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], endingConfig.returnToMenu.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], endingConfig.returnToMenu.color);
                }
            }
            
            // NightState設定
            if (jsonData.contains("night")) {
                auto& nt = jsonData["night"];
                if (nt.contains("nightDisplayBackground")) {
                    auto& cfg = nt["nightDisplayBackground"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], nightConfig.nightDisplayBackground.position);
                    if (cfg.contains("width")) nightConfig.nightDisplayBackground.width = cfg["width"];
                    if (cfg.contains("height")) nightConfig.nightDisplayBackground.height = cfg["height"];
                }
                if (nt.contains("nightDisplayText")) {
                    auto& cfg = nt["nightDisplayText"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], nightConfig.nightDisplayText.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], nightConfig.nightDisplayText.color);
                }
                if (nt.contains("nightOperationBackground")) {
                    auto& cfg = nt["nightOperationBackground"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], nightConfig.nightOperationBackground.position);
                    if (cfg.contains("width")) nightConfig.nightOperationBackground.width = cfg["width"];
                    if (cfg.contains("height")) nightConfig.nightOperationBackground.height = cfg["height"];
                }
                if (nt.contains("nightOperationText")) {
                    auto& cfg = nt["nightOperationText"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], nightConfig.nightOperationText.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], nightConfig.nightOperationText.color);
                }
                if (nt.contains("messageBoard")) {
                    auto& mb = nt["messageBoard"];
                    if (mb.contains("background")) {
                        auto& bg = mb["background"];
                        if (bg.contains("position")) loadPosition(bg["position"], nightConfig.messageBoard.background.position);
                        if (bg.contains("width")) nightConfig.messageBoard.background.width = bg["width"];
                        if (bg.contains("height")) nightConfig.messageBoard.background.height = bg["height"];
                    }
                    if (mb.contains("text")) {
                        auto& txt = mb["text"];
                        if (txt.contains("position")) loadPosition(txt["position"], nightConfig.messageBoard.text.position);
                        if (txt.contains("color")) loadColor(txt["color"], nightConfig.messageBoard.text.color);
                    }
                    // メッセージボードの背景色とボーダー色
                    if (mb.contains("backgroundColor")) {
                        auto& col = mb["backgroundColor"];
                        if (col.is_array() && col.size() >= 4) {
                            nightConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            nightConfig.messageBoard.backgroundColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                    if (mb.contains("borderColor")) {
                        auto& col = mb["borderColor"];
                        if (col.is_array() && col.size() >= 4) {
                            nightConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                static_cast<Uint8>(col[3])
                            };
                        } else if (col.is_array() && col.size() >= 3) {
                            nightConfig.messageBoard.borderColor = {
                                static_cast<Uint8>(col[0]),
                                static_cast<Uint8>(col[1]),
                                static_cast<Uint8>(col[2]),
                                255
                            };
                        }
                    }
                }
            }
            
            // FieldState設定
            if (jsonData.contains("field")) {
                auto& fd = jsonData["field"];
                if (fd.contains("monsterLevel")) {
                    auto& cfg = fd["monsterLevel"];
                    if (cfg.contains("position")) loadPosition(cfg["position"], fieldConfig.monsterLevel.position);
                    if (cfg.contains("color")) loadColor(cfg["color"], fieldConfig.monsterLevel.color);
                }
            }
            
            configLoaded = true;
            
            lastFileModificationTime = getFileModificationTime(configFilePath);
            
            printf("UI Config: Loaded successfully from %s\n", configFilePath.c_str());
            return true;
            
        } catch (const std::exception& e) {
            printf("UI Config: Error loading config: %s\n", e.what());
            configLoaded = false;
            return false;
        }
    }
    
    void UIConfigManager::reloadConfig() {
        // 常に元のファイル（../assets/config/ui_config.json）を優先的に読み込む
        std::string originalPath = "../assets/config/ui_config.json";
        std::string buildPath = "assets/config/ui_config.json";
        
        time_t originalModTime = getFileModificationTime(originalPath);
        time_t buildModTime = getFileModificationTime(buildPath);
        
        std::string pathToLoad;
        if (originalModTime > 0) {
            // 元のファイルを優先
            pathToLoad = originalPath;
        } else if (buildModTime > 0) {
            // 元のファイルが存在しない場合のみ、build内のファイルを使用
            pathToLoad = buildPath;
        } else {
            // どちらも見つからない場合は、既存のconfigFilePathを使用
            if (!configFilePath.empty()) {
                loadConfig(configFilePath);
            }
            return;
        }
        
        configFilePath = pathToLoad;
        loadConfig(pathToLoad);
    }
    
    time_t UIConfigManager::getFileModificationTime(const std::string& filepath) const {
#ifdef _WIN32
        struct _stat fileInfo;
        if (_stat(filepath.c_str(), &fileInfo) == 0) {
            return fileInfo.st_mtime;
        }
        // 代替パスも試す
        std::string altPath = filepath;
        if (altPath.find("../") == 0) {
            altPath = altPath.substr(3);
        } else if (altPath.find("assets/") == 0) {
            altPath = "../" + altPath;
        }
        if (_stat(altPath.c_str(), &fileInfo) == 0) {
            return fileInfo.st_mtime;
        }
#else
        struct stat fileInfo;
        if (stat(filepath.c_str(), &fileInfo) == 0) {
            return fileInfo.st_mtime;
        }
        // 代替パスも試す
        std::string altPath = filepath;
        if (altPath.find("../") == 0) {
            altPath = altPath.substr(3);
        } else if (altPath.find("assets/") == 0) {
            altPath = "../" + altPath;
        }
        if (stat(altPath.c_str(), &fileInfo) == 0) {
            return fileInfo.st_mtime;
        }
#endif
        return 0;
    }
    
    bool UIConfigManager::checkAndReloadConfig() {
        // 常に元のファイル（../assets/config/ui_config.json）を優先的に監視
        // これにより、編集したファイルが確実にホットリロードされる
        std::string originalPath = "../assets/config/ui_config.json";
        std::string buildPath = "assets/config/ui_config.json";
        
        time_t originalModTime = getFileModificationTime(originalPath);
        time_t buildModTime = getFileModificationTime(buildPath);
        
        time_t currentModTime = 0;
        std::string currentPath;
        
        // 元のファイルを優先（存在する場合）
        if (originalModTime > 0) {
            currentModTime = originalModTime;
            currentPath = originalPath;
        } else if (buildModTime > 0) {
            // 元のファイルが存在しない場合のみ、build内のファイルを使用
            currentModTime = buildModTime;
            currentPath = buildPath;
        } else {
            // どちらも見つからない場合はスキップ
            return false;
        }
        
        // パスが変わった場合は更新
        if (configFilePath != currentPath) {
            configFilePath = currentPath;
            lastFileModificationTime = 0; // 強制的にリロード
        }
        
        if (currentModTime > 0 && currentModTime != lastFileModificationTime && lastFileModificationTime > 0) {
            printf("UI Config: File changed! Reloading from %s...\n", currentPath.c_str());
            lastFileModificationTime = currentModTime;
            reloadConfig();
            printf("UI Config: Reload complete. Player position: absoluteX=%.0f, absoluteY=%.0f\n",
                   getBattleConfig().playerPosition.absoluteX,
                   getBattleConfig().playerPosition.absoluteY);
            return true;
        }
        
        if (lastFileModificationTime == 0 && currentModTime > 0) {
            lastFileModificationTime = currentModTime;
        }
        
        return false;
    }
    
    void UIConfigManager::calculatePosition(int& x, int& y, const UIPosition& pos, int windowWidth, int windowHeight) const {
        if (pos.useRelative) {
            x = static_cast<int>(windowWidth / 2.0f + pos.offsetX);
            y = static_cast<int>(windowHeight / 2.0f + pos.offsetY);
        } else {
            // 絶対位置（画面左上基準）
            x = static_cast<int>(pos.absoluteX);
            y = static_cast<int>(pos.absoluteY);
        }
    }
}

