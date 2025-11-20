/**
 * @file AudioManager.h
 * @brief オーディオ管理を担当するクラス
 * @details SDL2_mixerを使用してBGMや効果音の再生を管理する。
 */

#pragma once
#include <SDL2/SDL_mixer.h>
#include <string>
#include <memory>
#include <unordered_map>

/**
 * @brief オーディオ管理を担当するクラス
 * @details SDL2_mixerを使用してBGMや効果音の再生を管理する。
 */
class AudioManager {
private:
    std::unordered_map<std::string, Mix_Music*> musicMap;
    std::unordered_map<std::string, Mix_Chunk*> soundMap;
    bool isInitialized;
    std::string currentPlayingMusic;  /**< @brief 現在再生中の音楽名 */
    
    /**
     * @brief コンストラクタ（シングルトン）
     */
    AudioManager();
    
    /**
     * @brief デストラクタ
     */
    ~AudioManager();

public:
    /**
     * @brief コピーコンストラクタ（削除）
     */
    AudioManager(const AudioManager&) = delete;
    
    /**
     * @brief 代入演算子（削除）
     */
    AudioManager& operator=(const AudioManager&) = delete;
    
    /**
     * @brief インスタンスの取得
     * @return AudioManagerへの参照
     */
    static AudioManager& getInstance();
    
    /**
     * @brief 初期化
     * @return 初期化が成功したか
     */
    bool initialize();
    
    /**
     * @brief クリーンアップ
     */
    void cleanup();
    
    /**
     * @brief BGMの読み込み
     * @param filePath ファイルパス
     * @param name 登録名
     * @return 読み込みが成功したか
     */
    bool loadMusic(const std::string& filePath, const std::string& name);
    
    /**
     * @brief BGMの再生
     * @param name 登録名
     * @param loops ループ回数（-1で無限ループ）
     * @return 再生が成功したか
     */
    bool playMusic(const std::string& name, int loops = -1);
    
    /**
     * @brief BGMの停止
     */
    void stopMusic();
    
    /**
     * @brief BGMの一時停止
     */
    void pauseMusic();
    
    /**
     * @brief BGMの再開
     */
    void resumeMusic();
    
    /**
     * @brief BGMの音量設定
     * @param volume 音量（0-128）
     */
    void setMusicVolume(int volume);
    
    /**
     * @brief BGMが再生中かどうか
     * @return 再生中かどうか
     */
    bool isMusicPlaying() const;
    
    /**
     * @brief 効果音の読み込み
     * @param filePath ファイルパス
     * @param name 登録名
     * @return 読み込みが成功したか
     */
    bool loadSound(const std::string& filePath, const std::string& name);
    
    /**
     * @brief 効果音の再生
     * @param name 登録名
     * @param loops ループ回数（0で1回再生、-1で無限ループ）
     * @param channel 再生チャンネル（-1で自動割り当て）
     * @return 再生が成功したか
     */
    bool playSound(const std::string& name, int loops = 0, int channel = -1);
    
    /**
     * @brief 効果音の音量設定
     * @param volume 音量（0-128）
     */
    void setSoundVolume(int volume);
};

