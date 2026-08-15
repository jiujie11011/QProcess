import os
import subprocess
import sys
import traceback

log_path = r'd:\VSCode Test\quiterss-0.19.4\build_log.txt'
project_dir = r'd:\VSCode Test\quiterss-0.19.4'

try:
    with open(log_path, 'w', encoding='utf-8') as log:
        env = os.environ.copy()
        env['PATH'] = r'D:\Qt\Tools\mingw730_64\bin;D:\Qt\5.14.2\mingw73_64\bin;C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0'

        log.write(f"PATH: {env['PATH']}\n\n")

        log.write("Step 1: Testing g++ compiler...\n")
        r = subprocess.run(
            [r'D:\Qt\Tools\mingw730_64\bin\g++.exe', '--version'],
            capture_output=True, text=True, timeout=30, env=env
        )
        log.write(f"g++ stdout: {r.stdout[:200]}\n")
        log.write(f"g++ stderr: {r.stderr[:200]}\n")
        log.write(f"g++ return: {r.returncode}\n\n")

        if r.returncode != 0:
            log.write("g++ test failed, trying Qt bin g++...\n")
            r = subprocess.run(
                [r'D:\Qt\5.14.2\mingw73_64\bin\g++.exe', '--version'],
                capture_output=True, text=True, timeout=30, env=env
            )
            log.write(f"g++ stdout: {r.stdout[:200]}\n")
            log.write(f"g++ return: {r.returncode}\n\n")

        log.write("Step 2: Running qmake...\n")
        os.chdir(project_dir)
        r2 = subprocess.run(
            [r'D:\Qt\5.14.2\mingw73_64\bin\qmake.exe',
             'QuiteRSS.pro',
             '-spec', 'win32-g++',
             'QMAKE_CXX=D:\\Qt\\Tools\\mingw730_64\\bin\\g++.exe',
             'QMAKE_CC=D:\\Qt\\Tools\\mingw730_64\\bin\\gcc.exe',
             'QMAKE_LINK=D:\\Qt\\Tools\\mingw730_64\\bin\\g++.exe',
             'QMAKE_LINK_C=D:\\Qt\\Tools\\mingw730_64\\bin\\gcc.exe'],
            capture_output=True, text=True, timeout=120, env=env,
            cwd=project_dir
        )
        log.write(f"qmake stdout: {r2.stdout}\n")
        log.write(f"qmake stderr: {r2.stderr}\n")
        log.write(f"qmake return: {r2.returncode}\n\n")

        if r2.returncode == 0:
            log.write("=== qmake succeeded! Running make... ===\n")
            print("qmake succeeded, running mingw32-make...")
            r3 = subprocess.run(
                [r'D:\Qt\5.14.2\mingw73_64\bin\mingw32-make.exe', '-j4'],
                env=env,
                cwd=project_dir
            )
            log.write(f"make return: {r3.returncode}\n")

            if r3.returncode == 0:
                log.write("=== Build succeeded! ===\n")
                print("Build succeeded! Looking for QuiteRSS.exe...")

                import glob
                exe_files = glob.glob(os.path.join(project_dir, '**', 'QuiteRSS.exe'), recursive=True)
                if exe_files:
                    exe_path = exe_files[0]
                    log.write(f"Found: {exe_path}\n")
                    print(f"Launching: {exe_path}")
                    subprocess.Popen([exe_path], cwd=os.path.dirname(exe_path), env=env)
                else:
                    log.write("QuiteRSS.exe not found!\n")
                    print("QuiteRSS.exe not found!")
            else:
                log.write("=== make failed ===\n")
                print("make failed!")
        else:
            log.write("=== qmake failed ===\n")
            print("qmake failed!")

except Exception as e:
    with open(log_path, 'a', encoding='utf-8') as log:
        log.write(f"\nEXCEPTION: {e}\n")
        traceback.print_exc(file=log)
    print(f"Exception: {e}")