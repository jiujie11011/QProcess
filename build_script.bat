@echo off
set PATH=D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;%PATH%
cd /d "d:\VSCode Test\quiterss-0.19.4"
D:\Qt\5.14.2\mingw73_64\bin\qmake.exe QuiteRSS.pro
echo qmake exit code: %ERRORLEVEL%