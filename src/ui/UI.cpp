#include "UI.h"

// UIElement
UIElement::UIElement(int x, int y, int width, int height) 
    : x(x), y(y), width(width), height(height), visible(true), enabled(true) {
}

bool UIElement::isPointInside(int px, int py) const {
    return px >= x && px < x + width && py >= y && py < y + height;
}

// Button
Button::Button(int x, int y, int width, int height, const std::string& text, const std::string& fontName)
    : UIElement(x, y, width, height), text(text), fontName(fontName), 
      isHovered(false), isPressed(false) {
    
    textColor = {255, 255, 255, 255};
    normalColor = {64, 64, 64, 255};
    hoverColor = {96, 96, 96, 255};
    pressedColor = {32, 32, 32, 255};
}

void Button::render(Graphics& graphics) {
    if (!visible) return;
    
    SDL_Color currentColor = normalColor;
    if (!enabled) {
        currentColor = {32, 32, 32, 128};
    } else if (isPressed) {
        currentColor = pressedColor;
    } else if (isHovered) {
        currentColor = hoverColor;
    }
    
    graphics.setDrawColor(currentColor.r, currentColor.g, currentColor.b, currentColor.a);
    graphics.drawRect(x, y, width, height, true);
    
    graphics.setDrawColor(255, 255, 255, 255);
    graphics.drawRect(x, y, width, height, false);
    
    // テキストを中央に描画
    if (!text.empty()) {
        int textX = x + width / 2 - (text.length() * 6); // 簡易的な中央寄せ
        int textY = y + height / 2 - 8;
        graphics.drawText(text, textX, textY, fontName, textColor);
    }
}

bool Button::handleInput(const InputManager& input) {
    if (!visible || !enabled) return false;
    
    const MouseState& mouse = input.getMouseState();
    bool wasHovered = isHovered;
    isHovered = isPointInside(mouse.x, mouse.y);
    
    if (isHovered) {
        if (mouse.leftPressed && !isPressed) {
            isPressed = true;
        } else if (!mouse.leftPressed && isPressed) {
            isPressed = false;
            if (onClick) {
                onClick();
            }
            return true;
        }
    } else {
        isPressed = false;
    }
    
    if (!mouse.leftPressed) {
        isPressed = false;
    }
    
    return isHovered || wasHovered;
}

void Button::setColors(SDL_Color normal, SDL_Color hover, SDL_Color pressed) {
    normalColor = normal;
    hoverColor = hover;
    pressedColor = pressed;
}

// Label
Label::Label(int x, int y, const std::string& text, const std::string& fontName)
    : UIElement(x, y, 0, 0), text(text), fontName(fontName) {
    
    textColor = {255, 255, 255, 255};
    splitTextIntoLines();
}

Label::~Label() {
    // テクスチャは使用しないので何もしない
}

void Label::render(Graphics& graphics) {
    if (!visible || text.empty()) return;
    
    // 改行文字でテキストを分割して各ラインを個別に描画
    splitTextIntoLines();
    
    int currentY = y;
    for (const auto& line : lines) {
        if (!line.empty()) {
            graphics.drawText(line, x, currentY, fontName, textColor);
            // 次のラインの位置を計算（フォントサイズに応じて調整）
            currentY += 20; // 仮の行間
        }
    }
}

void Label::setText(const std::string& newText) {
    if (text != newText) {
        text = newText;
        splitTextIntoLines();
    }
}

void Label::splitTextIntoLines() {
    lines.clear();
    std::string currentLine;
    for (char c : text) {
        if (c == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
}



// UIManager
void UIManager::addElement(std::unique_ptr<UIElement> element) {
    elements.push_back(std::move(element));
}

void UIManager::clear() {
    elements.clear();
}

void UIManager::update(float deltaTime) {
    for (auto& element : elements) {
        element->update(deltaTime);
    }
}

void UIManager::render(Graphics& graphics) {
    for (auto& element : elements) {
        element->render(graphics);
    }
}

void UIManager::handleInput(const InputManager& input) {
    // 逆順で処理（後から追加された要素が優先）
    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
        if ((*it)->handleInput(input)) {
            break;
        }
    }
}

// StoryMessageBox
StoryMessageBox::StoryMessageBox(int x, int y, int width, int height, const std::string& fontName)
    : UIElement(x, y, width, height), fontName(fontName), graphics(nullptr), 
      displayTimer(0.0f), displayDuration(5.0f), padding(20), lineSpacing(5) {
    
    backgroundColor = {0, 0, 0, 200};     // 半透明黒背景
    textColor = {255, 255, 255, 255};     // 白文字
    borderColor = {255, 255, 255, 255};   // 白枠
    visible = false;  // 最初は非表示
}

StoryMessageBox::~StoryMessageBox() {
    clearTextures();
}

void StoryMessageBox::render(Graphics& graphics) {
    if (!visible) return;
    
    // テクスチャが未作成で、メッセージがある場合のみ作成
    if (lineTextures.empty() && !lines.empty()) {
        updateTextures(graphics);
    }
    
    // 背景描画（半透明黒）
    graphics.setDrawColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    graphics.drawRect(x, y, width, height, true);
    
    // 枠線描画（白）
    graphics.setDrawColor(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    graphics.drawRect(x, y, width, height, false);
    graphics.drawRect(x + 1, y + 1, width - 2, height - 2, false); // 二重枠
    
    // テキスト描画
    if (!lineTextures.empty()) {
        int currentY = y + padding;
        for (size_t i = 0; i < lineTextures.size(); ++i) {
            if (lineTextures[i]) {
                // テクスチャがある場合は描画
                graphics.drawTexture(lineTextures[i], x + padding, currentY);
                
                int textHeight;
                SDL_QueryTexture(lineTextures[i], nullptr, nullptr, nullptr, &textHeight);
                currentY += textHeight + lineSpacing;
            } else {
                // 空行の場合は、標準的な行の高さを追加
                currentY += 20 + lineSpacing; // 20は標準的なフォント高さ
            }
        }
    }
}

void StoryMessageBox::update(float deltaTime) {
    if (visible && displayTimer > 0) {
        displayTimer -= deltaTime;
        if (displayTimer <= 0) {
            visible = false;
        }
    }
}

void StoryMessageBox::setMessage(const std::vector<std::string>& messageLines) {
    lines = messageLines;
    // テクスチャを即座に作成せず、render時に遅延作成
    clearTextures();  // 既存のテクスチャをクリア
}

void StoryMessageBox::setMessage(const std::string& singleLineMessage) {
    lines.clear();
    lines.push_back(singleLineMessage);
    // テクスチャを即座に作成せず、render時に遅延作成
    clearTextures();  // 既存のテクスチャをクリア
}

void StoryMessageBox::show() {
    visible = true;
    displayTimer = displayDuration;
}

void StoryMessageBox::hide() {
    visible = false;
    displayTimer = 0;
}

void StoryMessageBox::updateTextures(Graphics& graphics) {
    clearTextures();
    this->graphics = &graphics;
    
    for (const std::string& line : lines) {
        if (!line.empty()) {
            SDL_Texture* texture = graphics.createTextTexture(line, fontName, textColor);
            if (texture) {
                lineTextures.push_back(texture);
            } else {
                lineTextures.push_back(nullptr); // テクスチャ作成失敗時
            }
        } else {
            lineTextures.push_back(nullptr); // 空行用
        }
    }
}

void StoryMessageBox::clearTextures() {
    for (SDL_Texture* texture : lineTextures) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    lineTextures.clear();
} 