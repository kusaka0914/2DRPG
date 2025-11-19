/**
 * @file BattleLogic.h
 * @brief 戦闘ロジックを担当するクラス
 * @details コマンド判定、ダメージ計算、戦闘統計の管理を行う。
 * 単一責任の原則に従い、戦闘の核心ロジックをBattleStateから分離している。
 */

#pragma once
#include "../entities/Player.h"
#include "../entities/Enemy.h"
#include "BattleConstants.h"
#include <vector>
#include <memory>

/**
 * @brief 戦闘ロジックを担当するクラス
 * @details コマンド判定、ダメージ計算、戦闘統計の管理を行う。
 * 単一責任の原則に従い、戦闘の核心ロジックをBattleStateから分離している。
 */
class BattleLogic {
public:
    /**
     * @brief コマンド判定結果を格納する構造体
     * @details 1ラウンドの判定結果と、プレイヤー・敵のコマンドを保持する。
     */
    struct JudgeResult {
        int result;  /**< @brief 判定結果（1=プレイヤー勝ち, -1=敵勝ち, 0=引き分け） */
        int playerCommand;  /**< @brief プレイヤーのコマンド（0=攻撃, 1=防御, 2=呪文） */
        int enemyCommand;  /**< @brief 敵のコマンド（0=攻撃, 1=防御, 2=呪文） */
    };
    
    /**
     * @brief ダメージ情報を格納する構造体
     * @details ダメージ値と、ダメージを受けた対象（プレイヤーまたは敵）を保持する。
     */
    struct DamageInfo {
        int damage;
        bool isPlayerHit;  /**< @brief true=プレイヤーがダメージを受けた, false=敵がダメージを受けた */
        bool isDraw;       /**< @brief true=引き分け（両方がダメージを受ける） */
        int playerDamage;  /**< @brief 引き分け時のプレイヤーのダメージ */
        int enemyDamage;   /**< @brief 引き分け時の敵のダメージ */
        int commandType;   /**< @brief コマンドタイプ（0=攻撃, 1=防御, 2=呪文）-1=未設定 */
        bool isCounterRush; /**< @brief true=カウンターラッシュ（防御で勝利した場合） */
        bool skipAnimation; /**< @brief true=攻撃アニメーションをスキップ（ステータス上昇呪文や回復呪文の場合） */
        bool isSpecialSkill;  /**< @brief true=特殊技を使用した */
        std::string specialSkillName;  /**< @brief 特殊技の名前（空文字列の場合は通常攻撃） */
    };
    
    /**
     * @brief 戦闘統計を格納する構造体
     * @details プレイヤーと敵の勝利数、3連勝フラグを保持する。
     */
    struct BattleStats {
        int playerWins;
        int enemyWins;
        bool hasThreeWinStreak;
    };

    /**
     * @brief 敵の行動タイプ
     * @details 敵の行動パターンを表す（攻撃型、防御型、呪文型）
     */
    enum class EnemyBehaviorType {
        ATTACK_TYPE,   /**< @brief 攻撃型：攻撃50%、防御30%、呪文20% */
        DEFEND_TYPE,   /**< @brief 防御型：防御50%、攻撃30%、呪文20% */
        SPELL_TYPE     /**< @brief 呪文型：呪文50%、攻撃30%、防御20% */
    };

private:
    std::shared_ptr<Player> player;
    Enemy* enemy;
    
    std::vector<int> playerCommands;
    std::vector<int> enemyCommands;
    int commandTurnCount;
    bool isDesperateMode;
    
    BattleStats stats;
    EnemyBehaviorType enemyBehaviorType;  /**< @brief 敵の行動タイプ（ランダムに決定） */
    bool behaviorTypeDetermined;  /**< @brief 行動タイプが確定したか（HP7割以下で確定） */
    EnemyBehaviorType excludedBehaviorType;  /**< @brief 除外する行動タイプ（ヒント表示用、戦闘開始時に一度だけ決定） */

public:
    /**
     * @brief コンストラクタ
     * @details プレイヤーと敵への参照を保持し、戦闘ロジックの初期化を行う。
     * 
     * @param player プレイヤーへの共有ポインタ
     * @param enemy 敵へのポインタ
     */
    BattleLogic(std::shared_ptr<Player> player, Enemy* enemy);
    
    /**
     * @brief コマンド判定
     * @details プレイヤーと敵のコマンドを比較し、勝敗を判定する。
     * 攻撃>呪文>防御>攻撃の循環関係に基づいて判定を行う。
     * 
     * @param playerCmd プレイヤーのコマンド（0=攻撃, 1=防御, 2=呪文）
     * @param enemyCmd 敵のコマンド（0=攻撃, 1=防御, 2=呪文）
     * @return 判定結果（1=プレイヤー勝ち, -1=敵勝ち, 0=引き分け）
     */
    int judgeRound(int playerCmd, int enemyCmd) const;
    
    /**
     * @brief 全ラウンドの判定
     * @details プレイヤーと敵の全コマンドを順に判定し、各ラウンドの結果を返す。
     * 
     * @return 全ラウンドの判定結果のベクター
     */
    std::vector<JudgeResult> judgeAllRounds() const;
    
    /**
     * @brief 戦闘統計の計算
     * @details 全ラウンドの判定結果から、プレイヤーと敵の勝利数を集計し、
     * 3連勝フラグを設定する。計算結果はメンバ変数のstatsに保存される。
     * 
     * @return 戦闘統計（勝利数、3連勝フラグなど）
     */
    BattleStats calculateBattleStats();
    
    /**
     * @brief ダメージリストの準備
     * @details 勝利したターンのコマンドに基づいて、実行すべきダメージを計算し、
     * リストとして準備する。攻撃コマンドと呪文コマンドで異なるダメージ計算を行う。
     * 
     * @param damageMultiplier ダメージ倍率（デフォルト: 1.0f）
     * @return ダメージ情報のベクター（ダメージ値と対象の情報を含む）
     */
    std::vector<DamageInfo> prepareDamageList(float damageMultiplier = 1.0f);
    
    /**
     * @brief プレイヤーの攻撃ダメージ計算
     * @details プレイヤーの攻撃力と敵の防御力を考慮して、攻撃ダメージを計算する。
     * 会心の一撃の判定も行う。
     * 
     * @param multiplier ダメージ倍率（デフォルト: 1.0f）
     * @return 計算されたダメージ値
     */
    int calculatePlayerAttackDamage(float multiplier = 1.0f) const;
    
    /**
     * @brief 敵の攻撃ダメージ計算
     * @details 敵の攻撃力とプレイヤーの防御力を考慮して、攻撃ダメージを計算する。
     * 
     * @return 計算されたダメージ値
     */
    int calculateEnemyAttackDamage() const;
    
    /**
     * @brief 呪文ダメージ計算
     * @details 呪文の種類に応じて、固定ダメージまたはプレイヤーの攻撃力に基づく
     * ダメージを計算する。
     * 
     * @param spellCommand 呪文コマンド（2=呪文）
     * @param multiplier ダメージ倍率（デフォルト: 1.0f）
     * @return 計算されたダメージ値
     */
    int calculateSpellDamage(int spellCommand, float multiplier = 1.0f) const;
    
    /**
     * @brief プレイヤーのコマンド設定
     * @details プレイヤーが選択したコマンドのリストを設定する。
     * 
     * @param commands プレイヤーのコマンドベクター
     */
    void setPlayerCommands(const std::vector<int>& commands);
    
    /**
     * @brief 敵のコマンド設定
     * @details 敵が選択したコマンドのリストを設定する。
     * 
     * @param commands 敵のコマンドベクター
     */
    void setEnemyCommands(const std::vector<int>& commands);
    
    /**
     * @brief コマンドターン数の設定
     * @details 通常モード（3ターン）または窮地モード（6ターン）のターン数を設定する。
     * 
     * @param count ターン数
     */
    void setCommandTurnCount(int count);
    
    /**
     * @brief 窮地モードの設定
     * @details 窮地モードの有効/無効を設定する。窮地モードではダメージ倍率が1.5倍になる。
     * 
     * @param desperate 窮地モードフラグ
     */
    void setDesperateMode(bool desperate);
    
    // ゲッター
    const std::vector<int>& getPlayerCommands() const { return playerCommands; }
    const std::vector<int>& getEnemyCommands() const { return enemyCommands; }
    int getCommandTurnCount() const { return commandTurnCount; }
    bool getIsDesperateMode() const { return isDesperateMode; }
    const BattleStats& getStats() const { return stats; }
    
    /**
     * @brief 窮地モード条件のチェック
     * @details プレイヤーのHPが一定値以下（30%以下）の場合、窮地モードの発動条件を満たす。
     * 
     * @return 窮地モード発動条件を満たしているか
     */
    bool checkDesperateModeCondition() const;
    
    /**
     * @brief 敵のコマンド生成
     * @details 敵のAIに基づいて、ランダムまたは戦略的なコマンドを生成する。
     * 生成されたコマンドはenemyCommandsに格納される。
     */
    void generateEnemyCommands();
    
    /**
     * @brief コマンド名取得
     * @details コマンド番号から対応するコマンド名の文字列を取得する。
     * 
     * @param cmd コマンド番号（0=攻撃, 1=防御, 2=呪文）
     * @return コマンド名の文字列（"攻撃", "防御", "呪文"）
     */
    static std::string getCommandName(int cmd);
    
    /**
     * @brief 3連勝チェック
     * @details プレイヤーが連続して3回以上勝利しているかどうかを判定する。
     * 3連勝時はダメージ倍率が2.5倍になる。
     * 
     * @return 3連勝しているか
     */
    bool checkThreeWinStreak() const;
    
    /**
     * @brief 敵の行動タイプをランダムに決定
     * @details 戦闘開始時に敵の行動タイプ（攻撃型、防御型、呪文型）をランダムに決定する。
     */
    void determineEnemyBehaviorType();
    
    /**
     * @brief 敵の行動タイプの取得
     * @return 敵の行動タイプ
     */
    EnemyBehaviorType getEnemyBehaviorType() const { return enemyBehaviorType; }
    
    /**
     * @brief 行動タイプが確定したかの取得
     * @return 行動タイプが確定したか
     */
    bool isBehaviorTypeDetermined() const { return behaviorTypeDetermined; }
    
    /**
     * @brief 行動タイプを確定する
     * @details プレイヤーのHPが7割を切った時に呼ばれる
     */
    void confirmBehaviorType();
    
    /**
     * @brief 行動タイプ名の取得
     * @param type 行動タイプ
     * @return 行動タイプ名の文字列（"攻撃型", "防御型", "呪文型"）
     */
    static std::string getBehaviorTypeName(EnemyBehaviorType type);
    
    /**
     * @brief 行動タイプのヒントテキストの取得
     * @param type 行動タイプ
     * @return ヒントテキスト（"気性が荒いみたいだ", "臆病みたいだ", "近づきたくないようだ"）
     */
    static std::string getBehaviorTypeHint(EnemyBehaviorType type);
    
    /**
     * @brief 行動タイプでない場合のヒントテキストの取得
     * @param type 行動タイプ
     * @return ヒントテキスト（"気性は荒くなさそうだ", "臆病ではなさそうだ", "距離を取るタイプではなさそうだ"）
     */
    static std::string getNegativeBehaviorTypeHint(EnemyBehaviorType type);
    
    /**
     * @brief 除外する行動タイプの取得
     * @return 除外する行動タイプ（ヒント表示用）
     */
    EnemyBehaviorType getExcludedBehaviorType() const { return excludedBehaviorType; }
    
    /**
     * @brief 敵の特殊技名を取得
     * @param enemyType 敵の種類
     * @return 特殊技名（空文字列の場合は通常攻撃）
     */
    static std::string getEnemySpecialSkillName(EnemyType enemyType);
};

