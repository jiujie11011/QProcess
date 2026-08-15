import os
import subprocess
import sys

env = os.environ.copy()
env['PATH'] = r'D:\Qt\5.14.2\mingw73_64\bin;C:\Windows\System32;C:\Windows'

os.chdir(r'd:\VSCode Test\quiterss-0.19.4')

result = subprocess.run(
    [r'D:\Qt\5.14.2\mingw73_64\bin\qmake.exe', 'QuiteRSS.pro'],
    capture_output=True,
    text=True,
    timeout=120,
    env=env,
    cwd=r'd:\VSCode Test\quiterss-0.19.4'
)

print("STDOUT:", result.stdout)
print("STDERR:", result.stderr)
print("Return code:", result.returncode)

if result.returncode == 0:
    print("=== qmake succeeded! ===")
else:
    print("=== qmake failed! ===")