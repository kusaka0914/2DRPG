#include "Player.h"
#include "../game/TownState.h"
#include "../core/GameState.h"
#include <iostream>
#include <random>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <cstdio>

Player::Player(const std::string& name)
    : Character(name, 30, 20, 8, 3, 1), inventory(20), equipmentManager(),
      playerStats(std::make_unique<PlayerStats>()),
      playerStory(std::make_unique<PlayerStory>(name)),
      playerTrust(std::make_unique<PlayerTrust>(50, true)),
      savedGameState(nullptr), savedGameOverExit(false),
      isNightTime(false), currentNight(0),
            hasSeenTownExplanation(false),
            hasSeenFieldExplanation(false),
            hasSeenFieldFirstVictoryExplanation(false),
            hasSeenBattleExplanation(false),
            hasSeenResidentBattleExplanation(false),
            hasSeenNightExplanation(false),
             hasSeenRoomStory(false),
             hasSeenCastleStory(false),
             hasSeenDemonCastleStory(false) {
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
    int defenseIncrease = 2;
    
    setMaxHp(getMaxHp() + hpIncrease);
    setMaxMp(getMaxMp() + mpIncrease);
    setAttack(getAttack() + attackIncrease);
    setDefense(getDefense() + defenseIncrease);
    
    hp = maxHp;
    mp = maxMp;
    
    // 新しいシステム：3種類の魔法は自動的に使用可能（学習不要）
    
    playerStory->setLevelUpStory(level);
}

void Player::displayInfo() const {
    displayStatus();
    
    int totalAttack = getTotalAttack();
    int totalDefense = getTotalDefense();
    int equipAttackBonus = equipmentManager.getTotalAttackBonus();
    int equipDefenseBonus = equipmentManager.getTotalDefenseBonus();
    
    // 新しいシステム：3種類の魔法は自動的に使用可能（表示不要）
}

void Player::gainExp(int expGained) {
    exp += expGained;
    
    // レベルアップに必要な経験値は段階的に増加: 10 + (現在のレベル - 1) × 5
    while (level < 100) {
        int requiredExp = 10 + (level - 1) * 5;
        if (exp < requiredExp) {
            break;
        }
        exp -= requiredExp;
        levelUp();
    }
    
    if (level >= 100) {
        
    }
}

bool Player::canCastSpell(SpellType spell) const {
    // 取得済みかのチェック不要：常に使用可能
    (void)spell; // 未使用警告を回避
    return true;
}

int Player::castSpell(SpellType spell, Character* target) {
    switch (spell) {
        case SpellType::HEAL:
            {
                // 回復魔法：自分のHPが4割回復
                int healAmount = static_cast<int>(getMaxHp() * 0.4);
                heal(healAmount);
                return healAmount;
            }
        case SpellType::STATUS_UP:
            {
                // ステータスアップ魔法：次の攻撃が2.5倍になる
                setNextTurnBonus(true, 2.5f, 1);
                return 1; // 成功を示す
            }
        case SpellType::ATTACK:
            {
                // 攻撃魔法：通常の攻撃と同じ攻撃力（BattleStateで処理）
                if (target) {
                    int baseAttack = getTotalAttack();
                    if (hasNextTurnBonusActive()) {
                        baseAttack = static_cast<int>(baseAttack * getNextTurnMultiplier());
                    }
                    int baseDamage = baseAttack;
                    int finalDamage = std::max(1, baseDamage - target->getEffectiveDefense());
                    target->takeDamage(finalDamage);
                    return finalDamage;
                }
            }
            break;
    }
    return 0;
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
    // assets/saves/ディレクトリに保存
    std::string savePath = "assets/saves/" + filename;
    nlohmann::json j;
    
    // 基本ステータス
    j["level"] = level;
    j["exp"] = exp;
    j["hp"] = hp;
    j["mp"] = mp;
    j["maxHp"] = maxHp;
    j["maxMp"] = maxMp;
    j["attack"] = getAttack();
    j["defense"] = getDefense();
    
    // 拡張ステータス
    auto& extStats = playerStats->getExtendedStats();
    j["gold"] = extStats.gold;
    j["mental"] = extStats.mental;
    j["demonTrust"] = extStats.demonTrust;
    j["kingTrust"] = extStats.kingTrust;
    
    // 信頼度データ
    auto trustData = playerTrust->getTrustData();
    j["trustLevel"] = trustData.trustLevel;
    j["evilActions"] = trustData.evilActions;
    j["goodActions"] = trustData.goodActions;
    
    // 夜の情報
    j["currentNight"] = currentNight;
    j["killedResidents"] = nlohmann::json::array();
    for (const auto& pos : killedResidents) {
        nlohmann::json resident;
        resident["x"] = pos.first;
        resident["y"] = pos.second;
        j["killedResidents"].push_back(resident);
    }
    
    // 名前
    j["name"] = name;
    
    // インベントリ
    j["inventory"] = inventory.toJson();
    
    // 装備
    j["equipment"] = equipmentManager.toJson();
    
    // タイマー情報
    j["nightTimer"] = nightTimer;
    j["nightTimerActive"] = nightTimerActive;
    
    // 説明UIの完了状態
    j["hasSeenTownExplanation"] = hasSeenTownExplanation;
    j["hasSeenFieldExplanation"] = hasSeenFieldExplanation;
    j["hasSeenFieldFirstVictoryExplanation"] = hasSeenFieldFirstVictoryExplanation;
    j["hasSeenBattleExplanation"] = hasSeenBattleExplanation;
    j["hasSeenResidentBattleExplanation"] = hasSeenResidentBattleExplanation;
    j["hasSeenNightExplanation"] = hasSeenNightExplanation;
    
    // ストーリーメッセージUIの完了状態
    j["hasSeenRoomStory"] = hasSeenRoomStory;
    j["hasSeenCastleStory"] = hasSeenCastleStory;
    j["hasSeenDemonCastleStory"] = hasSeenDemonCastleStory;
    
    // 目標レベルと夜の回数（静的変数の状態を保存）
    j["targetLevel"] = TownState::s_targetLevel;
    j["nightCount"] = TownState::s_nightCount;
    
    // ゲーム状態情報（外部から設定される）
    if (savedGameState) {
        j["gameState"] = *savedGameState;
    }
    
    // ゲームオーバーからの終了フラグ（外部から設定される）
    if (savedGameOverExit) {
        j["gameOverExit"] = true;
    }
    
    // JSONファイルに書き込み（既存ファイルを上書き）
    // まず既存ファイルを削除してから新しく作成（macOSの自動リネームを防ぐため）
    std::remove(savePath.c_str());
    
    std::ofstream file(savePath, std::ios::trunc);
    if (file.is_open()) {
        file << j.dump(4); // インデント4で整形して出力
        file.close();
    } else {
        std::cerr << "Error: Could not open file for writing: " << savePath << std::endl;
    }
}

bool Player::loadGame(const std::string& filename, float& nightTimer, bool& nightTimerActive) {
    // assets/saves/ディレクトリから読み込み（複数の候補を試す）
    std::vector<std::string> candidates = {
        "assets/saves/" + filename,           // assets/saves/autosave.json
        "../assets/saves/" + filename,        // ../assets/saves/autosave.json (buildディレクトリから実行)
        filename,                              // 直接指定されたパス（後方互換性のため）
        "autosave.dat"                         // 古いバイナリ形式（後方互換性のため）
    };
    
    std::ifstream file;
    bool fileFound = false;
    for (const auto& path : candidates) {
        file.open(path);
        if (file.is_open()) {
            fileFound = true;
            break;
        }
    }
    
    if (!fileFound) {
        return false;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        
        // 基本ステータス
        if (j.contains("level")) level = j["level"];
        if (j.contains("exp")) exp = j["exp"];
        if (j.contains("hp")) hp = j["hp"];
        if (j.contains("mp")) mp = j["mp"];
        if (j.contains("maxHp")) maxHp = j["maxHp"];
        if (j.contains("maxMp")) maxMp = j["maxMp"];
        if (j.contains("attack")) setAttack(j["attack"]);
        if (j.contains("defense")) setDefense(j["defense"]);
        
        // HPが0の場合はマックスにする（戦闘画面で終了した場合など）
        if (hp <= 0 && maxHp > 0) {
            hp = maxHp;
        }
        // MPが0の場合はマックスにする（戦闘画面で終了した場合など）
        if (mp <= 0 && maxMp > 0) {
            mp = maxMp;
        }
        
        // 拡張ステータス
        auto& extStats = playerStats->getExtendedStats();
        if (j.contains("gold")) extStats.gold = j["gold"];
        if (j.contains("mental")) extStats.mental = j["mental"];
        if (j.contains("demonTrust")) extStats.demonTrust = j["demonTrust"];
        if (j.contains("kingTrust")) extStats.kingTrust = j["kingTrust"];
        
        // 信頼度データ
        if (j.contains("trustLevel") && j.contains("evilActions") && j.contains("goodActions")) {
            PlayerTrust::TrustData trustData;
            trustData.trustLevel = j["trustLevel"];
            trustData.evilActions = j["evilActions"];
            trustData.goodActions = j["goodActions"];
            playerTrust->setTrustData(trustData);
        }
        
        // 夜の情報
        if (j.contains("currentNight")) currentNight = j["currentNight"];
        if (j.contains("killedResidents") && j["killedResidents"].is_array()) {
            killedResidents.clear();
            for (const auto& resident : j["killedResidents"]) {
                if (resident.contains("x") && resident.contains("y")) {
                    killedResidents.push_back({resident["x"], resident["y"]});
                }
            }
        }
        
        // 名前
        if (j.contains("name")) name = j["name"];
        
        // インベントリ
        if (j.contains("inventory")) {
            inventory.fromJson(j["inventory"]);
        }
        
        // 装備
        if (j.contains("equipment")) {
            equipmentManager.fromJson(j["equipment"]);
        }
        
        // タイマー情報
        if (j.contains("nightTimer")) nightTimer = j["nightTimer"];
        bool savedNightTimerActive = false;
        if (j.contains("nightTimerActive")) {
            savedNightTimerActive = j["nightTimerActive"];
            nightTimerActive = savedNightTimerActive;
        }
        
        // タイマーが0の場合は1に設定し、activeをtrueにする（戦闘画面で終了した場合など）
        // ただし、一度でもactiveがtrueになったことがある場合のみ（初めてactiveがtrueになった後にのみ）
        if (nightTimer <= 0.0f && savedNightTimerActive) {
            nightTimer = 1.0f;
            nightTimerActive = true;
        }
        
        // 説明UIの完了状態
        if (j.contains("hasSeenTownExplanation")) hasSeenTownExplanation = j["hasSeenTownExplanation"];
        if (j.contains("hasSeenFieldExplanation")) hasSeenFieldExplanation = j["hasSeenFieldExplanation"];
        if (j.contains("hasSeenFieldFirstVictoryExplanation")) hasSeenFieldFirstVictoryExplanation = j["hasSeenFieldFirstVictoryExplanation"];
        if (j.contains("hasSeenBattleExplanation")) hasSeenBattleExplanation = j["hasSeenBattleExplanation"];
        if (j.contains("hasSeenResidentBattleExplanation")) hasSeenResidentBattleExplanation = j["hasSeenResidentBattleExplanation"];
        if (j.contains("hasSeenNightExplanation")) hasSeenNightExplanation = j["hasSeenNightExplanation"];
        
        // ストーリーメッセージUIの完了状態
        if (j.contains("hasSeenRoomStory")) hasSeenRoomStory = j["hasSeenRoomStory"];
        if (j.contains("hasSeenCastleStory")) hasSeenCastleStory = j["hasSeenCastleStory"];
        if (j.contains("hasSeenDemonCastleStory")) hasSeenDemonCastleStory = j["hasSeenDemonCastleStory"];
        
        // 目標レベルと夜の回数（静的変数の状態を復元）
        if (j.contains("targetLevel")) TownState::s_targetLevel = j["targetLevel"];
        if (j.contains("nightCount")) TownState::s_nightCount = j["nightCount"];
        
        // ゲーム状態情報
        if (j.contains("gameState") && !j["gameState"].is_null()) {
            savedGameState = std::make_unique<nlohmann::json>(j["gameState"]);
        }
        
        // ゲームオーバーからの終了フラグ
        if (j.contains("gameOverExit") && j["gameOverExit"].get<bool>()) {
            // HP/MPをマックスにする
            hp = maxHp;
            mp = maxMp;
            // フィールドの状態を設定（gameStateが存在しない場合、またはGameOverStateの場合）
            if (!savedGameState || (savedGameState->contains("stateType") && 
                static_cast<int>((*savedGameState)["stateType"]) == static_cast<int>(StateType::GAME_OVER))) {
                // フィールドの状態を作成
                nlohmann::json fieldState;
                fieldState["stateType"] = static_cast<int>(StateType::FIELD);
                // フィールドのデフォルト位置を使用（FieldStateの静的変数から取得）
                fieldState["playerX"] = 25;
                fieldState["playerY"] = 8;
                fieldState["showGameExplanation"] = false;
                fieldState["explanationStep"] = 0;
                savedGameState = std::make_unique<nlohmann::json>(fieldState);
            }
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading game: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

void Player::autoSave() {
    // タイマー情報も含めてセーブ
    float nightTimer = TownState::s_nightTimer;
    bool nightTimerActive = TownState::s_nightTimerActive;
    saveGame("autosave.json", nightTimer, nightTimerActive);
}

bool Player::autoLoad(float& nightTimer, bool& nightTimerActive) {
    return loadGame("autosave.json", nightTimer, nightTimerActive);
}

void Player::setSavedGameState(const nlohmann::json& stateJson) {
    savedGameState = std::make_unique<nlohmann::json>(stateJson);
}

void Player::setGameOverExit(bool gameOverExit) {
    savedGameOverExit = gameOverExit;
}

const nlohmann::json* Player::getSavedGameState() const {
    return savedGameState.get();
}

std::string Player::getSpellName(SpellType spell) {
    switch (spell) {
        case SpellType::HEAL:
            return "回復魔法";
        case SpellType::STATUS_UP:
            return "ステータスアップ魔法";
        case SpellType::ATTACK:
            return "攻撃魔法";
        default:
            return "不明な魔法";
    }
}

std::vector<SpellType> Player::getSpellsLearnedAtLevel(int level) {
    // 新しいシステム：3種類の魔法は自動的に使用可能（レベルに関係なく）
    (void)level;
    std::vector<SpellType> spells;
    // 全ての魔法を返す（常に使用可能）
    spells.push_back(SpellType::HEAL);
    spells.push_back(SpellType::STATUS_UP);
    spells.push_back(SpellType::ATTACK);
    return spells;
} 