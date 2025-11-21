#include "AudioManager.h"
#include <iostream>

AudioManager::AudioManager() : isInitialized(false), currentPlayingMusic("") {
}

AudioManager::~AudioManager() {
    cleanup();
}

AudioManager& AudioManager::getInstance() {
    static AudioManager instance;
    return instance;
}

bool AudioManager::initialize() {
    if (isInitialized) {
        return true;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer初期化エラー: " << Mix_GetError() << std::endl;
        return false;
    }
    
    isInitialized = true;
    return true;
}

void AudioManager::cleanup() {
    if (!isInitialized) {
        return;
    }
    
    stopMusic();
    
    for (auto& pair : musicMap) {
        if (pair.second) {
            Mix_FreeMusic(pair.second);
        }
    }
    musicMap.clear();
    
    for (auto& pair : soundMap) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    soundMap.clear();
    
    Mix_CloseAudio();
    isInitialized = false;
}

bool AudioManager::loadMusic(const std::string& filePath, const std::string& name) {
    if (!isInitialized) {
        std::cerr << "AudioManagerが初期化されていません" << std::endl;
        return false;
    }
    
    // 既に読み込まれている場合は解放
    if (musicMap.find(name) != musicMap.end()) {
        Mix_FreeMusic(musicMap[name]);
    }
    
    Mix_Music* music = Mix_LoadMUS(filePath.c_str());
    if (!music) {
        std::cerr << "BGM読み込みエラー (" << filePath << "): " << Mix_GetError() << std::endl;
        return false;
    }
    
    musicMap[name] = music;
    return true;
}

bool AudioManager::playMusic(const std::string& name, int loops) {
    if (!isInitialized) {
        std::cerr << "AudioManagerが初期化されていません" << std::endl;
        return false;
    }
    
    if (musicMap.find(name) == musicMap.end()) {
        std::cerr << "BGMが見つかりません: " << name << std::endl;
        return false;
    }
    
    // 既に同じ音楽が再生中の場合は何もしない（音楽が切れないようにする）
    if (currentPlayingMusic == name && isMusicPlaying()) {
        return true;
    }
    
    if (Mix_PlayMusic(musicMap[name], loops) == -1) {
        std::cerr << "BGM再生エラー: " << Mix_GetError() << std::endl;
        return false;
    }
    
    currentPlayingMusic = name;
    return true;
}

void AudioManager::stopMusic() {
    if (isInitialized) {
        Mix_HaltMusic();
        currentPlayingMusic = "";
    }
}

void Audi