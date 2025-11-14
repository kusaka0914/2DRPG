/**
 * @file MainMenuState.h
 * @brief メインメニューの状態を担当するクラス
 * @details ゲーム開始、セーブデータ読み込み、プレイヤー情報表示などの機能を管理する。
 */

#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include <memory>

/**
 * @brief メインメニューの状態を担当するクラス
 * @details ゲーム開始、セーブデータ読み込み、プレイヤー情報表示などの機能を管理する。
 */
class MainMenuState : public GameState {
private:
    UIManager ui;
    std::shared_ptr<Player> player;
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> playerInfoLabel;

public:
    /**
     * @brief コンストラクタ
     * @param player プレイヤーへの共有ポインタ
     */
    MainMenuState(std::shared_ptr<Player> player);
    
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
     * @return 状態タイプ（MAIN_MENU）
     */
    StateType getType() const override { return StateType::MAIN_MENU; }
    
private:
    /**
     * @brief UIのセットアップ
     */
    void setupUI();
    
    /**
     * @brief プレイヤー情報の更新
     */
    void updatePlayerInfo();
}; 