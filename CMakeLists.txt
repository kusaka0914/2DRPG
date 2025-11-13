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

# ソースファイル
set(SOURCES
    src/main_sdl.cpp
    src/Character.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/Battle.cpp
    src/Graphics.cpp
    src/InputManager.cpp
    src/GameState.cpp
    src/UI.cpp
    src/MainMenuState.cpp
    src/FieldState.cpp
    src/BattleState.cpp
    src/TownState.cpp
    src/CastleState.cpp
    src/RoomState.cpp
    src/NightState.cpp
    src/DemonCastleState.cpp
    src/GameOverState.cpp
    src/EndingState.cpp
    src/SDL2Game.cpp
    src/Item.cpp
    src/Inventory.cpp
    src/Equipment.cpp
    src/MapTerrain.cpp
    src/CommonUI.cpp
    src/TownLayout.cpp
)

# ヘッダーファイル
set(HEADERS
    src/Character.h
    src/Player.h
    src/Enemy.h
    src/Battle.h
    src/Graphics.h
    src/InputManager.h
    src/GameState.h
    src/UI.h
    src/MainMenuState.h
    src/FieldState.h
    src/BattleState.h
    src/TownState.h
    src/CastleState.h
    src/RoomState.h
    src/NightState.h
    src/DemonCastleState.h
    src/GameOverState.h
    src/EndingState.h
    src/SDL2Game.h
    src/Item.h
    src/Inventory.h
    src/Equipment.h
    src/MapTerrain.h
    src/CommonUI.h
    src/TownLayout.h
    src/GameUtils.h
)

# 実行ファイルを作成
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# インクルードディレクトリ
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

# リンクディレクトリ
target_link_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARY_DIRS}
    ${SDL2_IMAGE_LIBRARY_DIRS}
    ${SDL2_TTF_LIBRARY_DIRS}
)

# リンクライブラリ
target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)

# コンパイラフラグ
target_compile_options(${PROJECT_NAME} PRIVATE
    ${SDL2_CFLAGS_OTHER}
    ${SDL2_IMAGE_CFLAGS_OTHER}
    ${SDL2_TTF_CFLAGS_OTHER}
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
    FILES_MATCHING PATTERN "*.png" PATTERN "*.ttf" PATTERN "*.md"
)

