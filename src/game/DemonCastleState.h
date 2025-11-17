/**
 * @file DemonCastleState.h
 * @brief 魔王の城の状態を担当するクラス
 * @details 魔王の城での移動、魔王との会話、クエスト受領などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../core/GameUtils.h"
#include <memory>
#include <nlohmann/json.hpp>

/**
 * @brief 魔王の城の状態を担当するクラス
 * @details 魔王の城での移動、魔王との会話、クエスト受領などの機能を管理する。
 */
class DemonCastleState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int ROOM_WIDTH = 9;   // 28 → 9に変更
    const int ROOM_HEIGHT = 11; // 16 → 11に変更
    
    // 魔王の位置
    int demonX, demonY;
    int doorX, doorY;        // 出口ドア
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // メッセージ表示
    Label* messageBoard;
    bool isShowingMessage;
    
    // 魔王との会話
    bool isTalkingToDemon;
    int dialogueStep;
    std::vector<std::string> demonDialogues;
    bool hasReceivedEvilQuest;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* demonTexture;
    SDL_Texture* demonCastleTileTexture;
    
    // CastleStateから来たかどうか
    bool fromCastleState;
    
    // ダイアログ表示用フラグ
    bool pendingDialogue;
    
public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     * @param fromCastleState CastleStateから来たかどうか（デフォルト: false）
     */
    DemonCastleState(std::shared_ptr<Player> player, bool fromCastleState = false);
    
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
     * @return 状態タイプ（DEMON_CASTLE）
     */
    StateType getType() const override { return StateType::DEMON_CASTLE; }
    
    /**
     * @brief 状態をJSON形式に変換
     * @return JSONオブジェクト
     */
    nlohmann::json toJson() const override;
    
    /**
     * @brief JSON形式から状態を復元
     * @param j JSONオブジェクト
     */
    void fromJson(const nlohmann::json& j) override;
    
private:
    /**
     * @brief UIのセットアップ
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void setupUI(Graphics& graphics);
    
    /**
     * @brief 魔王の城のセットアップ
     */
    void setupDemonCastle();
    
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
     * @brief 魔王の城の描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawDemonCastle(Graphics& graphics);
    
    /**
     * @brief 魔王の城のオブジェクトの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawDemonCastleObjects(Graphics& graphics);
    
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
     * @brief 魔王との相互作用
     */
    void interactWithDemon();
    
    /**
     * @brief 街への退出
     */
    void exitToTown();
}; 