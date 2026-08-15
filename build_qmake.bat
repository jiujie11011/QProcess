@echo off
set PATH=D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;C:\Windows\System32;C:\Windows
cd /d "d:\VSCode Test\quiterss-0.19.4"
D:\Qt\5.14.2\mingw73_64\bin\qmake.exe -spec win32-g++ "QMAKE_CC=D:\Qt\Tools\mingw730_64\bin\gcc.exe" "QMAKE_CXX=D:\Qt\Tools\mingw730_64\bin\g++.exe" "QMAKE_LINK=D:\Qt\Tools\mingw730_64\bin\g++.exe" "QMAKE_LINK_C=D:\Qt\Tools\mingw730_64\bin\gcc.exe" QuiteRSS.pro
if %ERRORLEVEL%==0 (
    echo qmake succeeded
) else (
    echo qmake failed with error code %ERRORLEVEL%
)
pause