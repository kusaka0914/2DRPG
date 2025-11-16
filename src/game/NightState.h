/**
 * @file NightState.h
 * @brief 夜の街の状態を担当するクラス
 * @details 夜間の街での移動、住民の襲撃、衛兵との戦闘、城への侵入などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../core/GameUtils.h"
#include "../utils/TownLayout.h"
#include <memory>

/**
 * @brief 夜の街の状態を担当するクラス
 * @details 夜間の街での移動、住民の襲撃、衛兵との戦闘、城への侵入などの機能を管理する。
 */
class NightState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    
    // 夜間の街の状態
    bool isStealthMode;
    int stealthLevel;
    std::vector<std::pair<int, int>> residents; // 住民の位置
    std::vector<std::pair<int, int>> guards; // 見張りの位置
    std::vector<std::pair<int, int>> guardDirections; // 衛兵の移動方向
    
    // 住民を倒した回数
    int residentsKilled;
    const int MAX_RESIDENTS_PER_NIGHT = 3;
    
    // 合計倒した人数（メンタル計算用）
    static int totalResidentsKilled;
    
    // 倒した住民の位置を記録（次の夜に配置しないため）
    static std::vector<std::pair<int, int>> killedResidentPositions;
    
    // プレイヤーの位置を保存（戦闘から戻ってきた時に復元するため）
    static int s_savedPlayerX;
    static int s_savedPlayerY;
    static bool s_playerPositionSaved;
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* residentTextures[6]; // 住民画像（6人分）
    SDL_Texture* guardTexture;
    SDL_Texture* shopTexture;
    SDL_Texture* weaponShopTexture;
    SDL_Texture* houseTexture;
    SDL_Texture* castleTexture;
    SDL_Texture* stoneTileTexture;
    SDL_Texture* residentHomeTexture;
    SDL_Texture* toriiTexture;
    
    // メッセージ表示
    Label* messageBoard;
    bool isShowingMessage;
    
    // 住民襲撃時の選択肢表示
    bool isShowingResidentChoice;
    bool isShowingMercyChoice;
    int selectedChoice;
    std::vector<std::string> choiceOptions;
    int currentTargetX, currentTargetY; // 現在の襲撃対象の位置
    bool showResidentKilledMessage; // 住民を倒したメッセージ表示フラグ
    bool showReturnToTownMessage; // 街に戻るメッセージ表示フラグ（3人目を倒した後用）
    
    // 夜の表示
    Label* nightDisplayLabel; // 夜の表示用ラベル
    Label* nightOperationLabel; // 夜の操作用ラベル
    
    // 建物の位置（街と同じ）
    std::vector<std::pair<int, int>> buildings;
    std::vector<std::string> buildingTypes;
    std::vector<std::pair<int, int>> residentHomes;
    
    // 城の位置
    int castleX, castleY;
    
    // 衛兵の移動管理
    float guardMoveTimer;
    std::vector<int> guardTargetHomeIndices;
    std::vector<float> guardStayTimers;
    bool guardsInitialized;
    
    // ゲーム進行管理
    bool allResidentsKilled;  // 全住民を倒したか
    bool allGuardsKilled;     // 全衛兵を倒したか
    bool canAttackGuards;     // 衛兵を攻撃可能か
    bool canEnterCastle;      // 城に入れるか
    
    // 衛兵のHP管理
    std::vector<int> guardHp;  // 各衛兵のHP（2回アタックで倒す）
    

    
public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     */
    NightState(std::shared_ptr<Player> player);
    
    /**
     * @brief デストラクタ
     */
    ~NightState();
    
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
     * @return 状態タイプ（NIGHT）
     */
    StateType getType() const override { return StateType::NIGHT; }
    
    /**
     * @brief 倒した住民の位置を取得
     * @return 倒した住民の位置のリスト
     */
    static const std::vector<std::pair<int, int>>& getKilledResidentPositions() { return killedResidentPositions; }
    
    /**
     * @brief 住民を倒した処理を実行
     * @param x 住民のX座標
     * @param y 住民のY座標
     */
    void handleResidentKilled(int x, int y);
    
private:
    /**
     * @brief UIのセットアップ
     */
    void setupUI();
    
    /**
     * @brief 移動処理
     * @param input 入力マネージャーへの参照
     */
    void handleMovement(const InputManager& input);
    
    /**
     * @brief 住民との相互作用チェック
     */
    void checkResidentInteraction();
    
    /**
     * @brief 住民への攻撃
     * @param x 住民のX座標
     * @param y 住民のY座標
     */
    void attackResident(int x, int y);
    
    /**
     * @brief 証拠隠滅
     */
    void hideEvidence();
    
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
     * @brief 夜の街の描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawNightTown(Graphics& graphics);
    
    /**
     * @brief プレイヤーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawPlayer(Graphics& graphics);
    
    /**
     * @brief テクスチャの読み込み
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void loadTextures(Graphics& graphics);
    
    /**
     * @brief マップの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawMap(Graphics& graphics);
    
    /**
     * @brief 建物の描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawBuildings(Graphics& graphics);
    
    /**
     * @brief ゲートの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawGate(Graphics& graphics);
    
    /**
     * @brief 有効な位置かどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return 有効な位置かどうか
     */
    bool isValidPosition(int x, int y) const;
    
    /**
     * @brief 建物との衝突判定
     * @param x X座標
     * @param y Y座標
     * @return 建物と衝突しているか
     */
    bool isCollidingWithBuilding(int x, int y) const;
    
    /**
     * @brief 住民との衝突判定
     * @param x X座標
     * @param y Y座標
     * @return 住民と衝突しているか
     */
    bool isCollidingWithResident(int x, int y) const;
    
    /**
     * @brief 衛兵との衝突判定
     * @param x X座標
     * @param y Y座標
     * @return 衛兵と衝突しているか
     */
    bool isCollidingWithGuard(int x, int y) const;
    
    /**
     * @brief 住民のテクスチャインデックスの取得
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @return テクスチャインデックス
     */
    int getResidentTextureIndex(int x, int y) const;
    
    /**
     * @brief 住民の名前の取得
     * @param x 住民のX座標
     * @param y 住民のY座標
     * @return 住民の名前
     */
    std::string getResidentName(int x, int y) const;
    
    /**
     * @brief UIの更新
     */
    void updateUI();
    
    /**
     * @brief 信頼度レベルのチェック
     */
    void checkTrustLevels();
    
    /**
     * @brief 衛兵の更新
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void updateGuards(float deltaTime);
    
    /**
     * @brief 衛兵の近くにいるかどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return 衛兵の近くにいるか
     */
    bool isNearGuard(int x, int y) const;
    
    /**
     * @brief ゲーム進行のチェック
     */
    void checkGameProgress();
    
    /**
     * @brief 衛兵との相互作用チェック
     */
    void checkGuardInteraction();
    
    /**
     * @brief 衛兵への攻撃
     * @param x 衛兵のX座標
     * @param y 衛兵のY座標
     */
    void attackGuard(int x, int y);
    
    /**
     * @brief 城の入り口チェック
     */
    void checkCastleEntrance();
    
    /**
     * @brief 城への入場
     */
    void enterCastle();
    
    /**
     * @brief 王様の撃破
     */
    void defeatKing();
    
    /**
     * @brief 魔王の撃破
     */
    void defeatDemon();
    
    /**
     * @brief 住民襲撃時の選択肢処理
     */
    void handleResidentChoice();
    
    /**
     * @brief 住民襲撃時の選択肢実行
     * @param choice 選択された選択肢のインデックス
     */
    void executeResidentChoice(int choice);
    
    /**
     * @brief 選択肢表示の更新
     */
    void updateChoiceDisplay();
}; 