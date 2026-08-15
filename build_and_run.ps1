$env:PATH = 'D:\Qt\Tools\mingw730_64\bin;D:\Qt\5.14.2\mingw73_64\bin;C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0'
Set-Location 'd:\VSCode Test\quiterss-0.19.4'

Write-Host '=== Checking compiler ===' -ForegroundColor Cyan
& 'D:\Qt\Tools\mingw730_64\bin\g++.exe' --version
Write-Host ''

Write-Host '=== Step 1: Running qmake ===' -ForegroundColor Green
& 'D:\Qt\5.14.2\mingw73_64\bin\qmake.exe' QuiteRSS.pro -spec win32-g++ 'QMAKE_CXX=D:\Qt\Tools\mingw730_64\bin\g++.exe' 'QMAKE_CC=D:\Qt\Tools\mingw730_64\bin\gcc.exe' 'QMAKE_LINK=D:\Qt\Tools\mingw730_64\bin\g++.exe' 'QMAKE_LINK_C=D:\Qt\Tools\mingw730_64\bin\gcc.exe' 2>&1 | Tee-Object -FilePath 'd:\VSCode Test\quiterss-0.19.4\build_log.txt'
if ($LASTEXITCODE -ne 0) {
    Write-Host "qmake failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host 'qmake succeeded' -ForegroundColor Green

Write-Host '=== Step 2: Running mingw32-make ===' -ForegroundColor Green
& 'D:\Qt\5.14.2\mingw73_64\bin\mingw32-make.exe' -j4 2>&1 | Tee-Object -FilePath 'd:\VSCode Test\quiterss-0.19.4\build_log.txt' -Append
if ($LASTEXITCODE -ne 0) {
    Write-Host "make failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host 'make succeeded' -ForegroundColor Green

Write-Host '=== Step 3: Running QuiteRSS ===' -ForegroundColor Green
$exePath = $null
if (Test-Path 'd:\VSCode Test\quiterss-0.19.4\release\QuiteRSS.exe') {
    $exePath = 'd:\VSCode Test\quiterss-0.19.4\release\QuiteRSS.exe'
} elseif (Test-Path 'd:\VSCode Test\quiterss-0.19.4\debug\QuiteRSS.exe') {
    $exePath = 'd:\VSCode Test\quiterss-0.19.4\debug\QuiteRSS.exe'
}
if ($exePath) {
    Write-Host "Launching: $exePath" -ForegroundColor Green
    Start-Process $exePath
} else {
    Write-Host 'QuiteRSS.exe not found! Searching...' -ForegroundColor Yellow
    Get-ChildItem -Path 'd:\VSCode Test\quiterss-0.19.4' -Recurse -Filter 'QuiteRSS.exe' | ForEach-Object { Write-Host "Found: $($_.FullName)" }
}