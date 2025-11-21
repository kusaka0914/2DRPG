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
        
        // プレイヤー名と体力バーの設定（相対位置オフセット）
        struct {
            float offsetX = -80.0f;      // プレイヤー名のXオフセット（playerXからの相対位置）
            float offsetY = -40.0f;      // プレイヤー名のYオフセット（playerY - playerHeight/2からの相対位置）
            SDL_Color color = {255, 255, 255, 255};  // プレイヤー名の色
        } playerName;
        
        struct {
            float offsetX = 10.0f;       // 体力バーのXオフセット（playerXからの相対位置）
            float offsetY = -40.0f;      // 体力バーのYオフセット（playerY - playerHeight/2からの相対位置）
            int width = 150;              // 体力バーの幅
            int height = 20;              // 体力バーの高さ
            SDL_Color barColor = {150, 255, 150, 255};      // 体力バーの色（黄緑色）
            SDL_Color bgColor = {50, 50, 50, 255};          // 体力バーの背景色（暗いグレー）
            SDL_Color borderColor = {255, 255, 255, 255};   // 体力バーの枠線色（白色）
        } healthBar;
        
        // コマンド選択UIの設定
        struct {
            float selectedCommandOffsetY = -150.0f;  // 選択済みコマンドのYオフセット（centerYからの相対位置）
            int selectedCommandImageSize = 50;       // 選択済みコマンド画像のサイズ（幅）
            int imageSpacing = 60;                   // 画像間のスペース
            int arrowSpacing = 20;                    // 矢印のスペース
            
            float buttonBaseOffsetY = -50.0f;        // コマンドボタンのベースYオフセット（centerYからの相対位置）
            int buttonWidth = 200;                   // ボタンの幅
            int buttonHeight = 60;                   // ボタンの高さ
            int buttonSpacing = 80;                  // ボタン間のスペース
            int buttonImageSize = 60;                 // ボタン内のコマンド画像のサイズ（幅）
            
            SDL_Color selectedBgColor = {100, 200, 255, 200};    // 選択中のボタンの背景色
            SDL_Color unselectedBgColor = {50, 50, 50, 150};     // 未選択のボタンの背景色
            SDL_Color selectedBorderColor = {255, 215, 0, 255};   // 選択中のボタンの枠線色
            SDL_Color unselectedBorderColor = {150, 150, 150, 255}; // 未選択のボタンの枠線色
            SDL_Color selectedTextColor = {255, 255, 255, 255};   // 選択中のボタンのテキスト色
            SDL_Color unselectedTextColor = {200, 200, 200, 255}; // 未選択のボタンのテキスト色
            SDL_Color arrowColor = {255, 255, 255, 255};          // 矢印の色
        } commandSelection;
        
        // 三すくみ画像の設定
        struct {
            UIPosition position;          // 三すくみ画像の位置
            int width = 200;              // 三すくみ画像の幅
        } rockPaperScissors;
        
        // ジャッジ結果フェーズのUI設定
        struct {
            // メイン結果テキスト（勝利/敗北/引き分け）
            struct {
                UIPosition position;      // 位置（centerYからの相対位置）
                float offsetY = -100.0f;   // Yオフセット（centerYからの相対位置）
                int baseWidth = 400;      // ベース幅
                int baseHeight = 100;     // ベース高さ
                
                struct {
                    SDL_Color textColor = {255, 255, 255, 255};      // 勝利時のテキスト色（白色）
                    SDL_Color backgroundColor = {255, 215, 0, 255};  // 勝利時の背景色（金色）
                } victory;
                
                struct {
                    SDL_Color textColor = {255, 255, 255, 255};      // 敗北時のテキスト色（白色）
                    SDL_Color backgroundColor = {255, 0, 0, 255};    // 敗北時の背景色（赤色）
                } defeat;
                
                struct {
                    SDL_Color textColor = {255, 255, 255, 255};      // 引き分け時のテキスト色（白色）
                    SDL_Color backgroundColor = {200, 200, 200, 255}; // 引き分け時の背景色（グレー）
                } draw;
                
                struct {
                    SDL_Color textColor = {255, 255, 255, 255};      // 一発逆転成功時のテキスト色（白色）
                    SDL_Color backgroundColor = {255, 215, 0, 255};  // 一発逆転成功時の背景色（金色）
                } desperateVictory;
                
                struct {
                    SDL_Color textColor = {255, 255, 255, 255};      // 大敗北時のテキスト色（白色）
                    SDL_Color backgroundColor = {255, 0, 0, 255};   // 大敗北時の背景色（赤色）
                } desperateDefeat;
            } resultText;
            
            // 3連勝テキスト
            struct {
                UIPosition position;      // 位置（centerYからの相対位置）
                float offsetY = -150.0f;  // Yオフセット（centerYからの相対位置）
                int baseWidth = 300;      // ベース幅
                int baseHeight = 80;      // ベース高さ
                SDL_Color color = {255, 215, 0, 255};  // 色（金色）
                std::string format = "3連勝！ダメージ{multiplier}倍ボーナス！";  // テキストフォーマット（{multiplier}が置換される）
            } threeWinStreak;
            
            // ダメージボーナステキスト
            struct {
                UIPosition position;      // 位置（centerYからの相対位置）
                float offsetY = 80.0f;    // Yオフセット（centerYからの相対位置）
                float offsetX = -150.0f;  // Xオフセット（centerXからの相対位置）
                SDL_Color color = {255, 255, 100, 255};  // 色（黄色）
                std::string format = "✨ ダメージ{multiplier}倍ボーナス！ ✨";  // テキストフォーマット（{multiplier}が置換される）
            } damageBonus;
        } judgeResult;
        
        // 勝敗UIの設定（renderWinLossUI用）
        struct {
            // 「自分 X勝 敵 Y勝」テキスト
            struct {
                UIPosition position;      // 位置（VSの下からの相対位置）
                float offsetY = 20.0f;     // Yオフセット（VSの下からの相対位置）
                SDL_Color color = {255, 255, 255, 255};  // 色（白色）
                int padding = 8;           // 背景のパディング
                std::string format = "自分 {playerWins}勝  敵 {enemyWins}勝";  // テキストフォーマット（{playerWins}, {enemyWins}が置換される）
            } winLossText;
            
            // 「〜ターン分の攻撃を実行」テキスト
            struct {
                UIPosition position;      // 位置（勝敗UIの下からの相対位置）
                float offsetY = 20.0f;     // Yオフセット（勝敗UIの下からの相対位置）
                SDL_Color color = {255, 255, 255, 255};  // 色（白色）
                int padding = 8;           // 背景のパディング
                std::string playerWinFormat = "{playerName}が{turns}ターン分の攻撃を実行！";  // プレイヤー勝利時のフォーマット
                std::string enemyWinFormat = "敵が{turns}ターン分の攻撃を実行！";  // 敵勝利時のフォーマット
                std::string drawFormat = "相打ち！";  // 引き分け時のフォーマット
                std::string hesitateFormat = "{playerName}はメンタルの影響で攻撃をためらいました。";  // 住民戦で攻撃失敗時のフォーマット
            } totalAttackText;
            
            // 「〜のアタック！」などのテキスト
            struct {
                UIPosition position;      // 位置（totalAttackTextの下からの相対位置）
                float offsetY = 20.0f;     // Yオフセット（totalAttackTextの下からの相対位置）
                SDL_Color color = {255, 255, 255, 255};  // 色（白色）
                int padding = 8;           // 背景のパディング
                std::string attackFormat = "{playerName}のアタック！";  // 通常攻撃のフォーマット
                std::string rushFormat = "{playerName}のラッシュアタック！";  // ラッシュアタックのフォーマット
                std::string statusUpSpellFormat = "{playerName}が強化呪文発動！";  // ステータスアップ呪文のフォーマット
                std::string healSpellFormat = "{playerName}が回復呪文発動！";  // 回復呪文のフォーマット
                std::string attackSpellFormat = "{playerName}が攻撃呪文発動！";  // 攻撃呪文のフォーマット
                std::string defaultSpellFormat = "{playerName}が呪文発動！";  // デフォルト呪文のフォーマット
                std::string defaultAttackFormat = "{playerName}の攻撃！";  // デフォルト攻撃のフォーマット
            } attackText;
        } winLossUI;
        
        // コマンド選択ヒントテキストの設定
        struct {
            UIPosition position;          // 位置（画面下部からの相対位置）
            float offsetY = -100.0f;      // Yオフセット（screenHeightからの相対位置）
            float offsetX = -120.0f;      // Xオフセット（centerXからの相対位置）
            SDL_Color color = {255, 255, 255, 255};  // 色（白色）
            int padding = 8;               // 背景のパディング
            std::string normalText = "選択: W/S 決定: ENTER 戻る: Q";  // 通常時のテキスト
            std::string residentText = "選択: W/S 決定: ENTER";  // 住民戦時のテキスト
        } commandHint;
        
        // ジャッジフェーズの結果表示UI設定（「勝ち！」「負け...」「引き分け」）
        struct {
            UIPosition position;      // 位置（centerX/centerYからの相対位置または絶対位置）
            
            struct {
                std::string text = "勝ち！";  // 勝利時のテキスト
                SDL_Color textColor = {255, 255, 255, 255};  // 勝利時のテキスト色（白色）
                SDL_Color backgroundColor = {255, 215, 0, 255};  // 勝利時の背景色（金色）
            } win;
            
            struct {
                std::string text = "負け...";  // 敗北時のテキスト
                SDL_Color textColor = {255, 255, 255, 255};  // 敗北時のテキスト色（白色）
                SDL_Color backgroundColor = {255, 0, 0, 255};  // 敗北時の背景色（赤色）
            } lose;
            
            struct {
                std::string text = "引き分け";  // 引き分け時のテキスト
                SDL_Color textColor = {255, 255, 255, 255};  // 引き分け時のテキスト色（白色）
                SDL_Color backgroundColor = {200, 200, 200, 255};  // 引き分け時の背景色（グレー）
            } draw;
            
            int baseWidth = 200;      // ベース幅
            int baseHeight = 60;      // ベース高さ
            int backgroundPadding = 20;  // 背景のパディング
            SDL_Color glowColor = {255, 215, 0, 255};  // 勝利時のグロー色（金色）
        } judgePhase;
        
        // ステータス上昇呪文の攻撃倍率表示UI設定
        struct {
            UIPosition position;      // 位置（プレイヤーHPバーの下からの相対位置）
            float offsetX = -100.0f;  // Xオフセット（プレイヤーXからの相対位置）
            float offsetY = 0.0f;     // Yオフセット（プレイヤーHPバーの下からの相対位置）
            SDL_Color textColor = {255, 255, 100, 255};  // テキスト色（黄色）
            SDL_Color bgColor = {0, 0, 0, 255};  // 背景色（黒）
            SDL_Color borderColor = {255, 255, 100, 255};  // ボーダー色（黄色）
            int padding = 8;           // 背景のパディング
            std::string format = "攻撃倍率: {multiplier}倍 (残り{turns}ターン)";  // テキストフォーマット（{multiplier}, {turns}が置換される）
        } attackMultiplier;
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
        struct {
            UITextConfig text;
            UIRectConfig background;
            SDL_Color backgroundColor = {0, 0, 0, 255};  // 背景色（黒）
            SDL_Color borderColor = {255, 255, 255, 255};  // ボーダー色（白）
        } title;
        struct {
            UITextConfig text;
            UIRectConfig background;
            SDL_Color backgroundColor = {0, 0, 0, 255};  // 背景色（黒）
            SDL_Color borderColor = {255, 255, 255, 255};  // ボーダー色（白）
        } reason;
        struct {
            UITextConfig text;
            UIRectConfig background;
            SDL_Color backgroundColor = {0, 0, 0, 255};  // 背景色（黒）
            SDL_Color borderColor = {255, 255, 255, 255};  // ボーダー色（白）
        } instruction;
        struct {
            int baseSize = 300;  // 画像のベースサイズ
        } image;
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
