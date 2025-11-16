/**
 * @file CastleState.h
 * @brief 城の状態を担当するクラス
 * @details 城での移動、王様との会話、クエスト受領などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../core/GameUtils.h"
#include <memory>

/**
 * @brief 城の状態を担当するクラス
 * @details 城での移動、王様との会話、クエスト受領などの機能を管理する。
 */
class CastleState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int ROOM_WIDTH = 9;  // 28 → 13に変更
    const int ROOM_HEIGHT = 11; // 16 → 11に変更
    
    // 城のオブジェクト位置（1タイルサイズ）
    int throneX, throneY;    // 王座（中央）
    int guardLeftX, guardLeftY;  // 左衛兵
    int guardRightX, guardRightY; // 右衛兵
    int doorX, doorY;        // 出口ドア
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // メッセージ表示
    Label* messageBoard;
    bool isShowingMessage;
    
    // 王様との会話
    bool isTalkingToKing;
    int dialogueStep;
    std::vector<std::string> kingDialogues;
    bool hasReceivedQuest;
    
    // 魔王の城への移行フラグ
    bool shouldGoToDemonCastle;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* kingTexture;
    SDL_Texture* guardTexture;
    SDL_Texture* castleTileTexture;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒
    
    // NightStateから来たかどうか
    bool fromNightState;
    
    // NightStateからの場合の戦闘用変数
    bool kingDefeated;
    bool guardLeftDefeated;
    bool guardRightDefeated;
    bool allDefeated;
    
    // ダイアログ表示用フラグ
    bool pendingDialogue;
    
public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     * @param fromNightState NightStateから来たかどうか（デフォルト: false）
     */
    CastleState(std::shared_ptr<Player> player, bool fromNightState = false);
    
    /**
     * @brief 状態に入る
     */
    void enter() override;
    
    /**
     * @brief 状態から出る
     */
    void exit() override;
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime) override;
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics) override;
    
    /**
     * @brief 入力処理
     * @param input 入力マネージャーへの参照
     */
    void handleInput(const InputManager& input) override;
    
    /**
     * @brief 状態タイプの取得
     * @return 状態タイプ（CASTLE）
     */
    StateType getType() const override { return StateType::CASTLE; }
    
private:
    /**
     * @brief UIのセットアップ
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void setupUI(Graphics& graphics);
    
    /**
     * @brief 城のセットアップ
     */
    void setupCastle();
    
    /**
     * @brief テクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void loadTextures(Graphics& graphics);
    
    /**
     * @brief 移動処理
     * @param input 入力マネージャーへの参照
     */
    void handleMovement(const InputManager& input);
    
    /**
     * @brief 相互作用チェック
     */
    void checkInteraction();
    
    /**
     * @brief ダイアログの開始
     */
    void startDialogue();
    
    /**
     * @brief 次のダイアログへ進む
     */
    void nextDialogue();
    
    /**
     * @brief 現在のダイアログの表示
     */
    void showCurrentDialogue();
    
    /**
     * @brief メッセージの表示
     * @param message 表示するメッセージ
     */
    void showMessage(const std::string& message);
    
    /**
     * @brief メッセージのクリア
     */
    void clearMessage();
    
    /**
     * @brief 城の描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawCastle(Graphics& graphics);
    
    /**
     * @brief 城のオブジェクトの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawCastleObjects(Graphics& graphics);
    
    /**
     * @brief プレイヤーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawPlayer(Graphics& graphics);
    
    /**
     * @brief 有効な位置かどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return 有効な位置かどうか
     */
    bool isValidPosition(int x, int y) const;
    
    /**
     * @brief オブジェクトの近くにいるかどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return オブジェクトの近くにいるか
     */
    bool isNearObject(int x, int y) const;
    
    /**
     * @brief 王座との相互作用
     */
    void interactWithThrone();
    
    /**
     * @brief 衛兵との相互作用
     */
    void interactWithGuard();
    
    /**
     * @brief 王様への攻撃
     */
    void attackKing();
    
    /**
     * @brief 左衛兵への攻撃
     */
    void attackGuardLeft();
    
    /**
     * @brief 右衛兵への攻撃
     */
    void attackGuardRight();
    
    /**
     * @brief 全員撃破のチェック
     */
    void checkAllDefeated();
    
    /**
     * @brief 街への退出
     */
    void exitToTown();
}; 