import os
import subprocess
import traceback

log_path = r'd:\VSCode Test\quiterss-0.19.4\build_log2.txt'

try:
    with open(log_path, 'w', encoding='utf-8') as log:
        env = os.environ.copy()
        env['PATH'] = r'D:\Qt\5.14.2\mingw73_64\bin;C:\Windows\System32;C:\Windows'

        log.write(f"PATH: {env['PATH']}\n\n")

        log.write("Step 1: Testing g++ directly...\n")
        r = subprocess.run(
            [r'D:\Qt\5.14.2\mingw73_64\bin\g++.exe', '--version'],
            capture_output=True, text=True, timeout=30, env=env
        )
        log.write(f"g++ stdout: {r.stdout[:500]}\n")
        log.write(f"g++ stderr: {r.stderr[:500]}\n")
        log.write(f"g++ return: {r.returncode}\n\n")

        log.write("Step 2: Testing gcc directly...\n")
        r1b = subprocess.run(
            [r'D:\Qt\5.14.2\mingw73_64\bin\gcc.exe', '--version'],
            capture_output=True, text=True, timeout=30, env=env
        )
        log.write(f"gcc stdout: {r1b.stdout[:500]}\n")
        log.write(f"gcc stderr: {r1b.stderr[:500]}\n")
        log.write(f"gcc return: {r1b.returncode}\n\n")

        log.write("Step 3: Testing simple compile...\n")
        test_src = r'd:\VSCode Test\quiterss-0.19.4\test_compile.cpp'
        with open(test_src, 'w') as f:
            f.write('int main() { return 0; }\n')
        r1c = subprocess.run(
            [r'D:\Qt\5.14.2\mingw73_64\bin\g++.exe', '-c', test_src, '-o', r'd:\VSCode Test\quiterss-0.19.4\test_compile.o'],
            capture_output=True, text=True, timeout=30, env=env
        )
        log.write(f"compile stdout: {r1c.stdout}\n")
        log.write(f"compile stderr: {r1c.stderr}\n")
        log.write(f"compile return: {r1c.returncode}\n\n")

        log.write("Step 4: Running qmake with spec and compiler overrides...\n")
        r2 = subprocess.run(
            [r'D:\Qt\5.14.2\mingw73_64\bin\qmake.exe',
             'QuiteRSS.pro',
             '-spec', 'win32-g++',
             'QMAKE_CXX=D:\\Qt\\5.14.2\\mingw73_64\\bin\\g++.exe',
             'QMAKE_CC=D:\\Qt\\5.14.2\\mingw73_64\\bin\\gcc.exe',
             'QMAKE_LINK=D:\\Qt\\5.14.2\\mingw73_64\\bin\\g++.exe',
             'QMAKE_LINK_C=D:\\Qt\\5.14.2\\mingw73_64\\bin\\gcc.exe'],
            capture_output=True, text=True, timeout=120, env=env,
            cwd=r'd:\VSCode Test\quiterss-0.19.4'
        )
        log.write(f"qmake stdout: {r2.stdout}\n")
        log.write(f"qmake stderr: {r2.stderr}\n")
        log.write(f"qmake return: {r2.returncode}\n\n")

        if r2.returncode == 0:
            log.write("=== qmake succeeded! Running make... ===\n")
            r3 = subprocess.run(
                [r'D:\Qt\5.14.2\mingw73_64\bin\mingw32-make.exe', '-j4'],
                capture_output=True, text=True, timeout=1800, env=env,
                cwd=r'd:\VSCode Test\quiterss-0.19.4'
            )
            log.write(f"make stdout (last 3000): {r3.stdout[-3000:]}\n")
            log.write(f"make stderr (last 3000): {r3.stderr[-3000:]}\n")
            log.write(f"make return: {r3.returncode}\n")
        else:
            log.write("=== qmake failed ===\n")

except Exception as e:
    with open(log_path, 'a', encoding='utf-8') as log:
        log.write(f"\nEXCEPTION: {e}\n")
        traceback.print_exc(file=log)