#!/bin/bash

# ドラクエ風RPG 起動スクリプト (macOS/Linux)

# スクリプトのディレクトリに移動
cd "$(dirname "$0")"

# ビルドディレクトリが存在しない場合は作成
if [ ! -d "build" ]; then
    echo "ビルドディレクトリを作成中・・・"
    mkdir -p build
fi

# CMakeの設定（初回のみ）
if [ ! -f "build/CMakeCache.txt" ]; then
    echo "⚙️  CMakeの設定中..."
    cd build
    cmake ..
    if [ $? -ne 0 ]; then
        echo "CMakeの設定に失敗しました。"
        exit 1
    fi
    cd ..
fi

# ビルド
echo "ゲームをビルド中・・・"
cd build
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo "ビルドに失敗しました。"
    exit 1
fi

# ゲームを起動
echo "ゲームを起動中・・・"
./DragonQuestRPG

cd ..

