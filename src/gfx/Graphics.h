/**
 * @file Graphics.h
 * @brief グラフィックス描画を担当するクラス
 * @details SDL2を使用したテクスチャ管理、フォント管理、描画処理を提供する。
 */

#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>

/**
 * @brief グラフィックス描画を担当するクラス
 * @details SDL2を使用したテクスチャ管理、フォント管理、描画処理を提供する。
 */
class Graphics {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::unordered_map<std::string, SDL_Texture*> textures;
    std::unordered_map<std::string, TTF_Font*> fonts;
    int screenWidth;
    int screenHeight;

public:
    /**
     * @brief コンストラクタ
     */
    Graphics();
    
    /**
     * @brief デストラクタ
     */
    ~Graphics();
    
    /**
     * @brief 初期化
     * @param title ウィンドウタイトル
     * @param width 画面幅
     * @param height 画面高さ
     * @return 初期化が成功したか
     */
    bool initialize(const std::string& title, int width, int height);
    
    /**
     * @brief クリーンアップ
     */
    void cleanup();
    
    /**
     * @brief 画面クリア
     */
    void clear();
    
    /**
     * @brief 画面更新
     */
    void present();
    
    /**
     * @brief 描画色の設定
     * @param r 赤成分
     * @param g 緑成分
     * @param b 青成分
     * @param a アルファ成分（デフォルト: 255）
     */
    void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    
    /**
     * @brief テクスチャの読み込み
     * @param filepath ファイルパス
     * @param name テクスチャ名
     * @return テクスチャへのポインタ
     */
    SDL_Texture* loadTexture(const std::string& filepath, const std::string& name);
    
    /**
     * @brief テクスチャの取得
     * @param name テクスチャ名
     * @return テクスチャへのポインタ
     */
    SDL_Texture* getTexture(const std::string& name);
    
    /**
     * @brief テクスチャの描画（名前指定）
     * @param name テクスチャ名
     * @param x X座標
     * @param y Y座標
     * @param width 幅（-1の場合は元のサイズ）
     * @param height 高さ（-1の場合は元のサイズ）
     */
    void drawTexture(const std::string& name, int x, int y, int width = -1, int height = -1);
    
    /**
     * @brief テクスチャの描画（ポインタ指定）
     * @param texture テクスチャへのポインタ
     * @param x X座標
     * @param y Y座標
     * @param width 幅（-1の場合は元のサイズ）
     * @param height 高さ（-1の場合は元のサイズ）
     */
    void drawTexture(SDL_Texture* texture, int x, int y, int width = -1, int height = -1);
    
    /**
     * @brief アスペクト比を保持したテクスチャの描画
     * @details テクスチャの元のアスペクト比を保持しながら、基準サイズに合わせて描画する。
     * 横長の画像は幅を基準に、縦長の画像は高さを基準にサイズを計算する。
     * 
     * @param texture テクスチャへのポインタ
     * @param x X座標（画像の中心X座標）
     * @param y Y座標（画像の中心Y座標）
     * @param baseSize 基準サイズ（横長の画像は幅、縦長の画像は高さとして使用）
     * @param centerX 画像をX座標を中心に配置するか（デフォルト: true）
     * @param centerY 画像をY座標を中心に配置するか（デフォルト: true）
     */
    void drawTextureAspectRatio(SDL_Texture* texture, int x, int y, int baseSize, bool centerX = true, bool centerY = true);
    
    /**
     * @brief テクスチャのクリッピング描画
     * @param name テクスチャ名
     * @param x X座標
     * @param y Y座標
     * @param srcRect ソース矩形（nullptrの場合は全体）
     * @param dstRect デスティネーション矩形（nullptrの場合は元のサイズ）
     */
    void drawTextureClip(const std::string& name, int x, int y, SDL_Rect* srcRect, SDL_Rect* dstRect = nullptr);
    
    /**
     * @brief フォントの読み込み
     * @param filepath ファイルパス
     * @param size フォントサイズ
     * @param name フォント名
     * @return フォントへのポインタ
     */
    TTF_Font* loadFont(const std::string& filepath, int size, const std::string& name);
    
    /**
     * @brief フォントの取得
     * @param name フォント名
     * @return フォントへのポインタ
     */
    TTF_Font* getFont(const std::string& name);
    
    /**
     * @brief テキストの描画
     * @param text テキスト
     * @param x X座標
     * @param y Y座標
     * @param fontName フォント名
     * @param color 色（デフォルト: 白）
     */
    void drawText(const std::string& text, int x, int y, const std::string& fontName, SDL_Color color = {255, 255, 255, 255});
    
    /**
     * @brief テキストテクスチャの作成
     * @param text テキスト
     * @param fontName フォント名
     * @param color 色（デフォルト: 白）
     * @return テキストテクスチャへのポインタ
     */
    SDL_Texture* createTextTexture(const std::string& text, const std::string& fontName, SDL_Color color = {255, 255, 255, 255});
    
    /**
     * @brief 矩形の描画
     * @param x X座標
     * @param y Y座標
     * @param width 幅
     * @param height 高さ
     * @param filled 塗りつぶすか（デフォルト: false）
     */
    void drawRect(int x, int y, int width, int height, bool filled = false);
    
    /**
     * @brief 線の描画
     * @param x1 始点X座標
     * @param y1 始点Y座標
     * @param x2 終点X座標
     * @param y2 終点Y座標
     */
    void drawLine(int x1, int y1, int x2, int y2);
    
    /**
     * @brief レンダラーの取得
     * @return SDLレンダラーへのポインタ
     */
    SDL_Renderer* getRenderer() { return renderer; }
    
    /**
     * @brief 画面幅の取得
     * @return 画面幅
     */
    int getScreenWidth() const { return screenWidth; }
    
    /**
     * @brief 画面高さの取得
     * @return 画面高さ
     */
    int getScreenHeight() const { return screenHeight; }
}; 