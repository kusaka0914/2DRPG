#include "Player.h"
#include <iostream>
#include <random>

Player::Player(const std::string& name) 
    : Character(name, 50, 20, 15, 8, 1), gold(100), inventory(20), hasLevelUpStoryToShow(false), levelUpStoryLevel(1) {
    // 初期呪文を覚える
    learnSpell(SpellType::HEAL, 3);
    learnSpell(SpellType::FIREBALL, 5);
    
    // 初期アイテムを追加
    addStartingItems();
}

void Player::levelUp() {
    level++;
    int hpIncrease = 8 + (level * 2);
    int mpIncrease = 3 + level;
    int attackIncrease = 3 + (level / 2);
    int defenseIncrease = 2 + (level / 3);
    
    setMaxHp(getMaxHp() + hpIncrease);
    setMaxMp(getMaxMp() + mpIncrease);
    setAttack(getAttack() + attackIncrease);
    setDefense(getDefense() + defenseIncrease);
    
    // HPとMPを全回復
    hp = maxHp;
    mp = maxMp;
    
    std::cout << "\n★レベルアップ！★" << std::endl;
    std::cout << name << "はレベル" << level << "になった！" << std::endl;
    std::cout << "HP+" << hpIncrease << " MP+" << mpIncrease 
              << " 攻撃力+" << attackIncrease << " 防御力+" << defenseIncrease << std::endl;
    
    // 新しい呪文を覚える
    if (level == 3) {
        learnSpell(SpellType::LIGHTNING, 8);
        std::cout << "新しい呪文「いなずま」を覚えた！" << std::endl;
    }
    if (level == 4) {
        learnSpell(SpellType::POISON_DART, 6);
        std::cout << "新しい呪文「毒の針」を覚えた！" << std::endl;
    }
    
    // レベルアップストーリーフラグを設定
    levelUpStoryLevel = level;
    hasLevelUpStoryToShow = true;
}

void Player::displayInfo() const {
    displayStatus();
    std::cout << "ゴールド: " << gold << std::endl;
    
    // 装備ボーナス込みの表示
    int totalAttack = getTotalAttack();
    int totalDefense = getTotalDefense();
    int equipAttackBonus = equipmentManager.getTotalAttackBonus();
    int equipDefenseBonus = equipmentManager.getTotalDefenseBonus();
    
    if (equipAttackBonus > 0) {
        std::cout << "攻撃力: " << getAttack() << "+" << equipAttackBonus << "=" << totalAttack;
    } else {
        std::cout << "攻撃力: " << totalAttack;
    }
    
    if (equipDefenseBonus > 0) {
        std::cout << " 防御力: " << getDefense() << "+" << equipDefenseBonus << "=" << totalDefense << std::endl;
    } else {
        std::cout << " 防御力: " << totalDefense << std::endl;
    }
    
    std::cout << "習得呪文: ";
    for (const auto& spell : spells) {
        switch (spell.first) {
            case SpellType::HEAL:
                std::cout << "ホイミ(" << spell.second << "MP) ";
                break;
            case SpellType::FIREBALL:
                std::cout << "メラ(" << spell.second << "MP) ";
                break;
            case SpellType::LIGHTNING:
                std::cout << "いなずま(" << spell.second << "MP) ";
                break;
            case SpellType::POISON_DART:
                std::cout << "毒の針(" << spell.second << "MP) ";
                break;
        }
    }
    std::cout << std::endl;
}

void Player::gainExp(int expGained) {
    exp += expGained;
    std::cout << expGained << "の経験値を得た！" << std::endl;
    
    // レベルアップ判定（必要経験値 = レベル * 10）
    int requiredExp = level * 10;
    if (exp >= requiredExp) {
        exp -= requiredExp;
        levelUp();
    }
}

void Player::gainGold(int goldGained) {
    gold += goldGained;
    if (goldGained > 0) {
        std::cout << goldGained << "ゴールドを手に入れた！" << std::endl;
    } else {
        std::cout << (-goldGained) << "ゴールドを支払った。" << std::endl;
    }
}

bool Player::canCastSpell(SpellType spell) const {
    auto it = spells.find(spell);
    return it != spells.end() && mp >= it->second && isAlive;
}

int Player::castSpell(SpellType spell, Character* target) {
    if (!canCastSpell(spell)) {
        std::cout << "MPが足りない、または呪文を覚えていない！" << std::endl;
        return 0;
    }
    
    mp -= spells.at(spell);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    switch (spell) {
        case SpellType::HEAL:
            {
                int healAmount = 15 + (level * 2);
                heal(healAmount);
                std::cout << name << "はホイミを唱えた！" << std::endl;
                return healAmount;
            }
            break;
        case SpellType::FIREBALL:
            if (target) {
                int damage = 12 + (level * 3);
                std::cout << name << "はメラを唱えた！" << std::endl;
                target->takeDamage(damage);
                return damage;
            }
            break;
        case SpellType::LIGHTNING:
            if (target) {
                int damage = 20 + (level * 4);
                std::cout << name << "はいなずまを唱えた！" << std::endl;
                target->takeDamage(damage);
                return damage;
            }
            break;
        case SpellType::POISON_DART:
            if (target) {
                int damage = 8 + (level * 2);
                std::cout << name << "は毒の針を唱えた！" << std::endl;
                target->takeDamage(damage);
                target->applyStatusEffect(StatusEffect::POISON, 3); // 3ターン毒
                std::cout << target->getName() << "は毒におかされた！" << std::endl;
                return damage;
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
        std::cout << name << "の攻撃！【会心の一撃！】" << std::endl;
    } else {
        std::cout << name << "の攻撃！" << std::endl;
    }
    
    target.takeDamage(damage);
    return damage;
}

void Player::defend() {
    std::cout << name << "は身を守っている..." << std::endl;
    // 次のターンのダメージを半減（実装は戦闘システムで）
}

bool Player::tryToEscape() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    bool escaped = dis(gen) <= 70; // 70%の確率で逃走成功
    if (escaped) {
        std::cout << name << "は逃げ出した！" << std::endl;
    } else {
        std::cout << name << "は逃げ出そうとしたが、失敗した！" << std::endl;
    }
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