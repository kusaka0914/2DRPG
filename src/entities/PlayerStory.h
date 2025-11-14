/**
 * @file PlayerStory.h
 * @brief プレイヤーのストーリー管理を担当するクラス
 * @details プレイヤーのストーリー（オープニング、レベルアップストーリー）を管理する。
 * 単一責任の原則に従い、ストーリー管理をPlayerクラスから分離している。
 */

#pragma once
#include <string>
#include <vector>

/**
 * @brief プレイヤーのストーリー管理を担当するクラス
 * @details プレイヤーのストーリー（オープニング、レベルアップストーリー）を管理する。
 * 単一責任の原則に従い、ストーリー管理をPlayerクラスから分離している。
 */
class PlayerStory {
private:
    bool hasLevelUpStoryToShow;
    int levelUpStoryLevel;
    std::string playerName;

public:
    /**
     * @brief コンストラクタ
     * @param playerName プレイヤー名
     */
    PlayerStory(const std::string& playerName);
    
    /**
     * @brief レベルアップストーリーがあるか
     * @return レベルアップストーリーがあるか
     */
    bool hasLevelUpStory() const { return hasLevelUpStoryToShow; }
    
    /**
     * @brief レベルアップストーリーのレベル取得
     * @return レベルアップストーリーのレベル
     */
    int getLevelUpStoryLevel() const { return levelUpStoryLevel; }
    
    /**
     * @brief レベルアップストーリーフラグのクリア
     */
    void clearLevelUpStoryFlag() { hasLevelUpStoryToShow = false; }
    
    /**
     * @brief レベルアップストーリーの設定
     * @param newLevel 新しいレベル
     */
    void setLevelUpStory(int newLevel) {
        levelUpStoryLevel = newLevel;
        hasLevelUpStoryToShow = true;
    }
    
    /**
     * @brief オープニングストーリー取得
     * @return オープニングストーリーの文字列ベクター
     */
    std::vector<std::string> getOpeningStory() const;
    
    /**
     * @brief レベルアップストーリー取得
     * @param newLevel 新しいレベル
     * @return レベルアップストーリーの文字列ベクター
     */
    std::vector<std::string> getLevelUpStory(int newLevel) const;
};

