#pragma once
#include "GameState.h"
#include "UI.h"
#include "Player.h"
#include "Equipment.h"
#include <memory>
#include <vector>

enum class NPCType {
    SHOPKEEPER,
    INNKEEPER,
    TOWNSPERSON,
    GUARD
};

struct NPC {
    NPCType type;
    std::string name;
    std::string dialogue;
    int x, y;  // 位置
    
    NPC(NPCType type, const std::string& name, const std::string& dialogue, int x, int y)
        : type(type), name(name), dialogue(dialogue), x(x), y(y) {}
};

enum class TownLocation {
    SQUARE,      // 広場
    SHOP,        // お店
    INN,         // 宿屋
    CHURCH,      // 教会
    ROOM_ENTRANCE // 自室入り口
};

class TownState : public GameState {
private:
    std::shared_ptr<Player> player;
    UIManager ui;
    
    // メッセージボード用UI
    Label* messageBoard;
    bool isShowingMessage;
    
    // プレイヤーの位置
    int playerX, playerY;
    const int TILE_SIZE = 32;
    const int MAP_WIDTH = 25;
    const int MAP_HEIGHT = 18;
    
    // 移動タイマー
    float moveTimer;
    const float MOVE_DELAY = 0.2f;
    
    // NPCと建物
    std::vector<NPC> npcs;
    // ゲーム状態
    TownLocation currentLocation;
    bool isShopOpen, isInnOpen;
    
    // 自室の入り口位置
    int roomEntranceX, roomEntranceY;
    
    // 建物の位置
    std::vector<std::pair<int, int>> buildings;
    std::vector<std::string> buildingTypes;
    
    // 城の位置
    int castleX, castleY;
    bool hasVisitedCastle;
    
    // ゲームパッド接続状態
    bool gameControllerConnected;
    
    // ショップ関連
    std::vector<std::unique_ptr<Item>> shopItems;

public:
    TownState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::TOWN; }
    
private:
    void setupUI();
    void setupNPCs();
    void setupShopItems();
    void handleMovement(const InputManager& input);
    void checkNPCInteraction();
    void handleNPCDialogue(const NPC& npc);
    void openShop();
    void openInn();
    void showShopMenu();
    void buyItem(int itemIndex);
    void sellItem(int itemIndex);
    
    void drawMap(Graphics& graphics);
    void drawPlayer(Graphics& graphics);
    void drawNPCs(Graphics& graphics);
    void drawBuildings(Graphics& graphics);
    void drawBuildingInterior(Graphics& graphics);
    
    // メッセージボード関連
    void showMessage(const std::string& message);
    void clearMessage();
    
    // 自室への入り口関連
    void checkRoomEntrance();
    void enterRoom();
    
    // 建物への入場関連
    void checkBuildingEntrance();
    void checkCastleEntrance();
    void enterShop();
    void enterInn();
    void enterChurch();
    void exitBuilding();
    
    bool isValidPosition(int x, int y) const;
    bool isNearNPC(int x, int y) const;
    NPC* getNearbyNPC(int x, int y);
}; 