# Makefile for SDL2 Dragon Quest-style RPG game

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# SDL2 flags (macOS用)
SDL_FLAGS = `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf

# Target executable name
TARGET = rpg_game_2d

# Source files (from src/ directory)
SOURCES = src/main_sdl.cpp src/Character.cpp src/Player.cpp src/Enemy.cpp src/Battle.cpp \
          src/Graphics.cpp src/InputManager.cpp src/GameState.cpp src/UI.cpp \
          src/MainMenuState.cpp src/FieldState.cpp src/BattleState.cpp src/TownState.cpp src/CastleState.cpp src/SDL2Game.cpp \
          src/Item.cpp src/Inventory.cpp src/Equipment.cpp src/MapTerrain.cpp

# Object files (generated from source files) 
OBJECTS = $(SOURCES:.cpp=.o)

# Header files (for dependency tracking, from src/ directory)
HEADERS = src/Character.h src/Player.h src/Enemy.h src/Battle.h src/Graphics.h src/InputManager.h \
          src/GameState.h src/UI.h src/MainMenuState.h src/FieldState.h src/BattleState.h src/CastleState.h src/SDL2Game.h

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(SDL_FLAGS)
	@echo "2Dビルド完了！実行するには './$(TARGET)' を入力してください。"

# Build object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) `sdl2-config --cflags` -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "ビルドファイルを削除しました。"

# Rebuild everything
rebuild: clean all

# Install dependencies (macOS)
install-deps:
	@echo "SDL2依存関係のインストール..."
	@echo "Homebrewがインストールされている場合:"
	@echo "brew install sdl2 sdl2_image sdl2_ttf"
	@echo ""
	@echo "または手動でSDL2をインストールしてください:"
	@echo "https://www.libsdl.org/"

# Run the game
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# Help
help:
	@echo "利用可能なターゲット:"
	@echo "  all          - 2Dゲームをビルド (デフォルト)"
	@echo "  clean        - ビルドファイルを削除"
	@echo "  rebuild      - クリーンビルド"
	@echo "  run          - ゲームをビルドして実行"
	@echo "  debug        - デバッグ版をビルド"
	@echo "  release      - リリース版をビルド"
	@echo "  install-deps - 依存関係のインストール方法を表示"
	@echo "  help         - このヘルプメッセージを表示"

# Create assets directory
setup-assets:
	mkdir -p assets
	@echo "assetsディレクトリを作成しました。"
	@echo "フォントファイル(font.ttf)をassetsディレクトリに配置してください。"

# Phony targets
.PHONY: all clean rebuild install-deps run debug release help setup-assets 