#pragma once
#include "../core/GameState.h"
#include "../ui/UI.h"
#include "../entities/Player.h"
#include "../items/Equipment.h"
#include "../core/GameUtils.h"
#include "../utils/TownLayout.h"
#include "NightState.h"
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
    const int TILE_SIZE = 38;
    const int MAP_WIDTH = 28;  // 20 → 28に拡大（画面幅1100px ÷ 38px = 約29タイル、UI部分を考慮して28）
    const int MAP_HEIGHT = 16; // 15 → 16に拡大（画面高さ650px ÷ 38px = 約17タイル、UI部分を考慮して16）
    const int BUILDING_SIZE = 2; // 建物は2タイルサイズ
    
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
    
    // 建物の位置（2タイルサイズ）
    std::vector<std::pair<int, int>> buildings;
    std::vector<std::string> buildingTypes;
    
    // 城の位置（画面中央）
    int castleX, castleY;
    bool hasVisitedCastle;
    
    // ゲームパッド接続状態
    bool gameControllerConnected;
    
    // 夜の街へのタイマー機能
    bool nightTimerActive;
    float nightTimer;
    const float NIGHT_TIMER_DURATION = 360.0f; // 6分 = 360秒
    
    // ショップ関連
    std::vector<std::unique_ptr<Item>> shopItems;
    
    // 画像テクスチャ
    SDL_Texture* playerTexture;
    SDL_Texture* shopTexture;
    SDL_Texture* weaponShopTexture;
    SDL_Texture* houseTexture;
    SDL_Texture* castleTexture;
    SDL_Texture* stoneTileTexture;
    
    // 住人画像テクスチャ
    SDL_Texture* residentTextures[6]; // resident_1 から resident_5
    SDL_Texture* guardTexture; // 衛兵画像
    SDL_Texture* toriiTexture; // 鳥居画像
    SDL_Texture* residentHomeTexture; // 住人の家画像

public:
    TownState(std::shared_ptr<Player> player);
    
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;
    void render(Graphics& graphics) override;
    void handleInput(const InputManager& input) override;
    
    StateType getType() const override { return StateType::TOWN; }
    
    // 夜のタイマー関連
    void startNightTimer();
    void setupGameExplanation();

    static bool saved;
    
    // 夜のタイマーの静的状態管理（他の状態からアクセス可能）
    static bool s_nightTimerActive;
    static float s_nightTimer;
    
    // 目標レベル機能
    static int s_targetLevel;
    static bool s_levelGoalAchieved;
    const int DEFAULT_TARGET_LEVEL = 10; // デフォルト目標レベル
    
    // DemonCastleStateから来たことを示すフラグ
    static bool s_fromDemonCastle;
    
    // 夜の回数を追跡
    static int s_nightCount;
    
    // ゲーム説明機能
    bool showGameExplanation;
    int explanationStep;
    std::vector<std::string> gameExplanationTexts;
    
    // 初回メッセージ表示用フラグ
    bool pendingWelcomeMessage;
    std::string pendingMessage;
    
private:
    void setupUI(Graphics& graphics);
    void setupNPCs();
    void setupShopItems();
    void loadTextures(Graphics& graphics);
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
    void drawGate(Graphics& graphics);
    void drawBuildingInterior(Graphics& graphics);
    
    // メッセージボード関連
    void showMessage(const std::string& message);
    void clearMessage();
    
    // 自室への入り口関連
    void checkRoomEntrance();
    void enterRoom();
    
    // フィールドへの入り口関連
    void checkFieldEntrance();
    void enterField();
    
    // 建物への入場関連
    void checkBuildingEntrance();
    void checkCastleEntrance();
    void enterShop();
    void enterInn();
    void enterChurch();
    void exitBuilding();
    
    bool isValidPosition(int x, int y) const;
    bool isNearNPC(int x, int y) const;
    bool isCollidingWithBuilding(int x, int y) const;
    bool isCollidingWithNPC(int x, int y) const;
    NPC* getNearbyNPC(int x, int y);
    void checkTrustLevels();
}; 