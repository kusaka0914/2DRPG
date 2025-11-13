#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

class Graphics {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::unordered_map<std::string, SDL_Texture*> textures;
    std::unordered_map<std::string, TTF_Font*> fonts;
    int screenWidth;
    int screenHeight;

public:
    Graphics();
    ~Graphics();
    
    bool initialize(const std::string& title, int width, int height);
    void cleanup();
    
    // レンダリング
    void clear();
    void present();
    void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    
    // テクスチャ管理
    SDL_Texture* loadTexture(const std::string& filepath, const std::string& name);
    SDL_Texture* getTexture(const std::string& name);
    void drawTexture(const std::string& name, int x, int y, int width = -1, int height = -1);
    void drawTexture(SDL_Texture* texture, int x, int y, int width = -1, int height = -1);
    void drawTextureClip(const std::string& name, int x, int y, SDL_Rect* srcRect, SDL_Rect* dstRect = nullptr);
    
    // テキスト描画
    TTF_Font* loadFont(const std::string& filepath, int size, const std::string& name);
    TTF_Font* getFont(const std::string& name);
    void drawText(const std::string& text, int x, int y, const std::string& fontName, SDL_Color color = {255, 255, 255, 255});
    SDL_Texture* createTextTexture(const std::string& text, const std::string& fontName, SDL_Color color = {255, 255, 255, 255});
    
    // 基本図形
    void drawRect(int x, int y, int width, int height, bool filled = false);
    void drawLine(int x1, int y1, int x2, int y2);
    
    // ゲッター
    SDL_Renderer* getRenderer() { return renderer; }
    int getScreenWidth() const { return screenWidth; }
    int getScreenHeight() const { return screenHeight; }
}; 