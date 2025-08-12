#!/bin/bash

# Web用ビルドスクリプト
echo "🐉 ドラクエ風RPG - Webビルドスクリプト"
echo "=========================================="

# Emscriptenの確認
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscriptenが見つかりません。"
    echo "以下のコマンドでEmscriptenをインストールしてください："
    echo ""
    echo "1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "2. cd emsdk"
    echo "3. ./emsdk install latest"
    echo "4. ./emsdk activate latest"
    echo "5. source ./emsdk_env.sh"
    echo ""
    echo "または、Homebrewを使用している場合："
    echo "brew install emscripten"
    echo ""
    exit 1
fi

echo "✅ Emscriptenが見つかりました: $(emcc --version | head -n1)"

# ビルドディレクトリの作成
echo "📁 ビルドディレクトリを作成中..."
mkdir -p build_web
cd build_web

# CMakeの設定
echo "⚙️  CMakeの設定中..."
emcmake cmake -DCMAKE_BUILD_TYPE=Release ../

if [ $? -ne 0 ]; then
    echo "❌ CMakeの設定に失敗しました。"
    exit 1
fi

# ビルドの実行
echo "🔨 ビルド中..."
emmake make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ ビルドに失敗しました。"
    exit 1
fi

# ファイルのコピー
echo "📋 ファイルをコピー中..."
cp DragonQuestRPG_Web.html ../web/
cp DragonQuestRPG_Web.js ../web/
cp DragonQuestRPG_Web.wasm ../web/

# assetsディレクトリのコピー
if [ -d "assets" ]; then
    cp -r assets ../web/
fi

echo ""
echo "✅ ビルドが完了しました！"
echo ""
echo "📁 生成されたファイル:"
echo "   - web/DragonQuestRPG_Web.html"
echo "   - web/DragonQuestRPG_Web.js"
echo "   - web/DragonQuestRPG_Web.wasm"
echo "   - web/assets/ (画像ファイル)"
echo ""
echo "🌐 ローカルサーバーでテストする場合："
echo "   cd web"
echo "   python3 -m http.server 8000"
echo "   または"
echo "   npx serve ."
echo ""
echo "📱 ブラウザで http://localhost:8000 にアクセスしてください。" 