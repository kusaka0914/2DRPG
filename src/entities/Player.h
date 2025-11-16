/**
 * @file Player.h
 * @brief プレイヤーキャラクターを担当するクラス
 * @details プレイヤーキャラクターのステータス、アイテム、装備、呪文、ストーリーなどを管理する。
 * Characterクラスを継承し、プレイヤー固有の機能を提供する。
 * 単一責任の原則に従い、ステータス管理はPlayerStats、ストーリー管理はPlayerStory、
 * 信頼度管理はPlayerTrustに分離している。
 */

#pragma once
#include "Character.h"
#include "PlayerStats.h"
#include "PlayerStory.h"
#include "PlayerTrust.h"
#include "../items/Inventory.h"
#include "../items/Equipment.h"
#include <map>

/**
 * @brief 魔法の種類
 * @details プレイヤーが使用可能な魔法の種類を定義する。
 */
enum class SpellType {
    STATUS_UP,    /**< @brief ステータスアップ魔法（次の攻撃が2.5倍になる） */
    HEAL,         /**< @brief 回復魔法（HPが4割回復） */
    ATTACK        /**< @brief 攻撃魔法（通常の攻撃と同じ攻撃力） */
};

/**
 * @brief プレイヤーキャラクターを担当するクラス
 * @details プレイヤーキャラクターのステータス、アイテム、装備、呪文、ストーリーなどを管理する。
 * Characterクラスを継承し、プレイヤー固有の機能を提供する。
 * 単一責任の原則に従い、ステータス管理はPlayerStats、ストーリー管理はPlayerStory、
 * 信頼度管理はPlayerTrustに分離している。
 */
class Player : public Character {
private:
    std::map<SpellType, int> spells; // 呪文とそのMP消費量
    Inventory inventory;
    EquipmentManager equipmentManager;
    
    // ステータス管理（単一責任の原則）
    std::unique_ptr<PlayerStats> playerStats;
    
    // ストーリー管理（単一責任の原則）
    std::unique_ptr<PlayerStory> playerStory;
    
    // 信頼度管理（単一責任の原則）
    std::unique_ptr<PlayerTrust> playerTrust;
    
    // 夜間システム
    bool isNightTime; // 夜間かどうか
    
    // 夜の情報
    int currentNight;     // 現在の夜の回数
    std::vector<std::pair<int, int>> killedResidents; // 倒した住民の位置

public:
    /**
     * @brief コンストラクタ
     * @param name プレイヤー名
     */
    Player(const std::string& name);
    
    /**
     * @brief レベルアップ
     * @details Characterクラスの純粋仮想関数を実装。
     */
    void levelUp() override;
    
    /**
     * @brief 情報表示
     * @details Characterクラスの純粋仮想関数を実装。
     */
    void displayInfo() const override;
    
    /**
     * @brief 実効攻撃力の取得
     * @details Characterクラスの仮想関数を実装。装備ボーナスを含む。
     * 
     * @return 実効攻撃力
     */
    int getEffectiveAttack() const override;
    
    /**
     * @brief 実効防御力の取得
     * @details Characterクラスの仮想関数を実装。装備ボーナスを含む。
     * 
     * @return 実効防御力
     */
    int getEffectiveDefense() const override;
    
    /**
     * @brief 経験値の獲得
     * @param expGained 獲得する経験値
     */
    void gainExp(int expGained);
    
    /**
     * @brief ゴールドの獲得
     * @param goldGained 獲得するゴールド
     */
    void gainGold(int goldGained) { playerStats->changeGold(goldGained); }
    
    /**
     * @brief 呪文を唱えられるか判定
     * @param spell 呪文の種類
     * @return 呪文を唱えられるか
     */
    bool canCastSpell(SpellType spell) const;
    
    /**
     * @brief 呪文を唱える
     * @param spell 呪文の種類
     * @param target ターゲット（nullptrの場合は自分自身）
     * @return 与えたダメージ（回復呪文の場合は回復量）
     */
    int castSpell(SpellType spell, Character* target = nullptr);
    
    /**
     * @brief 呪文を習得
     * @param spell 呪文の種類
     * @param mpCost MP消費量
     */
    void learnSpell(SpellType spell, int mpCost);
    
    /**
     * @brief ゴールドの取得
     * @return 所持ゴールド
     */
    int getGold() const { return playerStats->getExtendedStats().gold; }
    
    /**
     * @brief インベントリの取得
     * @return インベントリへの参照
     */
    Inventory& getInventory() { return inventory; }
    
    /**
     * @brief インベントリの取得（const版）
     * @return インベントリへのconst参照
     */
    const Inventory& getInventory() const { return inventory; }
    
    /**
     * @brief PlayerStatsの取得
     * @return PlayerStatsへの参照
     */
    PlayerStats& getPlayerStats() { return *playerStats; }
    
    /**
     * @brief PlayerStatsの取得（const版）
     * @return PlayerStatsへのconst参照
     */
    const PlayerStats& getPlayerStats() const { return *playerStats; }
    
    /**
     * @brief 初期アイテムの追加
     */
    void addStartingItems();
    
    /**
     * @brief インベントリの表示
     */
    void showInventory() const;
    
    /**
     * @brief アイテムの使用
     * @param itemIndex アイテムのインデックス
     * @param target ターゲット（nullptrの場合は自分自身）
     * @return アイテムの使用が成功したか
     */
    bool useItem(int itemIndex, Character* target = nullptr);
    
    /**
     * @brief 装備管理の取得
     * @return 装備管理への参照
     */
    EquipmentManager& getEquipmentManager() { return equipmentManager; }
    
    /**
     * @brief 装備管理の取得（const版）
     * @return 装備管理へのconst参照
     */
    const EquipmentManager& getEquipmentManager() const { return equipmentManager; }
    
    /**
     * @brief 装備の表示
     */
    void showEquipment() const;
    
    /**
     * @brief アイテムを装備
     * @param equipment 装備するアイテム
     * @return 装備が成功したか
     */
    bool equipItem(std::unique_ptr<Equipment> equipment);
    
    /**
     * @brief アイテムを外す
     * @param slot 装備スロット
     * @return 外すことができたか
     */
    bool unequipItem(EquipmentSlot slot);
    
    /**
     * @brief 総攻撃力の取得（装備ボーナス含む）
     * @return 総攻撃力
     */
    int getTotalAttack() const;
    
    /**
     * @brief 総防御力の取得（装備ボーナス含む）
     * @return 総防御力
     */
    int getTotalDefense() const;
    
    /**
     * @brief 総最大HPの取得（装備ボーナス含む）
     * @return 総最大HP
     */
    int getTotalMaxHp() const;
    
    /**
     * @brief 総最大MPの取得（装備ボーナス含む）
     * @return 総最大MP
     */
    int getTotalMaxMp() const;
    
    /**
     * @brief 攻撃を実行
     * @param target ターゲット
     * @return 与えたダメージ
     */
    int attack(Character& target);
    
    /**
     * @brief 防御状態にする
     */
    void defend();
    
    /**
     * @brief 逃走を試みる
     * @return 逃走が成功したか
     */
    bool tryToEscape();
    
    /**
     * @brief 攻撃ボーナスを考慮したダメージ計算
     * @param target ターゲット
     * @return 計算されたダメージ
     */
    int calculateDamageWithBonus(const Character& target) const;
    
    /**
     * @brief オープニングストーリーの取得
     * @return オープニングストーリーの文字列リスト
     */
    std::vector<std::string> getOpeningStory() const { return playerStory->getOpeningStory(); }
    
    /**
     * @brief レベルアップストーリーの取得
     * @param newLevel 新しいレベル
     * @return レベルアップストーリーの文字列リスト
     */
    std::vector<std::string> getLevelUpStory(int newLevel) const { return playerStory->getLevelUpStory(newLevel); }
    
    /**
     * @brief レベルアップストーリーがあるか
     * @return レベルアップストーリーがあるか
     */
    bool hasLevelUpStory() const { return playerStory->hasLevelUpStory(); }
    
    /**
     * @brief レベルアップストーリーのレベルを取得
     * @return レベルアップストーリーのレベル
     */
    int getLevelUpStoryLevel() const { return playerStory->getLevelUpStoryLevel(); }
    
    /**
     * @brief レベルアップストーリーのフラグをクリア
     */
    void clearLevelUpStoryFlag() { playerStory->clearLevelUpStoryFlag(); }
    
    /**
     * @brief PlayerStoryの取得
     * @return PlayerStoryへの参照
     */
    PlayerStory& getPlayerStory() { return *playerStory; }
    
    /**
     * @brief PlayerStoryの取得（const版）
     * @return PlayerStoryへのconst参照
     */
    const PlayerStory& getPlayerStory() const { return *playerStory; }
    
    /**
     * @brief 呪文名の取得
     * @param spell 呪文の種類
     * @return 呪文名の文字列
     */
    static std::string getSpellName(SpellType spell);
    
    /**
     * @brief レベルで習得可能な呪文の取得
     * @param level レベル
     * @return 習得可能な呪文のリスト
     */
    static std::vector<SpellType> getSpellsLearnedAtLevel(int level);

    /**
     * @brief 信頼度の取得
     * @return 信頼度（範囲: 0-100）
     */
    int getTrustLevel() const { return playerTrust->getTrustLevel(); }
    
    /**
     * @brief 信頼度の変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeTrustLevel(int amount) { playerTrust->changeTrustLevel(amount); }
    
    /**
     * @brief 悪キャラクターかどうか
     * @return 悪キャラクターかどうか
     */
    bool isEvilCharacter() const { return playerTrust->isEvilCharacter(); }
    
    /**
     * @brief 悪行を記録
     */
    void performEvilAction() { playerTrust->recordEvilAction(); }
    
    /**
     * @brief 善行を記録
     */
    void performGoodAction() { playerTrust->recordGoodAction(); }
    
    /**
     * @brief 悪行の回数の取得
     * @return 悪行の回数
     */
    int getEvilActions() const { return playerTrust->getEvilActions(); }
    
    /**
     * @brief 善行の回数の取得
     * @return 善行の回数
     */
    int getGoodActions() const { return playerTrust->getGoodActions(); }
    
    /**
     * @brief PlayerTrustの取得
     * @return PlayerTrustへの参照
     */
    PlayerTrust& getPlayerTrust() { return *playerTrust; }
    
    /**
     * @brief PlayerTrustの取得（const版）
     * @return PlayerTrustへのconst参照
     */
    const PlayerTrust& getPlayerTrust() const { return *playerTrust; }
    
    /**
     * @brief 夜間モードかどうか
     * @return 夜間モードかどうか
     */
    bool isNightTimeMode() const { return isNightTime; }
    
    /**
     * @brief 夜間モードの設定
     * @param night 夜間モードかどうか
     */
    void setNightTime(bool night);
    
    /**
     * @brief 夜間モードの切り替え
     */
    void toggleNightTime();
    
    /**
     * @brief メンタルの取得
     * @return メンタル（範囲: 0-100）
     */
    int getMental() const { return playerStats->getExtendedStats().mental; }
    
    /**
     * @brief メンタルの設定
     * @param value メンタル値（範囲: 0-100）
     */
    void setMental(int value) { 
        auto& stats = playerStats->getExtendedStats();
        stats.mental = std::max(0, std::min(100, value));
    }
    
    /**
     * @brief 魔王からの信頼度の取得
     * @return 魔王からの信頼度（範囲: 0-100）
     */
    int getDemonTrust() const { return playerStats->getExtendedStats().demonTrust; }
    
    /**
     * @brief 魔王からの信頼度の設定
     * @param value 信頼度（範囲: 0-100）
     */
    void setDemonTrust(int value) {
        auto& stats = playerStats->getExtendedStats();
        stats.demonTrust = std::max(0, std::min(100, value));
    }
    
    /**
     * @brief 王様からの信頼度の取得
     * @return 王様からの信頼度（範囲: 0-100）
     */
    int getKingTrust() const { return playerStats->getExtendedStats().kingTrust; }
    
    /**
     * @brief 王様からの信頼度の設定
     * @param value 信頼度（範囲: 0-100）
     */
    void setKingTrust(int value) {
        auto& stats = playerStats->getExtendedStats();
        stats.kingTrust = std::max(0, std::min(100, value));
    }
    
    /**
     * @brief メンタルの変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeMental(int amount) { playerStats->changeMental(amount); }
    
    /**
     * @brief 魔王からの信頼度の変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeDemonTrust(int amount) { playerStats->changeDemonTrust(amount); }
    
    /**
     * @brief 王様からの信頼度の変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeKingTrust(int amount) { playerStats->changeKingTrust(amount); }
    
    /**
     * @brief 現在の夜の回数の取得
     * @return 現在の夜の回数
     */
    int getCurrentNight() const { return currentNight; }
    
    /**
     * @brief 現在の夜の回数の設定
     * @param night 夜の回数
     */
    void setCurrentNight(int night) { currentNight = night; }
    
    /**
     * @brief 倒した住民の位置の取得
     * @return 倒した住民の位置のリスト
     */
    const std::vector<std::pair<int, int>>& getKilledResidents() const { return killedResidents; }
    
    /**
     * @brief 倒した住民の位置を追加
     * @param x X座標
     * @param y Y座標
     */
    void addKilledResident(int x, int y) { killedResidents.push_back({x, y}); }
    
    /**
     * @brief 倒した住民の位置をクリア
     */
    void clearKilledResidents() { killedResidents.clear(); }
    
    /**
     * @brief ゲームの保存
     * @param filename ファイル名
     * @param nightTimer 夜のタイマー
     * @param nightTimerActive 夜のタイマーがアクティブか
     */
    void saveGame(const std::string& filename, float nightTimer = 0.0f, bool nightTimerActive = false);
    
    /**
     * @brief ゲームの読み込み
     * @param filename ファイル名
     * @param nightTimer 夜のタイマー（参照渡し）
     * @param nightTimerActive 夜のタイマーがアクティブか（参照渡し）
     * @return 読み込みが成功したか
     */
    bool loadGame(const std::string& filename, float& nightTimer, bool& nightTimerActive);
    
    /**
     * @brief 自動保存
     */
    void autoSave();
    
    /**
     * @brief 自動読み込み
     * @param nightTimer 夜のタイマー（参照渡し）
     * @param nightTimerActive 夜のタイマーがアクティブか（参照渡し）
     * @return 読み込みが成功したか
     */
    bool autoLoad(float& nightTimer, bool& nightTimerActive);
    
    /**
     * @brief レベルの設定
     * @param newLevel 新しいレベル
     */
    void setLevel(int newLevel);
    
    /**
     * @brief カウンター効果が有効か
     * @return カウンター効果が有効か
     */
    bool hasCounterEffectActive() const { return playerStats->getSpellEffects().hasCounterEffect; }
    
    /**
     * @brief カウンター効果の設定
     * @param active 有効にするか
     */
    void setCounterEffect(bool active) { playerStats->setCounterEffect(active); }
    
    /**
     * @brief 次のターンボーナスが有効か
     * @return 次のターンボーナスが有効か
     */
    bool hasNextTurnBonusActive() const { return playerStats->getSpellEffects().hasNextTurnBonus; }
    
    /**
     * @brief 次のターンの倍率の取得
     * @return 次のターンの倍率
     */
    float getNextTurnMultiplier() const { return playerStats->getSpellEffects().nextTurnMultiplier; }
    
    /**
     * @brief 次のターンボーナスの残りターン数の取得
     * @return 残りターン数
     */
    int getNextTurnBonusTurns() const { return playerStats->getSpellEffects().nextTurnBonusTurns; }
    
    /**
     * @brief 次のターンボーナスの設定
     * @param active 有効にするか
     * @param multiplier 倍率（デフォルト: 1.0f）
     * @param turns 残りターン数（デフォルト: 1）
     */
    void setNextTurnBonus(bool active, float multiplier = 1.0f, int turns = 1) { 
        playerStats->setNextTurnBonus(active, multiplier, turns);
    }
    
    /**
     * @brief 次のターンボーナスの処理
     */
    void processNextTurnBonus() { playerStats->processNextTurnBonus(); }
    
    /**
     * @brief 次のターンボーナスのクリア
     */
    void clearNextTurnBonus() { playerStats->clearNextTurnBonus(); }
}; 