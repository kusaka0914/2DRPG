# 🐉 ドラクエ風RPG - Web版

C++とSDL2で作られたドラクエスタイルのRPGゲームを、Emscriptenを使用してWebブラウザで動作するようにしたバージョンです。

## 🌐 特徴

- **Webブラウザで動作**: Emscriptenを使用してC++コードをWebAssemblyにコンパイル
- **完全なRPGシステム**: レベルアップ、魔法、戦闘、宿屋など
- **美しいUI**: モダンなWebデザインとレスポンシブレイアウト
- **日本語完全対応**: ヒラギノフォントによる美しい日本語表示
- **ポートフォリオ対応**: ポートフォリオサイトに簡単に組み込み可能

## 🛠️ セットアップ

### 1. Emscriptenのインストール

#### 方法1: Homebrewを使用（推奨）
```bash
brew install emscripten
```

#### 方法2: 公式インストーラーを使用
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### 2. プロジェクトのビルド

```bash
# ビルドスクリプトを実行
./build_web.sh
```

### 3. ローカルサーバーでのテスト

```bash
cd web
python3 -m http.server 8000
# または
npx serve .
```

ブラウザで `http://localhost:8000` にアクセスしてください。

## 📁 ファイル構成

```
RPG/
├── src/                    # ソースコード
├── assets/                 # 画像・音声ファイル
├── web/                    # Web用ファイル
│   ├── index.html         # メインHTMLファイル
│   ├── DragonQuestRPG_Web.js    # コンパイルされたJS
│   ├── DragonQuestRPG_Web.wasm  # WebAssemblyファイル
│   └── assets/            # コピーされたアセット
├── CMakeListsWeb.txt      # Web用CMake設定
├── build_web.sh           # Webビルドスクリプト
└── README_Web.md          # このファイル
```

## 🎮 操作方法

### フィールド
- **方向キー**: プレイヤー移動
- **ESC**: メインメニューに戻る

### 戦闘
- **1**: 攻撃
- **2**: 呪文
- **3**: 防御
- **4**: 逃げる

### メニュー
- **マウス**: ボタンクリック
- **スペース**: 決定

## 🚀 ポートフォリオサイトへの組み込み

### 1. 基本的な組み込み

```html
<!DOCTYPE html>
<html>
<head>
    <title>私のポートフォリオ</title>
</head>
<body>
    <h1>私の作品</h1>
    
    <!-- ゲームの組み込み -->
    <div id="game-container">
        <canvas id="game-canvas" width="1100" height="650"></canvas>
    </div>
    
    <script>
        var Module = {
            onRuntimeInitialized: function() {
                console.log('ゲームが読み込まれました');
            }
        };
        
        var script = document.createElement('script');
        script.src = 'path/to/DragonQuestRPG_Web.js';
        document.head.appendChild(script);
    </script>
</body>
</html>
```

### 2. レスポンシブ対応

```css
.game-container {
    max-width: 100%;
    overflow: hidden;
}

#game-canvas {
    max-width: 100%;
    height: auto;
}
```

### 3. ローディング表示

```javascript
// ローディング状態の管理
let gameLoaded = false;

var Module = {
    onRuntimeInitialized: function() {
        document.getElementById('loading').style.display = 'none';
        gameLoaded = true;
    }
};
```

## 🔧 カスタマイズ

### ゲームサイズの変更

`web/index.html` の canvas 要素の width と height を変更：

```html
<canvas id="game-canvas" width="800" height="600"></canvas>
```

### スタイルの変更

CSSを編集してデザインをカスタマイズ：

```css
.game-container {
    background: linear-gradient(135deg, #667eea, #764ba2);
    border-radius: 15px;
    padding: 30px;
}
```

## 🐛 トラブルシューティング

### よくある問題

1. **Emscriptenが見つからない**
   ```bash
   brew install emscripten
   ```

2. **ビルドエラー**
   ```bash
   # ビルドディレクトリをクリーン
   rm -rf build_web
   ./build_web.sh
   ```

3. **WebAssemblyが読み込まれない**
   - ローカルサーバーを使用していることを確認
   - ブラウザのコンソールでエラーを確認

4. **アセットが読み込まれない**
   - `web/assets/` ディレクトリが存在することを確認
   - ファイルパスが正しいことを確認

## 📝 ライセンス

このプロジェクトはMITライセンスの下で公開されています。

## 🤝 貢献

バグ報告や機能要望は、GitHubのIssuesでお知らせください。

---

**注意**: このWeb版は元のC++プロジェクトをEmscriptenでコンパイルしたものです。パフォーマンスや機能に制限がある場合があります。 