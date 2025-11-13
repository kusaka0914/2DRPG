#pragma once

#include <SDL.h>
#include <string>
#include <ctime>

namespace UIConfig {
    struct UIPosition {
        float offsetX = 0.0f;      // 画面中央からのオフセットX（相対位置）
        float offsetY = 0.0f;      // 画面中央からのオフセットY（相対位置）
        float absoluteX = 0.0f;    // 絶対位置X（画面左上基準）
        float absoluteY = 0.0f;    // 絶対位置Y（画面左上基準）
        bool useRelative = false;   // true: 相対位置を使用、false: 絶対位置を使用
    };
    
    struct UIRectConfig {
        UIPosition position;
        int width = 0;
        int height = 0;
    };
    
    struct UITextConfig {
        UIPosition position;
        SDL_Color color = {255, 255, 255, 255};
    };
    
    struct UIButtonConfig {
        UIPosition position;
        int width = 0;
        int height = 0;
        SDL_Color normalColor = {64, 64, 64, 255};
        SDL_Color hoverColor = {96, 96, 96, 255};
        SDL_Color pressedColor = {32, 32, 32, 255};
        SDL_Color textColor = {255, 255, 255, 255};
    };
    
    struct UIMessageBoardConfig {
        UIRectConfig background;
        UITextConfig text;
    };
    
    struct UICommonUIConfig {
        UIRectConfig nightTimer;
        UITextConfig nightTimerText;
        UIRectConfig targetLevel;
        UITextConfig targetLevelText;
        UIRectConfig trustLevels;
        UITextConfig trustLevelsText;
        UIRectConfig gameControllerStatus;
    };
    
    // MainMenuState用
    struct UIMainMenuConfig {
        UITextConfig title;
        UITextConfig playerInfo;
        UIButtonConfig adventureButton;
    };
    
    // BattleState用
    struct UIBattleConfig {
        UITextConfig battleLog;
        UITextConfig playerStatus;
        UITextConfig enemyStatus;
        UITextConfig message;
        UITextConfig playerHp;      // プレイヤーHP表示
        UITextConfig playerMp;       // プレイヤーMP表示
        UITextConfig enemyHp;        // 敵HP表示
        UIPosition enemyPosition;    // 敵の画像位置
        int enemyWidth;              // 敵の画像幅
        int enemyHeight;             // 敵の画像高さ
        UIPosition playerPosition;   // プレイヤーの画像位置
        int playerWidth;             // プレイヤーの画像幅
        int playerHeight;            // プレイヤーの画像高さ
    };
    
    // RoomState用
    struct UIRoomConfig {
        UIMessageBoardConfig messageBoard;
        UIRectConfig howToOperateBackground;
        UITextConfig howToOperateText;
    };
    
    // TownState用
    struct UITownConfig {
        UITextConfig playerInfo;
        UITextConfig controls;
    };
    
    // CastleState用
    struct UICastleConfig {
        UIMessageBoardConfig messageBoard;
    };
    
    // DemonCastleState用
    struct UIDemonCastleConfig {
        UIMessageBoardConfig messageBoard;
    };
    
    // GameOverState用
    struct UIGameOverConfig {
        UITextConfig title;
        UITextConfig reason;
        UITextConfig instruction;
    };
    
    // EndingState用
    struct UIEndingConfig {
        UITextConfig message;
        UITextConfig staffRoll;
        UITextConfig theEnd;
        UITextConfig returnToMenu;
    };
    
    // NightState用
    struct UINightConfig {
        UIRectConfig nightDisplayBackground;      // 夜の表示背景
        UIRectConfig nightOperationBackground;    // 夜の操作用背景
        UIMessageBoardConfig messageBoard;         // メッセージボード
    };
    
    // FieldState用
    struct UIFieldConfig {
        UITextConfig monsterLevel;                 // モンスターレベル表示
    };
    
    class UIConfigManager {
    public:
        static UIConfigManager& getInstance();
        
        bool loadConfig(const std::string& filepath = "assets/config/ui_config.json");
        void reloadConfig();
        bool checkAndReloadConfig();  // ファイル変更をチェックして再読み込み
        
        // UI要素の設定を取得
        UIMessageBoardConfig getMessageBoardConfig() const { return messageBoardConfig; }
        UICommonUIConfig getCommonUIConfig() const { return commonUIConfig; }
        UIMainMenuConfig getMainMenuConfig() const { return mainMenuConfig; }
        UIBattleConfig getBattleConfig() const { return battleConfig; }
        UIRoomConfig getRoomConfig() const { return roomConfig; }
        UITownConfig getTownConfig() const { return townConfig; }
        UICastleConfig getCastleConfig() const { return castleConfig; }
        UIDemonCastleConfig getDemonCastleConfig() const { return demonCastleConfig; }
        UIGameOverConfig getGameOverConfig() const { return gameOverConfig; }
        UIEndingConfig getEndingConfig() const { return endingConfig; }
        UINightConfig getNightConfig() const { return nightConfig; }
        UIFieldConfig getFieldConfig() const { return fieldConfig; }
        
        // 位置を計算（ウィンドウサイズを考慮）
        void calculatePosition(int& x, int& y, const UIPosition& pos, int windowWidth, int windowHeight) const;
        
    private:
        UIConfigManager() = default;
        ~UIConfigManager() = default;
        UIConfigManager(const UIConfigManager&) = delete;
        UIConfigManager& operator=(const UIConfigManager&) = delete;
        
        void setDefaultValues();
        time_t getFileModificationTime(const std::string& filepath) const;
        
        std::string configFilePath;
        bool configLoaded = false;
        time_t lastFileModificationTime = 0;  // ファイル監視用
        
        // UI設定
        UIMessageBoardConfig messageBoardConfig;
        UICommonUIConfig commonUIConfig;
        UIMainMenuConfig mainMenuConfig;
        UIBattleConfig battleConfig;
        UIRoomConfig roomConfig;
        UITownConfig townConfig;
        UICastleConfig castleConfig;
        UIDemonCastleConfig demonCastleConfig;
        UIGameOverConfig gameOverConfig;
        UIEndingConfig endingConfig;
        UINightConfig nightConfig;
        UIFieldConfig fieldConfig;
    };
}
