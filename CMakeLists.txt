cmake_minimum_required(VERSION 3.16)
project(DragonQuestRPG_Web VERSION 1.0.0 LANGUAGES CXX)

# Emscripten用の設定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emscripten固有の設定
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_FUNCTIONS=['_main']")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_WEBGL2=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FULL_ES3=1")
    # PNGサポートを有効にする
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s SDL2_IMAGE_FORMATS=['png']")
    # デバッグ情報を追加
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ASSERTIONS=1")
    # WebGL2の設定を改善
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s MIN_WEBGL_VERSION=2")
endif()

# ソースファイル
set(SOURCES
    src/main_sdl.cpp
    src/Character.cpp
    src/Player.cpp
    src/Enemy.cpp
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
    src/Battle.cpp
    src/CastleState.cpp
    src/RoomState.cpp
    src/NightState.cpp
    src/DemonCastleState.cpp
    src/GameOverState.cpp
    src/CommonUI.cpp
    src/TownLayout.cpp
)

# ヘッダーファイル
set(HEADERS
    src/Character.h
    src/Player.h
    src/Enemy.h
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
    src/Battle.h
    src/CastleState.h
    src/RoomState.h
    src/NightState.h
    src/DemonCastleState.h
    src/GameOverState.h
    src/CommonUI.h
    src/TownLayout.h
    src/GameUtils.h
)

# 実行ファイル作成
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Emscripten用の設定
if(EMSCRIPTEN)
    # assetsディレクトリをコピー
    file(COPY assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets)
    
    # HTMLテンプレートを設定
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s SDL2_IMAGE_FORMATS=['png'] -s USE_WEBGL2=1 -s FULL_ES3=1"
    )
endif()