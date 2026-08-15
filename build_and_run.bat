@echo off
set PATH=D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0
cd /d "d:\VSCode Test\quiterss-0.19.4"

echo === Step 1: Running qmake ===
D:\Qt\5.14.2\mingw73_64\bin\qmake.exe -spec win32-g++ "QMAKE_CC=D:\Qt\Tools\mingw730_64\bin\gcc.exe" "QMAKE_CXX=D:\Qt\Tools\mingw730_64\bin\g++.exe" "QMAKE_LINK=D:\Qt\Tools\mingw730_64\bin\g++.exe" "QMAKE_LINK_C=D:\Qt\Tools\mingw730_64\bin\gcc.exe" QuiteRSS.pro
if %ERRORLEVEL% neq 0 (
    echo qmake failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo qmake succeeded

echo === Step 2: Running mingw32-make ===
D:\Qt\5.14.2\mingw73_64\bin\mingw32-make.exe -j4
if %ERRORLEVEL% neq 0 (
    echo make failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo make succeeded

echo === Step 3: Running QuiteRSS ===
if exist "release\QuiteRSS.exe" (
    echo Launching release\QuiteRSS.exe
    start "" "release\QuiteRSS.exe"
) else if exist "debug\QuiteRSS.exe" (
    echo Launching debug\QuiteRSS.exe
    start "" "debug\QuiteRSS.exe"
) else (
    echo QuiteRSS.exe not found, searching...
    dir /s /b QuiteRSS.exe 2>nul
)