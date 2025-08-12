#include "Player.h"
#include <iostream>
#include <random>
#include <fstream>

Player::Player(const std::string& name)
    : Character(name, 30, 20, 8, 3, 1), gold(0), inventory(20), equipmentManager(),
      hasLevelUpStoryToShow(false), levelUpStoryLevel(0),
      trustLevel(50), isEvil(true), evilActions(0), goodActions(0), isNightTime(false),
      mental(100), demonTrust(50), kingTrust(50), currentNight(0),
      hasCounterEffect(false), hasNextTurnBonus(false), nextTurnMultiplier(1.0f), nextTurnBonusTurns(0) {
    // 初期アイテムを追加
    addStartingItems();
}

void Player::setLevel(int newLevel) {
    level = newLevel;
}

void Player::levelUp() {
    level++;
    int hpIncrease = 5;
    int mpIncrease = 1;
    int attackIncrease = 2;
    int defenseIncrease = 1;
    
    setMaxHp(getMaxHp() + hpIncrease);
    setMaxMp(getMaxMp() + mpIncrease);
    setAttack(getAttack() + attackIncrease);
    setDefense(getDefense() + defenseIncrease);
    
    // HPとMPを全回復
    hp = maxHp;
    mp = maxMp;
    
    // 新しい呪文を覚える
    if (level == 3) {
        learnSpell(SpellType::KIZUGAIAERU, 10); // キズガイエール
        learnSpell(SpellType::ATSUIATSUI, 4); // アツイアツーイ
    }
    if (level == 10) {
        learnSpell(SpellType::BIRIBIRIDOKKAN, 8); // ビリビリドッカーン
        learnSpell(SpellType::ICHIKABACHIKA, 6); // イチカバチーカ
    }
    if (level == 30) {
        learnSpell(SpellType::TSUGICHOTTOTSUYOI, 5); // ツギチョットツヨーイ
        learnSpell(SpellType::DARKNESSIMPACT, 12); // ダークネスインパクト
    }
    if (level == 60) {
        learnSpell(SpellType::TSUGIMECHATSUYOI, 8); // ツギメッチャツヨーイ
        learnSpell(SpellType::WANCHANTAOSERU, 15); // ワンチャンタオセール
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
        // 新しい呪文のMP消費を表示
        int mpCost = 0;
        switch (spell.first) {
            case SpellType::KIZUGAIAERU:
                mpCost = 10; // キズガイエール: レベル×3
                break;
            case SpellType::ATSUIATSUI:
                mpCost = 4; // アツイアツーイ: レベル×4
                break;
            case SpellType::BIRIBIRIDOKKAN:
                mpCost = 8; // ビリビリドッカーン: レベル×8
                break;
            case SpellType::DARKNESSIMPACT:
                mpCost = 12; // ダークネスインパクト: レベル×12
                break;
            case SpellType::ICHIKABACHIKA:
                mpCost = 4; // イチカバチーカ: レベル×6
                break;
            case SpellType::TSUGICHOTTOTSUYOI:
                mpCost = 2; // ツギチョットツヨーイ: レベル×5
                break;
            case SpellType::TSUGIMECHATSUYOI:
                mpCost = 4; // ツギメッチャツヨーイ: レベル×8
                break;
            case SpellType::WANCHANTAOSERU:
                mpCost = 8; // ワンチャンタオセール: レベル×15
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
        case SpellType::KIZUGAIAERU:
            requiredMp = 10; // キズガイエール: レベル×3
            break;
        case SpellType::ATSUIATSUI:
            requiredMp = 4; // アツイアツーイ: レベル×4
            break;
        case SpellType::BIRIBIRIDOKKAN:
            requiredMp = 8; // ビリビリドッカーン: レベル×8
            break;
        case SpellType::DARKNESSIMPACT:
            requiredMp = 12; // ダークネスインパクト: レベル×12
            break;
        case SpellType::ICHIKABACHIKA:
            requiredMp = 4; // イチカバチーカ: レベル×6
            break;
        case SpellType::TSUGICHOTTOTSUYOI:
            requiredMp = 2; // ツギチョットツヨーイ: レベル×5
            break;
        case SpellType::TSUGIMECHATSUYOI:
            requiredMp = 4; // ツギメッチャツヨーイ: レベル×8
            break;
        case SpellType::WANCHANTAOSERU:
            requiredMp = 8; // ワンチャンタオセール: レベル×15
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
        case SpellType::KIZUGAIAERU:
            mpCost = 10; // キズガイエール: レベル×3
            break;
        case SpellType::ATSUIATSUI:
            mpCost = 4; // アツイアツーイ: レベル×4
            break;
        case SpellType::BIRIBIRIDOKKAN:
            mpCost = 8; // ビリビリドッカーン: レベル×8
            break;
        case SpellType::DARKNESSIMPACT:
            mpCost = 12; // ダークネスインパクト: レベル×12
            break;
        case SpellType::ICHIKABACHIKA:
            mpCost = 4; // イチカバチーカ: レベル×6
            break;
        case SpellType::TSUGICHOTTOTSUYOI:
            mpCost = 2; // ツギチョットツヨーイ: レベル×5
            break;
        case SpellType::TSUGIMECHATSUYOI:
            mpCost = 4; // ツギメッチャツヨーイ: レベル×8
            break;
        case SpellType::WANCHANTAOSERU:
            mpCost = 8; // ワンチャンタオセール: レベル×15
            break;
    }
    mp -= mpCost;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    switch (spell) {
        case SpellType::KIZUGAIAERU:
            {
                int healAmount = static_cast<int>(getMaxHp() * 0.8); // 体力の20%回復
                heal(healAmount);
                return healAmount;
            }
            break;
        case SpellType::ATSUIATSUI:
            if (target) {
                int baseAttack = getTotalAttack();
                if (hasNextTurnBonusActive()) {
                    baseAttack = static_cast<int>(baseAttack * nextTurnMultiplier);
                }
                int baseDamage = baseAttack * 1.25; // 低MP攻撃呪文
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                return finalDamage;
            }
            break;
        case SpellType::BIRIBIRIDOKKAN:
            if (target) {
                int baseAttack = getTotalAttack();
                if (hasNextTurnBonusActive()) {
                    baseAttack = static_cast<int>(baseAttack * nextTurnMultiplier);
                }
                int baseDamage = baseAttack * 1.5; // 中MP攻撃呪文
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                return finalDamage;
            }
            break;
        case SpellType::DARKNESSIMPACT:
            if (target) {
                int baseAttack = getTotalAttack();
                if (hasNextTurnBonusActive()) {
                    baseAttack = static_cast<int>(baseAttack * nextTurnMultiplier);
                }
                int baseDamage = baseAttack * 2; // 高MP攻撃呪文
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                return finalDamage;
            }
            break;
        case SpellType::ICHIKABACHIKA:
            {
                // 50%の確率でカウンター効果を発動
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(gen) < 0.5f) {
                    hasCounterEffect = true;
                    return 1; // 成功を示す
                } else {
                    return 0; // 失敗を示す
                }
            }
            break;
        case SpellType::TSUGICHOTTOTSUYOI:
            {
                // 80%の確率で次のターンの攻撃が2.5倍になる
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(gen) < 0.8f) {
                    setNextTurnBonus(true, 1.8f, 1);
                    return 1; // 成功を示す
                } else {
                    return 0; // 失敗を示す
                }
            }
            break;
        case SpellType::TSUGIMECHATSUYOI:
            {
                // 50%の確率で次のターンの攻撃が3倍になる
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(gen) < 0.5f) {
                    setNextTurnBonus(true, 2.5f, 1);
                    return 1; // 成功を示す
                } else {
                    return 0; // 失敗を示す
                }
            }
            break;
        case SpellType::WANCHANTAOSERU:
            if (target) {
                // 5%の確率で即死
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(gen) < 100.0f) {
                    target->takeDamage(target->getHp()); // 現在のHP分のダメージ
                    return target->getHp();
                } else {
                    // 失敗した場合は何もしない
                    return 0;
                }
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

int Player::calculateDamageWithBonus(const Character& target) const {
    // 基本ダメージ計算（攻撃力 - 相手の防御力）
    int baseAttack = getTotalAttack();
    if (hasNextTurnBonusActive()) {
        baseAttack = static_cast<int>(baseAttack * nextTurnMultiplier);
    }
    
    int baseDamage = baseAttack - target.getEffectiveDefense();
    
    return baseDamage;
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

void Player::setNextTurnBonus(bool active, float multiplier, int turns) {
    hasNextTurnBonus = active;
    if (active) {
        nextTurnMultiplier = multiplier;
        nextTurnBonusTurns = turns;
    } else {
        nextTurnMultiplier = 1.0f;
        nextTurnBonusTurns = 0;
    }
}

void Player::processNextTurnBonus() {
    if (hasNextTurnBonus && nextTurnBonusTurns > 0) {
        nextTurnBonusTurns--;
        if (nextTurnBonusTurns <= 0) {
            clearNextTurnBonus();
        }
    }
}

void Player::clearNextTurnBonus() {
    hasNextTurnBonus = false;
    nextTurnMultiplier = 1.0f;
    nextTurnBonusTurns = 0;
}

// セーブ/ロード機能の実装
void Player::saveGame(const std::string& filename, float nightTimer, bool nightTimerActive) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return;
    }
    
    // 基本ステータス
    file.write(reinterpret_cast<const char*>(&level), sizeof(level));
    file.write(reinterpret_cast<const char*>(&exp), sizeof(exp));
    file.write(reinterpret_cast<const char*>(&hp), sizeof(hp));
    file.write(reinterpret_cast<const char*>(&mp), sizeof(mp));
    file.write(reinterpret_cast<const char*>(&maxHp), sizeof(maxHp));
    file.write(reinterpret_cast<const char*>(&maxMp), sizeof(maxMp));
    int attackValue = getAttack();
    file.write(reinterpret_cast<const char*>(&attackValue), sizeof(attackValue));
    int defenseValue = getDefense();
    file.write(reinterpret_cast<const char*>(&defenseValue), sizeof(defenseValue));
    
    // プレイヤー専用ステータス
    file.write(reinterpret_cast<const char*>(&gold), sizeof(gold));
    file.write(reinterpret_cast<const char*>(&mental), sizeof(mental));
    file.write(reinterpret_cast<const char*>(&demonTrust), sizeof(demonTrust));
    file.write(reinterpret_cast<const char*>(&kingTrust), sizeof(kingTrust));
    file.write(reinterpret_cast<const char*>(&trustLevel), sizeof(trustLevel));
    file.write(reinterpret_cast<const char*>(&evilActions), sizeof(evilActions));
    file.write(reinterpret_cast<const char*>(&goodActions), sizeof(goodActions));
    
    // 夜の情報
    file.write(reinterpret_cast<const char*>(&currentNight), sizeof(currentNight));
    int killedResidentsSize = killedResidents.size();
    file.write(reinterpret_cast<const char*>(&killedResidentsSize), sizeof(killedResidentsSize));
    for (const auto& pos : killedResidents) {
        file.write(reinterpret_cast<const char*>(&pos.first), sizeof(pos.first));
        file.write(reinterpret_cast<const char*>(&pos.second), sizeof(pos.second));
    }
    
    // 名前の長さと名前
    int nameLength = name.length();
    file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
    file.write(name.c_str(), nameLength);
    
    // インベントリと装備の保存
    inventory.saveToFile(file);
    equipmentManager.saveToFile(file);

    // タイマー情報の保存
    file.write(reinterpret_cast<const char*>(&nightTimer), sizeof(nightTimer));
    file.write(reinterpret_cast<const char*>(&nightTimerActive), sizeof(nightTimerActive));
    
    file.close();
}

bool Player::loadGame(const std::string& filename, float& nightTimer, bool& nightTimerActive) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // 基本ステータス
    file.read(reinterpret_cast<char*>(&level), sizeof(level));
    file.read(reinterpret_cast<char*>(&exp), sizeof(exp));
    file.read(reinterpret_cast<char*>(&hp), sizeof(hp));
    file.read(reinterpret_cast<char*>(&mp), sizeof(mp));
    file.read(reinterpret_cast<char*>(&maxHp), sizeof(maxHp));
    file.read(reinterpret_cast<char*>(&maxMp), sizeof(maxMp));
    int attackValue;
    file.read(reinterpret_cast<char*>(&attackValue), sizeof(attackValue));
    setAttack(attackValue);
    int defenseValue;
    file.read(reinterpret_cast<char*>(&defenseValue), sizeof(defenseValue));
    setDefense(defenseValue);
    
    // プレイヤー専用ステータス
    file.read(reinterpret_cast<char*>(&gold), sizeof(gold));
    file.read(reinterpret_cast<char*>(&mental), sizeof(mental));
    file.read(reinterpret_cast<char*>(&demonTrust), sizeof(demonTrust));
    file.read(reinterpret_cast<char*>(&kingTrust), sizeof(kingTrust));
    file.read(reinterpret_cast<char*>(&trustLevel), sizeof(trustLevel));
    file.read(reinterpret_cast<char*>(&evilActions), sizeof(evilActions));
    file.read(reinterpret_cast<char*>(&goodActions), sizeof(goodActions));
    
    // 夜の情報
    file.read(reinterpret_cast<char*>(&currentNight), sizeof(currentNight));
    int killedResidentsSize;
    file.read(reinterpret_cast<char*>(&killedResidentsSize), sizeof(killedResidentsSize));
    killedResidents.clear();
    for (int i = 0; i < killedResidentsSize; ++i) {
        int x, y;
        file.read(reinterpret_cast<char*>(&x), sizeof(x));
        file.read(reinterpret_cast<char*>(&y), sizeof(y));
        killedResidents.push_back({x, y});
    }
    
    // 名前の長さと名前
    int nameLength;
    file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
    char* nameBuffer = new char[nameLength + 1];
    file.read(nameBuffer, nameLength);
    nameBuffer[nameLength] = '\0';
    name = std::string(nameBuffer);
    delete[] nameBuffer;
    
    // インベントリと装備の読み込み
    inventory.loadFromFile(file);
    equipmentManager.loadFromFile(file);

    // タイマー情報の読み込み
    file.read(reinterpret_cast<char*>(&nightTimer), sizeof(nightTimer));
    file.read(reinterpret_cast<char*>(&nightTimerActive), sizeof(nightTimerActive));
    
    file.close();
    return true;
}

void Player::autoSave() {
    // タイマー情報も含めてセーブ
    saveGame("autosave.dat");
}

bool Player::autoLoad(float& nightTimer, bool& nightTimerActive) {
    return loadGame("autosave.dat", nightTimer, nightTimerActive);
}

// 呪文名を取得する関数
std::string Player::getSpellName(SpellType spell) {
    switch (spell) {
        case SpellType::KIZUGAIAERU:
            return "キズガイエール";
        case SpellType::ATSUIATSUI:
            return "アツイアツーイ";
        case SpellType::BIRIBIRIDOKKAN:
            return "ビリビリドッカーン";
        case SpellType::DARKNESSIMPACT:
            return "ダークネスインパクト";
        case SpellType::ICHIKABACHIKA:
            return "イチカバチーカ";
        case SpellType::TSUGICHOTTOTSUYOI:
            return "ツギチョットツヨーイ";
        case SpellType::TSUGIMECHATSUYOI:
            return "ツギメッチャツヨーイ";
        case SpellType::WANCHANTAOSERU:
            return "ワンチャンタオセール";
        default:
            return "不明な呪文";
    }
}

// 指定レベルで覚える呪文を取得する関数
std::vector<SpellType> Player::getSpellsLearnedAtLevel(int level) {
    std::vector<SpellType> spells;
    
    switch (level) {
        case 3:
            spells.push_back(SpellType::KIZUGAIAERU);
            spells.push_back(SpellType::ATSUIATSUI);
            break;
        case 10:
            spells.push_back(SpellType::BIRIBIRIDOKKAN);
            spells.push_back(SpellType::ICHIKABACHIKA);
            break;
        case 30:
            spells.push_back(SpellType::TSUGICHOTTOTSUYOI);
            spells.push_back(SpellType::DARKNESSIMPACT);
            break;
        case 60:
            spells.push_back(SpellType::TSUGIMECHATSUYOI);
            spells.push_back(SpellType::WANCHANTAOSERU);
            break;
    }
    
    return spells;
} 