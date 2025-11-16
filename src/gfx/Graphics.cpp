#include "Graphics.h"
#include <iostream>

Graphics::Graphics() : window(nullptr), renderer(nullptr), screenWidth(800), screenHeight(600) {
}

Graphics::~Graphics() {
    cleanup();
}

bool Graphics::initialize(const std::string& title, int width, int height) {
    screenWidth = width;
    screenHeight = height;
    
    // SDL初期化
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL初期化エラー: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // SDL_image初期化
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image初期化エラー: " << IMG_GetError() << std::endl;
        return false;
    }
    
    // SDL_ttf初期化
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf初期化エラー: " << TTF_GetError() << std::endl;
        return false;
    }
    
    window = SDL_CreateWindow(title.c_str(), 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED,
                              0, 0, 
                              SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    if (!window) {
        std::cerr << "ウィンドウ作成エラー: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // レンダラー作成
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "レンダラー作成エラー: " << SDL_GetError() << std::endl;
        return false;
    }
    
    SDL_RenderSetLogicalSize(renderer, width, height);
    
    screenWidth = width;
    screenHeight = height;
    
    // デフォルト描画色設定
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    
    return true;
}

void Graphics::cleanup() {
    // テクスチャ解放
    for (auto& pair : textures) {
        SDL_DestroyTexture(pair.second);
    }
    textures.clear();
    
    // フォント解放
    for (auto& pair : fonts) {
        TTF_CloseFont(pair.second);
    }
    fonts.clear();
    
    // SDL解放
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void Graphics::clear() {
    SDL_RenderClear(renderer);
}

void Graphics::present() {
    SDL_RenderPresent(renderer);
}

void Graphics::setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

SDL_Texture* Graphics::loadTexture(const std::string& filepath, const std::string& name) {
    SDL_Surface* surface = IMG_Load(filepath.c_str());
    if (!surface) {
        std::cerr << "画像読み込みエラー " << filepath << ": " << IMG_GetError() << std::endl;
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        std::cerr << "テクスチャ作成エラー " << filepath << ": " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    textures[name] = texture;
    return texture;
}

SDL_Texture* Graphics::getTexture(const std::string& name) {
    auto it = textures.find(name);
    return (it != textures.end()) ? it->second : nullptr;
}

void Graphics::drawTexture(const std::string& name, int x, int y, int width, int height) {
    SDL_Texture* texture = getTexture(name);
    if (texture) {
        drawTexture(texture, x, y, width, height);
    }
}

void Graphics::drawTexture(SDL_Texture* texture, int x, int y, int width, int height) {
    if (!texture) return;
    
    SDL_Rect dstRect = {x, y, width, height};
    
    if (width == -1 || height == -1) {
        SDL_QueryTexture(texture, nullptr, nullptr, &dstRect.w, &dstRect.h);
    }
    
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
}

void Graphics::drawTextureClip(const std::string& name, int x, int y, SDL_Rect* srcRect, SDL_Rect* dstRect) {
    SDL_Texture* texture = getTexture(name);
    if (!texture) return;
    
    SDL_Rect defaultDst = {x, y, 0, 0};
    if (!dstRect) {
        if (srcRect) {
            defaultDst.w = srcRect->w;
            defaultDst.h = srcRect->h;
        } else {
            SDL_QueryTexture(texture, nullptr, nullptr, &defaultDst.w, &defaultDst.h);
        }
        dstRect = &defaultDst;
    }
    
    SDL_RenderCopy(renderer, texture, srcRect, dstRect);
}

TTF_Font* Graphics::loadFont(const std::string& filepath, int size, const std::string& name) {
    TTF_Font* font = TTF_OpenFont(filepath.c_str(), size);
    if (!font) {
        std::cerr << "フォント読み込みエラー " << filepath << ": " << TTF_GetError() << std::endl;
        return nullptr;
    }
    
    fonts[name] = font;
    return font;
}

TTF_Font* Graphics::getFont(const std::string& name) {
    auto it = fonts.find(name);
    return (it != fonts.end()) ? it->second : nullptr;
}

void Graphics::drawText(const std::string& text, int x, int y, const std::string& fontName, SDL_Color color) {
    TTF_Font* font = getFont(fontName);
    if (!font) {
        std::cerr << "フォントが見つかりません: " << fontName << std::endl;
        return;
    }
    
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!textSurface) {
        std::cerr << "テキストサーフェス作成エラー: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    
    if (!textTexture) {
        std::cerr << "テキストテクスチャ作成エラー: " << SDL_GetError() << std::endl;
        return;
    }
    
    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);
    
    SDL_Rect dstRect = {x, y, textWidth, textHeight};
    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
    
    SDL_DestroyTexture(textTexture);
}

SDL_Texture* Graphics::createTextTexture(const std::string& text, const std::string& fontName, SDL_Color color) {
    TTF_Font* font = getFont(fontName);
    if (!font) return nullptr;
    
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!textSurface) return nullptr;
    
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    
    return textTexture;
}

void Graphics::drawRect(int x, int y, int width, int height, bool filled) {
    SDL_Rect rect = {x, y, width, height};
    
    if (filled) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void Graphics::drawLine(int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
} 