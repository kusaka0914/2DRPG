/**
 * @file FieldState.h
 * @brief フィールドの状態を担当するクラス
 * @details フィールドでの移動、敵とのエンカウント、街への入場、ストーリー表示などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "../utils/MapTerrain.h"
#include <memory>

/**
 * @brief フィールドの状態を担当するクラス
 * @details フィールドでの移動、敵とのエンカウント、街への入場、ストーリー表示などの機能を管理する。
 */
class FieldState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    StoryMessageBox* storyBox;  // ストーリーメッセージ用
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 38;
    const int MAP_WIDTH = 28;  // 25 → 28に拡大（画面幅1100px ÷ 38px = 約29タイル、UI部分を考慮して28）
    const int MAP_HEIGHT = 16; // 18 → 16に調整（画面高さ650px ÷ 38px = 約17タイル、UI部分を考慮して16）
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // エンカウント関連（移動時のみチェック）
    
    // モンスター出現場所管理
    std::vector<std::pair<int, int>> monsterSpawnPoints; // モンスター出現場所のリスト
    std::vector<std::pair<int, int>> activeMonsterPoints; // 現在アクティブなモンスター出現場所
    std::vector<EnemyType> activeMonsterTypes; // 各出現場所の敵の種類
    std::vector<int> activeMonsterLevels; // 各出現場所の敵のレベル
    
    // 戦闘終了時の処理
    bool shouldRelocateMonster;
    int lastBattleX, lastBattleY;
    
    // 地形マップ
    std::vector<std::vector<MapTile>> terrainMap;
    bool hasMoved;
    
    // 夜のタイマー機能（TownStateと共有）
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒

public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     */
    FieldState(std::shared_ptr<Player> player);
    
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
     * @return 状態タイプ（FIELD）
     */
    StateType getType() const override { return StateType::FIELD; }
    
    /**
     * @brief オープニングストーリーの表示
     */
    void showOpeningStory();
    
    /**
     * @brief レベルアップストーリーの表示
     * @param newLevel 新しいレベル
     */
    void showLevelUpStory(int newLevel); 
    
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
     * @brief エンカウントチェック
     */
    void checkEncounter();
    
    /**
     * @brief 街の入り口チェック
     */
    void checkTownEntrance();
    
    /**
     * @brief マップの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawMap(Graphics& graphics);
    
    /**
     * @brief プレイヤーの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawPlayer(Graphics& graphics);
    
    /**
     * @brief 地形の描画
     * @param graphics グラフィックスオブジェクトへの参照
     * @param tile マップタイル
     * @param x X座標
     * @param y Y座標
     */
    void drawTerrain(Graphics& graphics, const MapTile& tile, int x, int y);
    
    /**
     * @brief 有効な位置かどうかの判定
     * @param x X座標
     * @param y Y座標
     * @return 有効な位置かどうか
     */
    bool isValidPosition(int x, int y) const;
    
    /**
     * @brief 現在の地形の取得
     * @return 現在の地形タイプ
     */
    TerrainType getCurrentTerrain() const;
    
    /**
     * @brief フィールド画像の読み込み
     */
    void loadFieldImages();
    
    /**
     * @brief モンスター出現ポイントの生成
     */
    void generateMonsterSpawnPoints();
    
    /**
     * @brief モンスター出現ポイントの再配置
     * @param oldX 古いX座標
     * @param oldY 古いY座標
     */
    void relocateMonsterSpawnPoint(int oldX, int oldY);
    
    /**
     * @brief フィールドゲートの描画
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void drawFieldGate(Graphics& graphics);
}; 