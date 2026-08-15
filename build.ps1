$env:PATH = 'D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;C:\Program Files\Python39\Scripts;C:\Program Files\Python39;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;D:\Qt_builder_venv\Scripts'
Set-Location 'd:\VSCode Test\quiterss-0.19.4'
& 'D:\Qt\5.14.2\mingw73_64\bin\g++.exe' --version 2>&1 | Out-File -FilePath 'd:\VSCode Test\quiterss-0.19.4\ps_build_log.txt' -Encoding utf8
& 'D:\Qt\5.14.2\mingw73_64\bin\qmake.exe' QuiteRSS.pro -spec win32-g++ 'QMAKE_CXX=D:\Qt\5.14.2\mingw73_64\bin\g++.exe' 'QMAKE_CC=D:\Qt\5.14.2\mingw73_64\bin\gcc.exe' 'QMAKE_LINK=D:\Qt\5.14.2\mingw73_64\bin\g++.exe' 'QMAKE_LINK_C=D:\Qt\5.14.2\mingw73_64\bin\gcc.exe' 2>&1 | Out-File -FilePath 'd:\VSCode Test\quiterss-0.19.4\ps_build_log.txt' -Encoding utf8 -Append
if ($LASTEXITCODE -eq 0) {
    Add-Content -Path 'd:\VSCode Test\quiterss-0.19.4\ps_build_log.txt' -Value '=== qmake succeeded, running make ===' -Encoding utf8
    & 'D:\Qt\5.14.2\mingw73_64\bin\mingw32-make.exe' -j4 2>&1 | Out-File -FilePath 'd:\VSCode Test\quiterss-0.19.4\ps_build_log.txt' -Encoding utf8 -Append
    Add-Content -Path 'd:\VSCode Test\quiterss-0.19.4\ps_build_log.txt' -Value "=== make exit code: $LASTEXITCODE ===" -Encoding utf8
} else {
    Add-Content -Path 'd:\VSCode Test\quiterss-0.19.4\ps_build_log.txt' -Value "=== qmake failed with exit code: $LASTEXITCODE ===" -Encoding utf8
}