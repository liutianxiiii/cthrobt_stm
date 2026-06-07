@echo off
setlocal enabledelayedexpansion
echo === cthrobt_stm Setup (Windows) ===
echo.

REM ── 1. arm-none-eabi-gcc ────────────────────────────────────────────────────
where arm-none-eabi-gcc >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] arm-none-eabi-gcc found in PATH.
) else (
    set "ARM=%ProgramFiles(x86)%\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin"
    if exist "!ARM!\arm-none-eabi-gcc.exe" (
        set "PATH=!ARM!;%PATH%"
        echo [OK] arm-none-eabi-gcc found at default install path.
    ) else (
        echo [INSTALL] arm-none-eabi-gcc...
        winget install --id Arm.GnuArmEmbeddedToolchain -e --silent
        if %errorlevel% neq 0 (
            echo ERROR: winget install failed. Download manually:
            echo   https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
            exit /b 1
        )
        echo [OK] arm-none-eabi-gcc installed. Re-open this terminal to update PATH.
    )
)

REM ── 2. cmake ────────────────────────────────────────────────────────────────
python -m cmake --version >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] cmake (python module) found.
) else (
    where cmake >nul 2>&1
    if %errorlevel% equ 0 (
        echo [OK] cmake found in PATH.
    ) else (
        echo [INSTALL] cmake...
        winget install --id Kitware.CMake -e --silent
        if %errorlevel% neq 0 (
            pip install cmake
            if %errorlevel% neq 0 (
                echo ERROR: cmake install failed. Install manually: winget install Kitware.CMake
                exit /b 1
            )
        )
        echo [OK] cmake installed.
    )
)

REM ── 3. ninja ────────────────────────────────────────────────────────────────
where ninja >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] ninja found in PATH.
) else (
    set "NINJA=%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"
    if exist "!NINJA!\ninja.exe" (
        set "PATH=!NINJA!;%PATH%"
        echo [OK] ninja found at default winget path.
    ) else (
        echo [INSTALL] ninja...
        winget install --id Ninja-build.Ninja -e --silent
        if %errorlevel% neq 0 (
            pip install ninja
            if %errorlevel% neq 0 (
                echo ERROR: ninja install failed. Install manually: winget install Ninja-build.Ninja
                exit /b 1
            )
        )
        echo [OK] ninja installed.
    )
)

REM ── 4. python3 ──────────────────────────────────────────────────────────────
where python >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] python found.
) else (
    echo [INSTALL] python...
    winget install --id Python.Python.3 -e --silent
    if %errorlevel% neq 0 (
        echo ERROR: python install failed. Download: https://www.python.org/downloads/
        exit /b 1
    )
    echo [OK] python installed.
)

REM ── 5. renode ───────────────────────────────────────────────────────────────
where renode >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] renode found in PATH.
) else (
    if exist "%ProgramFiles%\Renode\bin\Renode.exe" (
        echo [OK] renode found at default install path.
    ) else (
        echo [INSTALL] Renode...
        winget install --id Renode.Renode -e --silent
        if %errorlevel% neq 0 (
            echo ERROR: Renode install failed. Download: https://renode.io
            exit /b 1
        )
        echo [OK] Renode installed.
    )
)

REM ── 6. Vendor code (Drivers/ and Middlewares/) ──────────────────────────────
echo.
if exist "%~dp0..\Drivers" if exist "%~dp0..\Middlewares" (
    echo [OK] Drivers\ and Middlewares\ already present.
    goto setup_done
)

echo +-------------------------------------------------------------------+
echo ^|  ACTION REQUIRED: Regenerate vendor code                          ^|
echo ^|                                                                   ^|
echo ^|  Drivers\ and Middlewares\ are not tracked in git.               ^|
echo ^|  Regenerate them from the CubeMX project file:                   ^|
echo ^|                                                                   ^|
echo ^|    1. Open STM32CubeMX                                            ^|
echo ^|    2. File -^> Load Project -^> select cthrobt.ioc                ^|
echo ^|    3. Project -^> Generate Code                                   ^|
echo ^|                                                                   ^|
echo ^|  After that, run: scripts\build.bat                              ^|
echo +-------------------------------------------------------------------+

:setup_done
echo.
echo === Setup complete. Next: scripts\build.bat ===
