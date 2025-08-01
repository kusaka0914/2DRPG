cmake_minimum_required(VERSION 3.16)
project(DragonQuestRPG VERSION 1.0.0 LANGUAGES CXX)

# C++17を要求
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# デバッグ情報とビルドタイプ
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# コンパイラオプション
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# SDL2の検索と設定 (macOS対応)
find_package(PkgConfig REQUIRED)

# SDL2のライブラリを検索
execute_process(
    COMMAND pkg-config --libs sdl2
    OUTPUT_VARIABLE SDL2_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND pkg-config --cflags sdl2
    OUTPUT_VARIABLE SDL2_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# SDL2_imageのライブラリを検索
execute_process(
    COMMAND pkg-config --libs SDL2_image
    OUTPUT_VARIABLE SDL2_IMAGE_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# SDL2_ttfのライブラリを検索
execute_process(
    COMMAND pkg-config --libs SDL2_ttf
    OUTPUT_VARIABLE SDL2_TTF_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# ソースファイル（src/ディレクトリから）
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
    src/SDL2Game.cpp
    src/Item.cpp
    src/Inventory.cpp
    src/Equipment.cpp
    src/MapTerrain.cpp
)

# ヘッダーファイル（src/ディレクトリから）
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
    src/SDL2Game.h
    src/Item.h
    src/Inventory.h
    src/Equipment.h
    src/MapTerrain.h
)

# 実行ファイル作成
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# コンパイラフラグとライブラリを設定
separate_arguments(SDL2_CFLAGS_LIST UNIX_COMMAND "${SDL2_CFLAGS}")
separate_arguments(SDL2_LIBS_LIST UNIX_COMMAND "${SDL2_LIBRARIES}")
separate_arguments(SDL2_IMAGE_LIBS_LIST UNIX_COMMAND "${SDL2_IMAGE_LIBRARIES}")
separate_arguments(SDL2_TTF_LIBS_LIST UNIX_COMMAND "${SDL2_TTF_LIBRARIES}")

target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_LIST})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBS_LIST} ${SDL2_IMAGE_LIBS_LIST} ${SDL2_TTF_LIBS_LIST})

# インストール
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# assetsディレクトリも一緒にインストール
install(DIRECTORY assets/ DESTINATION share/${PROJECT_NAME}/assets)

# ヘルプメッセージ
message(STATUS "")
message(STATUS "ドラクエ風RPG ビルド設定:")
message(STATUS "  プロジェクト名: ${PROJECT_NAME}")
message(STATUS "  バージョン: ${PROJECT_VERSION}")
message(STATUS "  ビルドタイプ: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++標準: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "ビルド方法:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  make")
message(STATUS "") 