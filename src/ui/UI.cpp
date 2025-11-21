#include "UI.h"
#include <iostream>

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
    
    if (!text.empty()) {
        // フォントが読み込まれているか確認（Windows特有の問題：フォントが読み込まれていない場合にクラッシュする可能性がある）
        if (graphics.getFont(fontName)) {
            int textX = x + width / 2 - (text.length() * 6); // 簡易的な中央寄せ
            int textY = y + height / 2 - 8;
            try {
                graphics.drawText(text, textX, textY, fontName, textColor);
            } catch (const std::exception& e) {
                std::cerr << "Button::render drawTextエラー: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Button::render drawTextエラー: 不明なエラー" << std::endl;
            }
        }
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
}

void Label::render(Graphics& graphics) {
    if (!visible || text.empty()) return;
    
    // フォントが読み込まれているか確認（Windows特有の問題：フォントが読み込まれていない場合にクラッシュする可能性がある）
    if (!graphics.getFont(fontName)) {
        return; // フォントが読み込まれていない場合は描画をスキップ
    }
    
    splitTextIntoLines();
    
    int currentY = y;
    for (const auto& line : lines) {
        if (!line.empty()) {
            try {
                graphics.drawText(line, x, currentY, fontName, textColor);
            } catch (const std::exception& e) {
                std::cerr << "Label::render drawTextエラー: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Label::render drawTextエラー: 不明なエラー" << std::endl;
            }
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
    // elementsが変更される可能性があるため、コピーを作成してループ
    // ただし、パフォーマンスを考慮して、通常は直接ループする
    // Windows特有の問題：elementsが変更される可能性があるため、安全のためチェック
    try {
        // elementsが空の場合は何もしない
        if (elements.empty()) {
            return;
        }
        
        // 各要素を個別にtry-catchで囲む（1つの要素でエラーが発生しても他の要素は描画する）
        for (auto& element : elements) {
            if (element) {
                try {
                    element->render(graphics);
                } catch (const std::exception& e) {
                    std::cerr << "UIManager::render: 個別UI要素の描画中にエラー: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "UIManager::render: 個別UI要素の描画中に不明なエラー" << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "UIManager::renderエラー: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UIManager::renderエラー: 不明なエラー" << std::endl;
    }
}

void UIManager::handleInput(const InputManager& input) {
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

void StoryMessageBox::render(Graphics& gfx) {
    if (!visible) return;
    
    if (lineTextures.empty() && !lines.empty()) {
        updateTextures(gfx);
    }
    
    // 背景描画（半透明黒）
    gfx.setDrawColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    gfx.drawRect(x, y, width, height, true);
    
    // 枠線描画（白）
    gfx.setDrawColor(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    gfx.drawRect(x, y, width, height, false);
    gfx.drawRect(x + 1, y + 1, width - 2, height - 2, false); // 二重枠
    
    // テキスト描画
    if (!lineTextures.empty()) {
        int currentY = y + padding;
        for (size_t i = 0; i < lineTextures.size(); ++i) {
            if (lineTextures[i]) {
                // テクスチャがある場合は描画
                gfx.drawTexture(lineTextures[i], x + padding, currentY);
                
                int textHeight = 20; // デフォルト値
                if (SDL_QueryTexture(lineTextures[i], nullptr, nullptr, nullptr, &textHeight) != 0) {
                    // エラーが発生した場合はデフォルト値を使用
                    textHeight = 20;
                }
                // 無効な値の場合はデフォルト値を使用
                if (textHeight <= 0) {
                    textHeight = 20;
                }
                currentY += textHeight + lineSpacing;
            } else {
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
    clearTextures();  // 既存のテクスチャをクリア
}

void StoryMessageBox::setMessage(const std::string& singleLineMessage) {
    lines.clear();
    lines.push_back(singleLineMessage);
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

void StoryMessageBox::updateTextures(Graphics& gfx) {
    clearTextures();
    this->graphics = &gfx;
    
    // フォントが読み込まれているか確認（Windows特有の問題：フォントが読み込まれていない場合にクラッシュする可能性がある）
    if (!gfx.getFont(fontName)) {
        // フォントが読み込まれていない場合は空のテクスチャリストを返す
        for (size_t i = 0; i < lines.size(); ++i) {
            lineTextures.push_back(nullptr);
        }
        return;
    }
    
    for (const std::string& line : lines) {
        if (!line.empty()) {
            try {
                SDL_Texture* texture = gfx.createTextTexture(line, fontName, textColor);
                if (texture) {
                    lineTextures.push_back(texture);
                } else {
                    lineTextures.push_back(nullptr); // テクスチャ作成失敗時
                }
            } catch (const std::exception& e) {
                std::cerr << "StoryMessageBox::updateTextures createTextTextureエラー: " << e.what() << std::endl;
                lineTextures.push_back(nullptr);
            } catch (...) {
                std::cerr << "StoryMessageBox::updateTextures createTextTextureエラー: 不明なエラー" << std::endl;
                lineTextures.push_back(nullptr);
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