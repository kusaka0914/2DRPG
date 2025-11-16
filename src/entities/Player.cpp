#include "Player.h"
#include <iostream>
#include <random>
#include <fstream>

Player::Player(const std::string& name)
    : Character(name, 30, 20, 8, 3, 1), inventory(20), equipmentManager(),
      playerStats(std::make_unique<PlayerStats>()),
      playerStory(std::make_unique<PlayerStory>(name)),
      playerTrust(std::make_unique<PlayerTrust>(50, true)),
      isNightTime(false), currentNight(0) {
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
    
    hp = maxHp;
    mp = maxMp;
    
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
    
    playerStory->setLevelUpStory(level);
}

void Player::displayInfo() const {
    displayStatus();
    
    int totalAttack = getTotalAttack();
    int totalDefense = getTotalDefense();
    int equipAttackBonus = equipmentManager.getTotalAttackBonus();
    int equipDefenseBonus = equipmentManager.getTotalDefenseBonus();
    
    for (const auto& spell : spells) {
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
    
    while (exp >= 10 && level < 100) {
        exp -= 10;
        levelUp();
    }
    
    if (level >= 100) {
        
    }
}

bool Player::canCastSpell(SpellType spell) const {
    auto it = spells.find(spell);
    if (it == spells.end()) return false;
    
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
                    baseAttack = static_cast<int>(baseAttack * getNextTurnMultiplier());
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
                    baseAttack = static_cast<int>(baseAttack * getNextTurnMultiplier());
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
                    baseAttack = static_cast<int>(baseAttack * getNextTurnMultiplier());
                }
                int baseDamage = baseAttack * 2; // 高MP攻撃呪文
                int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                target->takeDamage(finalDamage);
                return finalDamage;
            }
            break;
        case SpellType::ICHIKABACHIKA:
            {
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(gen) < 0.5f) {
                    setCounterEffect(true);
                    return 1; // 成功を示す
                } else {
                    return 0; // 失敗を示す
                }
            }
            break;
        case SpellType::TSUGICHOTTOTSUYOI:
            {
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
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(gen) < 100.0f) {
                    target->takeDamage(target->getHp()); // 現在のHP分のダメージ
                    return target->getHp();
                } else {
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
    auto yakusou = std::make_unique<ConsumableItem>(ConsumableType::YAKUSOU);
    inventory.addItem(std::move(yakusou), 3);
    
    auto seisui = std::make_unique<ConsumableItem>(ConsumableType::SEISUI);
    inventory.addItem(std::move(seisui), 2);
    
    auto woodenStick = std::make_unique<Weapon>(WeaponType::WOODEN_STICK);
    equipmentManager.equipItem(std::move(woodenStick));
    
    auto clothArmor = std::make_unique<Armor>(ArmorType::CLOTH_ARMOR);
    equipmentManager.equipItem(std::move(clothArmor));
}

void Player::showInventory() const {
    inventory.displayInventory();
}

bool Player::useItem(int itemIndex, Character* target) {
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
    
    if (criticalDis(gen) <= 8) {
        damage = baseDamage * 2;
        isCritical = true;
    } else {

    }
    
    target.takeDamage(damage);
    return damage;
}

int Player::calculateDamageWithBonus(const Character& target) const {
    int baseAttack = getTotalAttack();
    if (hasNextTurnBonusActive()) {
        baseAttack = static_cast<int>(baseAttack * getNextTurnMultiplier());
    }
    
    int baseDamage = baseAttack - target.getEffectiveDefense();
    
    return baseDamage;
}

void Player::defend() {
    
}

bool Player::tryToEscape() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    bool escaped = dis(gen) <= 70; // 70%の確率で逃走成功
    return escaped;
}

void Player::setNightTime(bool night) {
    isNightTime = night;
}

void Player::toggleNightTime() {
    isNightTime = !isNightTime;
}

// 新しいパラメータ変更メソッド

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
    
    auto& extStats = playerStats->getExtendedStats();
    file.write(reinterpret_cast<const char*>(&extStats.gold), sizeof(extStats.gold));
    file.write(reinterpret_cast<const char*>(&extStats.mental), sizeof(extStats.mental));
    file.write(reinterpret_cast<const char*>(&extStats.demonTrust), sizeof(extStats.demonTrust));
    file.write(reinterpret_cast<const char*>(&extStats.kingTrust), sizeof(extStats.kingTrust));
    auto trustData = playerTrust->getTrustData();
    file.write(reinterpret_cast<const char*>(&trustData.trustLevel), sizeof(trustData.trustLevel));
    file.write(reinterpret_cast<const char*>(&trustData.evilActions), sizeof(trustData.evilActions));
    file.write(reinterpret_cast<const char*>(&trustData.goodActions), sizeof(trustData.goodActions));
    
    file.write(reinterpret_cast<const char*>(&currentNight), sizeof(currentNight));
    int killedResidentsSize = killedResidents.size();
    file.write(reinterpret_cast<const char*>(&killedResidentsSize), sizeof(killedResidentsSize));
    for (const auto& pos : killedResidents) {
        file.write(reinterpret_cast<const char*>(&pos.first), sizeof(pos.first));
        file.write(reinterpret_cast<const char*>(&pos.second), sizeof(pos.second));
    }
    
    int nameLength = name.length();
    file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
    file.write(name.c_str(), nameLength);
    
    inventory.saveToFile(file);
    equipmentManager.saveToFile(file);

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
    
    auto& extStats = playerStats->getExtendedStats();
    file.read(reinterpret_cast<char*>(&extStats.gold), sizeof(extStats.gold));
    file.read(reinterpret_cast<char*>(&extStats.mental), sizeof(extStats.mental));
    file.read(reinterpret_cast<char*>(&extStats.demonTrust), sizeof(extStats.demonTrust));
    file.read(reinterpret_cast<char*>(&extStats.kingTrust), sizeof(extStats.kingTrust));
    PlayerTrust::TrustData trustData;
    file.read(reinterpret_cast<char*>(&trustData.trustLevel), sizeof(trustData.trustLevel));
    file.read(reinterpret_cast<char*>(&trustData.evilActions), sizeof(trustData.evilActions));
    file.read(reinterpret_cast<char*>(&trustData.goodActions), sizeof(trustData.goodActions));
    playerTrust->setTrustData(trustData);
    
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
    
    int nameLength;
    file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
    char* nameBuffer = new char[nameLength + 1];
    file.read(nameBuffer, nameLength);
    nameBuffer[nameLength] = '\0';
    name = std::string(nameBuffer);
    delete[] nameBuffer;
    
    inventory.loadFromFile(file);
    equipmentManager.loadFromFile(file);

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