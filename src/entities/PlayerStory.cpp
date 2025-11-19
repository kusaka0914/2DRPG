#include "PlayerStory.h"

PlayerStory::PlayerStory(const std::string& playerName)
    : hasLevelUpStoryToShow(false), levelUpStoryLevel(0), playerName(playerName) {
}

std::vector<std::string> PlayerStory::getOpeningStory() const {
    std::vector<std::string> story;
    story.push_back("王様からの緊急依頼");
    story.push_back("");
    story.push_back("勇者" + playerName + "よ、我が国に危機が...");
    story.push_back("邪悪な魔王が復活し、モンスターが各地で暴れている！");
    story.push_back("どうか魔王を倒し、平和を取り戻してくれないか！");
    story.push_back("【目標】レベル3で森のボス戦！");
    return story;
}

std::vector<std::string> PlayerStory::getLevelUpStory(int newLevel) const {
    std::vector<std::string> story;
    
    switch (newLevel) {
        case 2:
            story.push_back("📜 ストーリー更新！");
            story.push_back("");
            story.push_back("まだまだ弱い...もっと強くなる必要がある。");
            story.push_back("目標：レベル3で森のボスと戦えるようになる！");
            break;
            
        case 3:
            story.push_back("🌟 重要な節目に到達！");
            story.push_back("");
            story.push_back("ついに森のボス「ゴブリンキング」と戦う力がついた！");
            story.push_back("次の目標：レベル5で山のボス「オークロード」討伐！");
            break;
            
        case 5:
            story.push_back("⚔️ 中級勇者の証！");
            story.push_back("");
            story.push_back("山のボス「オークロード」と戦う準備が整った！");
            story.push_back("次の目標：レベル8で魔王城への挑戦権を得る！");
            break;
            
        case 8:
            story.push_back("👑 真の勇者への覚醒！");
            story.push_back("");
            story.push_back("ついに魔王「ドラゴンロード」と戦う力を得た！");
            story.push_back("最終目標：魔王を倒して世界に平和を取り戻せ！");
            break;
            
        default:
            if (newLevel >= 10) {
                story.push_back("🏆 伝説の勇者！");
                story.push_back("");
                story.push_back("もはや敵なし！魔王すら恐れる力を手に入れた！");
            }
            break;
    }
    
    return story;
}

