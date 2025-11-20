cmake_minimum_required(VERSION 3.16)
project(DragonQuestRPG VERSION 1.0.0 LANGUAGES CXX)

# C++標準を設定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# コンパイラフラグ
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# ビルドタイプの設定
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# SDL2の検索
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

# JSON library (nlohmann/json)
find_package(nlohmann_json REQUIRED)

# ソースファイル
set(SOURCES
    src/app/main_sdl.cpp
    src/core/GameState.cpp
    src/core/SDL2Game.cpp
    src/gfx/Graphics.cpp
    src/io/InputManager.cpp
    src/game/MainMenuState.cpp
    src/game/FieldState.cpp
    src/game/BattleState.cpp
    src/game/BattleLogic.cpp
    src/game/BattleAnimationController.cpp
    src/game/BattleEffectManager.cpp
    src/game/BattleUI.cpp
    src/game/BattlePhaseManager.cpp
    src/entities/PlayerStats.cpp
    src/entities/PlayerStory.cpp
    src/entities/PlayerTrust.cpp
    src/game/TownState.cpp
    src/game/CastleState.cpp
    src/game/RoomState.cpp
    src/game/NightState.cpp
    src/game/DemonCastleState.cpp
    src/game/GameOverState.cpp
    src/game/EndingState.cpp
    src/entities/Character.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/core/Battle.cpp
    src/items/Item.cpp
    src/items/Inventory.cpp
    src/items/Equipment.cpp
    src/items/ItemFactory.cpp
    src/ui/UI.cpp
    src/ui/CommonUI.cpp
    src/utils/MapTerrain.cpp
    src/utils/TownLayout.cpp
    src/core/utils/ui_config_manager.cpp
    src/core/AudioManager.cpp
)

# ヘッダーファイル
set(HEADERS
    src/core/GameState.h
    src/core/SDL2Game.h
    src/core/GameUtils.h
    src/core/Battle.h
    src/gfx/Graphics.h
    src/io/InputManager.h
    src/game/MainMenuState.h
    src/game/FieldState.h
    src/game/BattleState.h
    src/game/BattleLogic.h
    src/game/BattleAnimationController.h
    src/game/BattleEffectManager.h
    src/game/BattleUI.h
    src/game/BattleConstants.h
    src/entities/PlayerStats.h
    src/entities/PlayerStory.h
    src/entities/PlayerTrust.h
    src/game/TownState.h
    src/game/CastleState.h
    src/game/RoomState.h
    src/game/NightState.h
    src/game/DemonCastleState.h
    src/game/GameOverState.h
    src/game/EndingState.h
    src/entities/Character.h
    src/entities/Player.h
    src/entities/Enemy.h
    src/items/Item.h
    src/items/Inventory.h
    src/items/Equipment.h
    src/ui/UI.h
    src/ui/CommonUI.h
    src/utils/MapTerrain.h
    src/utils/TownLayout.h
    src/core/utils/ui_config_manager.h
    src/core/AudioManager.h
)

# 実行ファイルを作成
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# インクルードディレクトリ
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

# リンクディレクトリ
target_link_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARY_DIRS}
    ${SDL2_IMAGE_LIBRARY_DIRS}
    ${SDL2_TTF_LIBRARY_DIRS}
    ${SDL2_MIXER_LIBRARY_DIRS}
)

# リンクライブラリ
target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# コンパイラフラグ
target_compile_options(${PROJECT_NAME} PRIVATE
    ${SDL2_CFLAGS_OTHER}
    ${SDL2_IMAGE_CFLAGS_OTHER}
    ${SDL2_TTF_CFLAGS_OTHER}
    ${SDL2_MIXER_CFLAGS_OTHER}
)

# デバッグビルドの設定
set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_FLAGS_DEBUG "-g -DDEBUG -O0"
    COMPILE_FLAGS_RELEASE "-O2 -DNDEBUG"
)

# インストール設定（オプション）
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# assetsディレクトリをビルドディレクトリにコピー（開発用）
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}
    FILES_MATCHING PATTERN "*.png" PATTERN "*.ttf" PATTERN "*.md" PATTERN "*.json" PATTERN "*.ogg"
)

