/**
 * @file UI.h
 * @brief UI要素の基底クラスと実装クラス
 * @details UIElement、Button、Label、StoryMessageBox、UIManagerを定義する。
 */

#pragma once
#include "../gfx/Graphics.h"
#include "../io/InputManager.h"
#include <string>
#include <vector>
#include <functional>

/**
 * @brief UI要素の基底クラス
 * @details 全てのUI要素の基底となる抽象クラス。
 * 位置、サイズ、表示状態、有効/無効状態を管理する。
 */
class UIElement {
protected:
    int x, y, width, height;
    bool visible;
    bool enabled;

public:
    /**
     * @brief コンストラクタ
     * @param x X座標
     * @param y Y座標
     * @param width 幅
     * @param height 高さ
     */
    UIElement(int x, int y, int width, int height);
    
    /**
     * @brief デストラクタ
     */
    virtual ~UIElement() = default;
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    virtual void update(float /*deltaTime*/) {}
    
    /**
     * @brief 描画処理（純粋仮想関数）
     * @param graphics グラフィックスオブジェクトへの参照
     */
    virtual void render(Graphics& graphics) = 0;
    
    /**
     * @brief 入力処理
     * @param input 入力マネージャーへの参照
     * @return 入力が処理されたか
     */
    virtual bool handleInput(const InputManager& /*input*/) { return false; }
    
    /**
     * @brief 位置の設定
     * @param newX 新しいX座標
     * @param newY 新しいY座標
     */
    void setPosition(int newX, int newY) { x = newX; y = newY; }
    
    /**
     * @brief サイズの設定
     * @param newWidth 新しい幅
     * @param newHeight 新しい高さ
     */
    void setSize(int newWidth, int newHeight) { width = newWidth; height = newHeight; }
    
    /**
     * @brief 表示状態の設定
     * @param vis 表示するか
     */
    void setVisible(bool vis) { visible = vis; }
    
    /**
     * @brief 有効状態の設定
     * @param en 有効にするか
     */
    void setEnabled(bool en) { enabled = en; }
    
    /**
     * @brief ポイントが要素内にあるか
     * @param px ポイントのX座標
     * @param py ポイントのY座標
     * @return ポイントが要素内にあるか
     */
    bool isPointInside(int px, int py) const;
    
    /**
     * @brief 表示されているか
     * @return 表示されているか
     */
    bool isVisible() const { return visible; }
    
    /**
     * @brief 有効か
     * @return 有効か
     */
    bool isEnabled() const { return enabled; }
};

/**
 * @brief ボタンクラス
 * @details クリック可能なボタンUI要素を実装する。
 */
class Button : public UIElement {
private:
    std::string text;
    std::string fontName;
    SDL_Color textColor;
    SDL_Color normalColor;
    SDL_Color hoverColor;
    SDL_Color pressedColor;
    std::function<void()> onClick;
    
    bool isHovered;
    bool isPressed;

public:
    /**
     * @brief コンストラクタ
     * @param x X座標
     * @param y Y座標
     * @param width 幅
     * @param height 高さ
     * @param text ボタンテキスト
     * @param fontName フォント名（デフォルト: "default"）
     */
    Button(int x, int y, int width, int height, const std::string& text, const std::string& fontName = "default");
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics) override;
    
    /**
     * @brief 入力処理
     * @param input 入力マネージャーへの参照
     * @return 入力が処理されたか
     */
    bool handleInput(const InputManager& input) override;
    
    /**
     * @brief クリック時のコールバック設定
     * @param callback コールバック関数
     */
    void setOnClick(std::function<void()> callback) { onClick = callback; }
    
    /**
     * @brief テキストの設定
     * @param newText 新しいテキスト
     */
    void setText(const std::string& newText) { text = newText; }
    
    /**
     * @brief 色の設定
     * @param normal 通常時の色
     * @param hover ホバー時の色
     * @param pressed 押下時の色
     */
    void setColors(SDL_Color normal, SDL_Color hover, SDL_Color pressed);
};

// テキストラベルクラス
class Label : public UIElement {
private:
    std::string text;
    std::string fontName;
    SDL_Color textColor;
    std::vector<std::string> lines; // 改行で分割された行

public:
    /**
     * @brief コンストラクタ
     * @param x X座標
     * @param y Y座標
     * @param text ラベルテキスト
     * @param fontName フォント名（デフォルト: "default"）
     */
    Label(int x, int y, const std::string& text, const std::string& fontName = "default");
    
    /**
     * @brief デストラクタ
     */
    ~Label();
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics) override;
    
    /**
     * @brief テキストの設定
     * @param newText 新しいテキスト
     */
    void setText(const std::string& newText);
    
    /**
     * @brief 色の設定
     * @param color テキスト色
     */
    void setColor(SDL_Color color) { textColor = color; }
    
    /**
     * @brief テキストの取得
     * @return テキストへの参照
     */
    const std::string& getText() const { return text; }
    
private:
    /**
     * @brief テキストを行に分割
     */
    void splitTextIntoLines();
};

// ストーリーメッセージボックスクラス（黒背景・白文字）
class StoryMessageBox : public UIElement {
private:
    std::vector<std::string> lines;
    std::string fontName;
    SDL_Color backgroundColor;
    SDL_Color textColor;
    SDL_Color borderColor;
    std::vector<SDL_Texture*> lineTextures;
    Graphics* graphics;
    float displayTimer;
    float displayDuration;
    int padding;
    int lineSpacing;
    
public:
    /**
     * @brief コンストラクタ
     * @param x X座標
     * @param y Y座標
     * @param width 幅
     * @param height 高さ
     * @param fontName フォント名（デフォルト: "default"）
     */
    StoryMessageBox(int x, int y, int width, int height, const std::string& fontName = "default");
    
    /**
     * @brief デストラクタ
     */
    ~StoryMessageBox();
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics) override;
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime) override;
    
    /**
     * @brief メッセージの設定（複数行）
     * @param messageLines メッセージ行のベクター
     */
    void setMessage(const std::vector<std::string>& messageLines);
    
    /**
     * @brief メッセージの設定（単一行）
     * @param singleLineMessage 単一行メッセージ
     */
    void setMessage(const std::string& singleLineMessage);
    
    /**
     * @brief 表示時間の設定
     * @param duration 表示時間（秒）
     */
    void setDisplayDuration(float duration) { displayDuration = duration; }
    
    /**
     * @brief 表示
     */
    void show();
    
    /**
     * @brief 非表示
     */
    void hide();
    
    /**
     * @brief 表示期限切れか
     * @return 表示期限切れか
     */
    bool isExpired() const { return displayTimer <= 0 && visible; }
    
private:
    /**
     * @brief テクスチャの更新
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void updateTextures(Graphics& graphics);
    
    /**
     * @brief テクスチャのクリア
     */
    void clearTextures();
};

// UI管理クラス
class UIManager {
private:
    std::vector<std::unique_ptr<UIElement>> elements;

public:
    /**
     * @brief 要素の追加
     * @param element 追加する要素へのユニークポインタ
     */
    void addElement(std::unique_ptr<UIElement> element);
    
    /**
     * @brief 全要素のクリア
     */
    void clear();
    
    /**
     * @brief 更新処理
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime);
    
    /**
     * @brief 描画処理
     * @param graphics グラフィックスオブジェクトへの参照
     */
    void render(Graphics& graphics);
    
    /**
     * @brief 入力処理
     * @param input 入力マネージャーへの参照
     */
    void handleInput(const InputManager& input);
    
    /**
     * @brief 要素の取得
     * @return 要素のベクターへの参照
     */
    const std::vector<std::unique_ptr<UIElement>>& getElements() const { return elements; }
    
    /**
     * @brief 要素の検索（テンプレート）
     * @tparam T 要素の型
     * @param index インデックス
     * @return 要素へのポインタ（見つからない場合はnullptr）
     */
    template<typename T>
    T* findElement(size_t index) {
        if (index < elements.size()) {
            return dynamic_cast<T*>(elements[index].get());
        }
        return nullptr;
    }
}; 