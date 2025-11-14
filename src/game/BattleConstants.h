/**
 * @file BattleConstants.h
 * @brief 戦闘関連の定数を定義するヘッダーファイル
 * @details 戦闘システムで使用される全ての定数（タイミング、アニメーション、画面位置など）を集約している。
 */

#pragma once

/**
 * @brief 読み合い判定のサブフェーズ
 * @details プレイヤーコマンド表示、敵コマンド表示、結果表示の3段階を管理する。
 */
enum class JudgeSubPhase {
    SHOW_PLAYER_COMMAND,  // プレイヤーのコマンドを左に表示
    SHOW_ENEMY_COMMAND,   // 敵のコマンドを右に表示
    SHOW_RESULT           // 勝敗を真ん中に大きく表示
};

/**
 * @brief 戦闘関連の定数名前空間
 * @details 戦闘システムで使用される全ての定数を集約している。
 */
namespace BattleConstants {
    /** @brief タイミング定数 */
    constexpr float INTRO_DURATION = 1.0f;
    constexpr float JUDGE_PLAYER_COMMAND_DISPLAY_TIME = 1.0f;
    constexpr float JUDGE_ENEMY_COMMAND_DISPLAY_TIME = 1.0f;
    constexpr float JUDGE_RESULT_DISPLAY_TIME = 1.5f;
    constexpr float JUDGE_RESULT_ANNOUNCEMENT_DURATION = 3.0f;
    constexpr float EXECUTE_DAMAGE_DELAY = 0.5f;
    constexpr float COMMAND_SELECT_ANIMATION_DURATION = 0.3f;
    constexpr float RESULT_SCALE_ANIMATION_DURATION = 0.5f;
    constexpr float RESULT_ROTATION_ANIMATION_DURATION = 1.0f;
    constexpr float RESULT_SHAKE_DURATION = 0.3f;
    constexpr float DESPERATE_RESULT_SHAKE_DURATION = 0.5f;
    
    // アニメーション定数
    constexpr float ATTACK_ANIMATION_HIT_MOMENT = 0.2f;
    constexpr float ATTACK_ANIMATION_HIT_WINDOW = 0.05f;
    constexpr float RESULT_PULSE_FREQUENCY = 3.14159f * 2.0f;
    constexpr float RESULT_PULSE_AMPLITUDE = 0.1f;
    constexpr float DESPERATE_RESULT_PULSE_FREQUENCY = 3.14159f * 3.0f;
    constexpr float DESPERATE_RESULT_PULSE_AMPLITUDE = 0.15f;
    constexpr float RESULT_ROTATION_DEGREES = 360.0f;
    constexpr float DESPERATE_RESULT_ROTATION_DEGREES = 720.0f;
    
    // 画面揺れ定数
    constexpr float VICTORY_SHAKE_INTENSITY = 15.0f;
    constexpr float DEFEAT_SHAKE_INTENSITY = 20.0f;
    constexpr float DRAW_SHAKE_INTENSITY = 10.0f;
    constexpr float DESPERATE_VICTORY_SHAKE_INTENSITY = 25.0f;
    constexpr float DESPERATE_DEFEAT_SHAKE_INTENSITY = 30.0f;
    constexpr float DESPERATE_DRAW_SHAKE_INTENSITY = 15.0f;
    constexpr float DAMAGE_SHAKE_INTENSITY = 15.0f;
    constexpr float DAMAGE_SHAKE_DURATION = 0.5f;
    constexpr float ATTACK_SHAKE_INTENSITY_NORMAL = 20.0f;
    constexpr float ATTACK_SHAKE_INTENSITY_STREAK = 30.0f;
    constexpr float ATTACK_SHAKE_DURATION = 0.6f;
    constexpr float DESPERATE_ATTACK_SHAKE_INTENSITY_NORMAL = 25.0f;
    constexpr float DESPERATE_ATTACK_SHAKE_INTENSITY_STREAK = 35.0f;
    constexpr float DESPERATE_ATTACK_SHAKE_DURATION = 0.7f;
    
    // コマンド定数
    constexpr int NORMAL_TURN_COUNT = 3;
    constexpr int DESPERATE_TURN_COUNT = 6;
    constexpr int COMMAND_ATTACK = 0;
    constexpr int COMMAND_DEFEND = 1;
    constexpr int COMMAND_SPELL = 2;
    
    // 判定結果定数
    constexpr int JUDGE_RESULT_PLAYER_WIN = 1;
    constexpr int JUDGE_RESULT_ENEMY_WIN = -1;
    constexpr int JUDGE_RESULT_DRAW = 0;
    
    // ダメージ倍率
    constexpr float THREE_WIN_STREAK_MULTIPLIER = 1.5f;
    constexpr float DESPERATE_MODE_MULTIPLIER = 1.5f;
    
    // 画面位置定数
    constexpr int SCREEN_WIDTH = 1920;
    constexpr int SCREEN_HEIGHT = 1080;
    constexpr int PLAYER_POSITION_X = SCREEN_WIDTH / 4;  // 480
    constexpr int PLAYER_POSITION_Y = SCREEN_HEIGHT / 2; // 540
    constexpr int ENEMY_POSITION_X = SCREEN_WIDTH * 3 / 4; // 1440
    constexpr int ENEMY_POSITION_Y = SCREEN_HEIGHT / 2;    // 540
    constexpr int CHARACTER_SIZE = 300; // 3倍サイズ
    
    // 会心の一撃
    constexpr int CRITICAL_HIT_CHANCE = 8; // 8%
    constexpr int CRITICAL_HIT_MULTIPLIER = 2;
    
    // 夜タイマー
    constexpr float NIGHT_TIMER_DURATION = 10.0f; // テスト用に10秒
}

