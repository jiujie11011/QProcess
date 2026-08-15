$env:PATH = "D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;D:\Qt\5.14.2\mingw73_64\lib"
Set-Location "d:\VSCode Test\quiterss-0.19.4"
& "D:\Qt\5.14.2\mingw73_64\bin\qmake.exe" QuiteRSS.pro 2>&1 | Out-File -FilePath "d:\VSCode Test\quiterss-0.19.4\qmake_result.txt" -Encoding UTF8
Write-Output "Complete"