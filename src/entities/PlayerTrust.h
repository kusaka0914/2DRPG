/**
 * @file PlayerTrust.h
 * @brief プレイヤーの信頼度管理を担当するクラス
 * @details プレイヤーの信頼度、善悪の判定、行動の記録を管理する。
 * 単一責任の原則に従い、信頼度管理をPlayerクラスから分離している。
 */

#pragma once

/**
 * @brief プレイヤーの信頼度管理を担当するクラス
 * @details プレイヤーの信頼度、善悪の判定、行動の記録を管理する。
 * 単一責任の原則に従い、信頼度管理をPlayerクラスから分離している。
 */
class PlayerTrust {
private:
    int trustLevel; // 王様からの信頼度 (0-100)
    bool isEvil; // 魔王の手下かどうか
    int evilActions; // 悪行の回数
    int goodActions; // 善行の回数

public:
    /**
     * @brief コンストラクタ
     * @param initialTrustLevel 初期信頼度（デフォルト: 50）
     * @param initialIsEvil 初期の善悪フラグ（デフォルト: true）
     */
    PlayerTrust(int initialTrustLevel = 50, bool initialIsEvil = true);
    
    /**
     * @brief 信頼度の取得
     * @return 現在の信頼度
     */
    int getTrustLevel() const { return trustLevel; }
    
    /**
     * @brief 信頼度の変更
     * @param amount 変更量（正の値で増加、負の値で減少）
     */
    void changeTrustLevel(int amount);
    
    /**
     * @brief 悪キャラクターかどうか
     * @return 悪キャラクターかどうか
     */
    bool isEvilCharacter() const { return isEvil; }
    
    /**
     * @brief 善悪フラグの設定
     * @param evil 悪キャラクターかどうか
     */
    void setEvil(bool evil) { isEvil = evil; }
    
    /**
     * @brief 悪行の記録
     */
    void recordEvilAction();
    
    /**
     * @brief 善行の記録
     */
    void recordGoodAction();
    
    /**
     * @brief 悪行の回数取得
     * @return 悪行の回数
     */
    int getEvilActions() const { return evilActions; }
    
    /**
     * @brief 善行の回数取得
     * @return 善行の回数
     */
    int getGoodActions() const { return goodActions; }
    
    /**
     * @brief シリアライゼーション用データ構造
     * @details セーブ・ロード時に信頼度関連のデータを保存・復元するための構造体。
     */
    struct TrustData {
        int trustLevel;  /**< @brief 信頼度（範囲: 0-100） */
        int evilActions;
        int goodActions;
    };
    
    /**
     * @brief 信頼度データの取得
     * @return 信頼度データ
     */
    TrustData getTrustData() const {
        return {trustLevel, evilActions, goodActions};
    }
    
    /**
     * @brief 信頼度データの設定
     * @param data 信頼度データ
     */
    void setTrustData(const TrustData& data) {
        trustLevel = data.trustLevel;
        evilActions = data.evilActions;
        goodActions = data.goodActions;
    }
};

