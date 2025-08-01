#pragma once
#include "Graphics.h"
#include "InputManager.h"
#include <string>
#include <vector>
#include <functional>

// UIの基底クラス
class UIElement {
protected:
    int x, y, width, height;
    bool visible;
    bool enabled;

public:
    UIElement(int x, int y, int width, int height);
    virtual ~UIElement() = default;
    
    virtual void update(float /*deltaTime*/) {}
    virtual void render(Graphics& graphics) = 0;
    virtual bool handleInput(const InputManager& /*input*/) { return false; }
    
    void setPosition(int newX, int newY) { x = newX; y = newY; }
    void setSize(int newWidth, int newHeight) { width = newWidth; height = newHeight; }
    void setVisible(bool vis) { visible = vis; }
    void setEnabled(bool en) { enabled = en; }
    
    bool isPointInside(int px, int py) const;
    bool isVisible() const { return visible; }
    bool isEnabled() const { return enabled; }
};

// ボタンクラス
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
    Button(int x, int y, int width, int height, const std::string& text, const std::string& fontName = "default");
    
    void render(Graphics& graphics) override;
    bool handleInput(const InputManager& input) override;
    
    void setOnClick(std::function<void()> callback) { onClick = callback; }
    void setText(const std::string& newText) { text = newText; }
    void setColors(SDL_Color normal, SDL_Color hover, SDL_Color pressed);
};

// テキストラベルクラス
class Label : public UIElement {
private:
    std::string text;
    std::string fontName;
    SDL_Color textColor;
    SDL_Texture* textTexture;
    Graphics* graphics;

public:
    Label(int x, int y, const std::string& text, const std::string& fontName = "default");
    ~Label();
    
    void render(Graphics& graphics) override;
    void setText(const std::string& newText);
    void setColor(SDL_Color color) { textColor = color; }
    
private:
    void updateTexture(Graphics& graphics);
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
    StoryMessageBox(int x, int y, int width, int height, const std::string& fontName = "default");
    ~StoryMessageBox();
    
    void render(Graphics& graphics) override;
    void update(float deltaTime) override;
    void setMessage(const std::vector<std::string>& messageLines);
    void setMessage(const std::string& singleLineMessage);
    void setDisplayDuration(float duration) { displayDuration = duration; }
    void show();
    void hide();
    bool isExpired() const { return displayTimer <= 0 && visible; }
    
private:
    void updateTextures(Graphics& graphics);
    void clearTextures();
};

// UI管理クラス
class UIManager {
private:
    std::vector<std::unique_ptr<UIElement>> elements;

public:
    void addElement(std::unique_ptr<UIElement> element);
    void clear();
    
    void update(float deltaTime);
    void render(Graphics& graphics);
    void handleInput(const InputManager& input);
    const std::vector<std::unique_ptr<UIElement>>& getElements() const { return elements; }
    
    template<typename T>
    T* findElement(size_t index) {
        if (index < elements.size()) {
            return dynamic_cast<T*>(elements[index].get());
        }
        return nullptr;
    }
}; 