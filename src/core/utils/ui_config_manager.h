/**
 * @file ui_config_manager.h
 * @brief UI設定管理の名前空間
 * @details 各GameStateのUI要素（位置、サイズ、色など）の設定を管理する構造体と関数を定義する。
 */

#pragma once

#include <SDL.h>
#include <string>
#include <ctime>

/**
 * @brief UI設定管理の名前空間
 * @details 各GameStateのUI要素（位置、サイズ、色など）の設定を管理する構造体と関数を定義する。
 */
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
        SDL_Color backgroundColor;    // 背景色（黒）
        SDL_Color borderColor;         // ボーダー色（白）
    };
    
    struct UICommonUIConfig {
        UIRectConfig nightTimer;
        UITextConfig nightTimerText;
        UIRectConfig targetLevel;
        UITextConfig targetLevelText;
        SDL_Color targetLevelAchievedColor;      // 目標達成時の緑色
        SDL_Color targetLevelRemainingColor;     // 残りレベルの黄色
        int targetLevelLineSpacing;              // 行間オフセット
        UIRectConfig trustLevels;
        UITextConfig trustLevelsText;
        int trustLevelsLineSpacing1;             // 2行目のオフセット
        int trustLevelsLineSpacing2;             // 3行目のオフセット
        UIRectConfig gameControllerStatus;
        SDL_Color gameControllerStatusColor;     // ゲームコントローラー状態の緑色
        SDL_Color backgroundColor;                // 背景色（黒）
        SDL_Color borderColor;                    // ボーダー色（白）
        Uint8 backgroundAlpha;                     // 背景の透明度
    };
    
    // MainMenuState用
    struct UIMainMenuConfig {
        UITextConfig title;
        UITextConfig playerInfo;
        UIButtonConfig adventureButton;
        UITextConfig startGameText;               // "START GAME : PRESS ENTER"テキスト
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
        UIMessageBoardConfig explanationMessageBoard;  // 説明用メッセージボード
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
        UITextConfig nightDisplayText;            // 夜の表示テキスト
        UIRectConfig nightOperationBackground;    // 夜の操作用背景
        UITextConfig nightOperationText;           // 夜の操作テキスト
        UIMessageBoardConfig messageBoard;         // メッセージボード
    };
    
    // FieldState用
    struct UIFieldConfig {
        UITextConfig monsterLevel;                 // モンスターレベル表示
    };
    
    /**
     * @brief UI設定管理を担当するクラス
     * @details UI設定の読み込み、保存、取得を管理する。シングルトンパターンを使用。
     */
    class UIConfigManager {
    public:
        /**
         * @brief インスタンスの取得（シングルトン）
         * @return UIConfigManagerへの参照
         */
        static UIConfigManager& getInstance();
        
        /**
         * @brief 設定の読み込み
         * @param filepath 設定ファイルパス（デフォルト: "assets/config/ui_config.json"）
         * @return 読み込みが成功したか
         */
        bool loadConfig(const std::string& filepath = "assets/config/ui_config.json");
        
        /**
         * @brief 設定の再読み込み
         */
        void reloadConfig();
        
        /**
         * @brief ファイル変更をチェックして再読み込み
         * @return 再読み込みが実行されたか
         */
        bool checkAndReloadConfig();
        
        /**
         * @brief メッセージボード設定の取得
         * @return メッセージボード設定
         */
        UIMessageBoardConfig getMessageBoardConfig() const { return messageBoardConfig; }
        
        /**
         * @brief 共通UI設定の取得
         * @return 共通UI設定
         */
        UICommonUIConfig getCommonUIConfig() const { return commonUIConfig; }
        
        /**
         * @brief メインメニュー設定の取得
         * @return メインメニュー設定
         */
        UIMainMenuConfig getMainMenuConfig() const { return mainMenuConfig; }
        
        /**
         * @brief 戦闘設定の取得
         * @return 戦闘設定
         */
        UIBattleConfig getBattleConfig() const { return battleConfig; }
        
        /**
         * @brief 部屋設定の取得
         * @return 部屋設定
         */
        UIRoomConfig getRoomConfig() const { return roomConfig; }
        
        /**
         * @brief 街設定の取得
         * @return 街設定
         */
        UITownConfig getTownConfig() const { return townConfig; }
        
        /**
         * @brief 城設定の取得
         * @return 城設定
         */
        UICastleConfig getCastleConfig() const { return castleConfig; }
        
        /**
         * @brief 魔王の城設定の取得
         * @return 魔王の城設定
         */
        UIDemonCastleConfig getDemonCastleConfig() const { return demonCastleConfig; }
        
        /**
         * @brief ゲームオーバー設定の取得
         * @return ゲームオーバー設定
         */
        UIGameOverConfig getGameOverConfig() const { return gameOverConfig; }
        
        /**
         * @brief エンディング設定の取得
         * @return エンディング設定
         */
        UIEndingConfig getEndingConfig() const { return endingConfig; }
        
        /**
         * @brief 夜設定の取得
         * @return 夜設定
         */
        UINightConfig getNightConfig() const { return nightConfig; }
        
        /**
         * @brief フィールド設定の取得
         * @return フィールド設定
         */
        UIFieldConfig getFieldConfig() const { return fieldConfig; }
        
        /**
         * @brief 位置を計算（ウィンドウサイズを考慮）
         * @param x X座標（参照渡し、計算結果が格納される）
         * @param y Y座標（参照渡し、計算結果が格納される）
         * @param pos UI位置設定
         * @param windowWidth ウィンドウ幅
         * @param windowHeight ウィンドウ高さ
         */
        void calculatePosition(int& x, int& y, const UIPosition& pos, int windowWidth, int windowHeight) const;
        
    private:
        /**
         * @brief コンストラクタ（private、シングルトンパターン）
         */
        UIConfigManager() = default;
        
        /**
         * @brief デストラクタ（private、シングルトンパターン）
         */
        ~UIConfigManager() = default;
        
        /**
         * @brief コピーコンストラクタ（削除、シングルトンパターン）
         */
        UIConfigManager(const UIConfigManager&) = delete;
        
        /**
         * @brief 代入演算子（削除、シングルトンパターン）
         */
        UIConfigManager& operator=(const UIConfigManager&) = delete;
        
        /**
         * @brief デフォルト値の設定
         */
        void setDefaultValues();
        
        /**
         * @brief ファイルの更新時刻の取得
         * @param filepath ファイルパス
         * @return ファイルの更新時刻
         */
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
