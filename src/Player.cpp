#include "Player.h"
#include <iostream>
#include <random>

Player::Player(const std::string& name)
    : Character(name, 30, 20, 8, 3, 1), gold(0), inventory(20), equipmentManager(),
      hasLevelUpStoryToShow(false), levelUpStoryLevel(0),
      trustLevel(50), isEvil(true), evilActions(0), goodActions(0), isNightTime(false),
      mental(100), demonTrust(50), kingTrust(50) {
    // 初期呪文を覚える
    learnSpell(SpellType::HEAL, 8); // レベル1 × 8 = 8MP
    learnSpell(SpellType::FIREBALL, 8); // レベル1 × 8 = 8MP
    
    // 初期アイテムを追加
    addStartingItems();
}

void Player::levelUp() {
    level++;
    int hpIncrease = 5;
    int mpIncrease = 1;
    int attackIncrease = 2;
    int defenseIncrease = 2;
    
    setMaxHp(getMaxHp() + hpIncrease);
    setMaxMp(getMaxMp() + mpIncrease);
    setAttack(getAttack() + attackIncrease);
    setDefense(getDefense() + defenseIncrease);
    
    // HPとMPを全回復
    hp = maxHp;
    mp = maxMp;
    
    // 新しい呪文を覚える
    if (level == 3) {
        learnSpell(SpellType::LIGHTNING, 24); // レベル3 × 8 = 24MP
    }
    if (level == 4) {
        learnSpell(SpellType::POISON_DART, 32); // レベル4 × 8 = 32MP
    }
    
    // レベルアップストーリーフラグを設定
    levelUpStoryLevel = level;
    hasLevelUpStoryToShow = true;
}

void Player::displayInfo() const {
    displayStatus();
    
    // 装備ボーナス込みの表示
    int totalAttack = getTotalAttack();
    int totalDefense = getTotalDefense();
    int equipAttackBonus = equipmentManager.getTotalAttackBonus();
    int equipDefenseBonus = equipmentManager.getTotalDefenseBonus();
    
    for (const auto& spell : spells) {
        int mpCost = 0;
        switch (spell.first) {
            case SpellType::HEAL:
                mpCost = 3; // ホイミ: レベル×2
                break;
            case SpellType::FIREBALL:
                mpCost = 6; // メラ: レベル×4
                break;
            case SpellType::LIGHTNING:
                mpCost = 10; // いなずま: レベル×6
                break;
            case SpellType::POISON_DART:
                mpCost = 4; // 毒の針: レベル×4
                break;
        }
    }
}

void Player::gainExp(int expGained) {
    exp += expGained;
    
    // レベルアップ判定（必要経験値 = 10固定、最大レベル100）
    while (exp >= 10 && level < 100) {
        exp -= 10;
        levelUp();
    }
    
    if (level >= 100) {
        
    }
}

void Player::gainGold(int goldGained) {
    gold += goldGained;
}

bool Player::canCastSpell(SpellType spell) const {
    auto it = spells.find(spell);
    if (it == spells.end()) return false;
    
    // 呪文ごとに異なるMP消費を計算
    int requiredMp = 0;
    switch (spell) {
        case SpellType::HEAL:
            requiredMp = 2; // ホイミ: レベル×2
            break;
        case SpellType::FIREBALL:
            requiredMp = 6; // メラ: レベル×4
            break;
        case SpellType::LIGHTNING:
            requiredMp = 10; // いなずま: レベル×6
            break;
        case SpellType::POISON_DART:
            requiredMp = 4; // 毒の針: レベル×4
            break;
    }
    return mp >= requiredMp;
}

int Player::castSpell(SpellType spell, Character* target) {
    if (!canCastSpell(spell)) {
        return 0;
    }
    
    // 呪文ごとに異なるMP消費を計算
    int mpCost = 0;
    switch (spell) {
        case SpellType::HEAL:
            mpCost = 2; // ホイミ: レベル×2
            break;
        case SpellType::FIREBALL:
            mpCost = 6; // メラ: レベル×4
            break;
        case SpellType::LIGHTNING:
            mpCost = 10; // いなずま: レベル×6
            break;
        case SpellType::POISON_DART:
            mpCost = 4; // 毒の針: レベル×4
            break;
    }
    mp -= mpCost;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    switch (spell) {
        case SpellType::HEAL:
            {
                int healAmount = 15 + (level * 2);
                heal(healAmount);
                return healAmount;
            }
            break;
        case SpellType::FIREBALL:
            if (target) {
                int baseDamage = getAttack() * 1.25;
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                return finalDamage;
            }
            break;
        case SpellType::LIGHTNING:
            if (target) {
                int baseDamage = getAttack() * 1.5;
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                return finalDamage;
            }
            break;
        case SpellType::POISON_DART:
            if (target) {
                int baseDamage = getAttack() * 2;
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                target->applyStatusEffect(StatusEffect::POISON, 3); // 3ターン毒
                return finalDamage;
            }
            break;
    }
    return 0; // デフォルトの戻り値
}

void Player::learnSpell(SpellType spell, int mpCost) {
    spells[spell] = mpCost;
}

void Player::addStartingItems() {
    // 初期アイテムを追加
    auto yakusou = std::make_unique<ConsumableItem>(ConsumableType::YAKUSOU);
    inventory.addItem(std::move(yakusou), 3);
    
    auto seisui = std::make_unique<ConsumableItem>(ConsumableType::SEISUI);
    inventory.addItem(std::move(seisui), 2);
    
    // 初期装備を追加
    auto woodenStick = std::make_unique<Weapon>(WeaponType::WOODEN_STICK);
    equipmentManager.equipItem(std::move(woodenStick));
    
    auto clothArmor = std::make_unique<Armor>(ArmorType::CLOTH_ARMOR);
    equipmentManager.equipItem(std::move(clothArmor));
}

void Player::showInventory() const {
    inventory.displayInventory();
}

bool Player::useItem(int itemIndex, Character* target) {
    // アイテムインデックスは1ベースなので0ベースに変換
    return inventory.useItem(itemIndex - 1, const_cast<Player*>(this), target);
}

void Player::showEquipment() const {
    equipmentManager.displayEquipment();
}

bool Player::equipItem(std::unique_ptr<Equipment> equipment) {
    return equipmentManager.equipItem(std::move(equipment));
}

bool Player::unequipItem(EquipmentSlot slot) {
    auto unequipped = equipmentManager.unequipItem(slot);
    if (unequipped) {
        // 外した装備をインベントリに戻す
        return inventory.addItem(std::move(unequipped));
    }
    return false;
}

int Player::getTotalAttack() const {
    return getAttack() + equipmentManager.getTotalAttackBonus();
}

int Player::getTotalDefense() const {
    return getDefense() + equipmentManager.getTotalDefenseBonus();
}

int Player::getTotalMaxHp() const {
    return getMaxHp() + equipmentManager.getTotalHpBonus();
}

int Player::getTotalMaxMp() const {
    return getMaxMp() + equipmentManager.getTotalMpBonus();
}

int Player::getEffectiveAttack() const {
    return getTotalAttack();
}

int Player::getEffectiveDefense() const {
    return getTotalDefense();
}

int Player::attack(Character& target) {
    if (!isAlive) return 0;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> criticalDis(1, 100);
    
    int baseDamage = calculateDamage(target);
    int damage = baseDamage;
    bool isCritical = false;
    
    // 会心の一撃判定（8%の確率）
    if (criticalDis(gen) <= 8) {
        damage = baseDamage * 2;
        isCritical = true;
    } else {

    }
    
    target.takeDamage(damage);
    return damage;
}

void Player::defend() {
    
    // 次のターンのダメージを半減（実装は戦闘システムで）
}

bool Player::tryToEscape() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    bool escaped = dis(gen) <= 70; // 70%の確率で逃走成功
    return escaped;
}

// ストーリーシステム
std::vector<std::string> Player::getOpeningStory() const {
    std::vector<std::string> story;
    story.push_back("王様からの緊急依頼");
    story.push_back("");
    story.push_back("勇者" + name + "よ、我が国に危機が...");
    story.push_back("邪悪な魔王が復活し、モンスターが各地で暴れている！");
    story.push_back("どうか魔王を倒し、平和を取り戻してくれないか！");
    story.push_back("【目標】レベル3で森のボス戦！");
    return story;
}

std::vector<std::string> Player::getLevelUpStory(int newLevel) const {
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

// 信頼度システムの実装
void Player::changeTrustLevel(int amount) {
    trustLevel += amount;
    if (trustLevel > 100) trustLevel = 100;
    if (trustLevel < 0) trustLevel = 0;
}

void Player::performEvilAction() {
    evilActions++;
    changeTrustLevel(-5); // 悪行で信頼度が下がる
}

void Player::performGoodAction() {
    goodActions++;
    changeTrustLevel(3); // 善行で信頼度が上がる
}

void Player::setNightTime(bool night) {
    isNightTime = night;
}

void Player::toggleNightTime() {
    isNightTime = !isNightTime;
}

// 新しいパラメータ変更メソッド
void Player::changeMental(int amount) {
    mental = std::max(0, std::min(100, mental + amount));
}

void Player::changeDemonTrust(int amount) {
    demonTrust = std::max(0, std::min(100, demonTrust + amount));
}

void Player::changeKingTrust(int amount) {
    kingTrust = std::max(0, std::min(100, kingTrust + amount));
} 