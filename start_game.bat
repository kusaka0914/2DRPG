@echo off
REM ドラクエ風RPG 起動スクリプト (Windows)

REM スクリプトのディレクトリに移動
cd /d "%~dp0"

REM ビルドディレクトリが存在しない場合は作成
if not exist "build" (
    echo ビルドディレクトリを作成中・・・
    mkdir build
)

REM CMakeの設定（初回のみ）
if not exist "build\CMakeCache.txt" (
    echo CMakeの設定中・・・
    cd build
    cmake ..
    if errorlevel 1 (
        echo CMakeの設定に失敗しました。
        pause
        exit /b 1
    )
    cd ..
)

REM ビルド
echo ゲームをビルド中・・・
cd build
cmake --build . --config Release

if errorlevel 1 (
    echo ビルドに失敗しました。
    pause
    exit /b 1
)

REM ゲームを起動
echo ゲームを起動中・・・
if exist "Release\FallenHeroAndTheDoomedCapital.exe" (
    Release\FallenHeroAndTheDoomedCapital.exe
) else if exist "FallenHeroAndTheDoomedCapital.exe" (
    FallenHeroAndTheDoomedCapital.exe
) else (
    echo 実行ファイルが見つかりません。
    pause
    exit /b 1
)

cd ..

