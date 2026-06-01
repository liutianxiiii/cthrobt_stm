@echo off
setlocal
echo === Build STM32 firmware (Windows) ===
cd /d "%~dp0.."

REM -- arm-none-eabi-gcc --
where arm-none-eabi-gcc >nul 2>&1
if %errorlevel% neq 0 (
    set "ARM=%ProgramFiles(x86)%\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin"
    if exist "%ARM%\arm-none-eabi-gcc.exe" (
        set "PATH=%ARM%;%PATH%"
    ) else (
        echo ERROR: arm-none-eabi-gcc not found.
        echo Install: winget install Arm.GnuArmEmbeddedToolchain
        exit /b 1
    )
)

REM -- ninja (winget default location) --
where ninja >nul 2>&1
if %errorlevel% neq 0 (
    set "NINJA=%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe"
    if exist "%NINJA%\ninja.exe" (
        set "PATH=%NINJA%;%PATH%"
    ) else (
        echo ERROR: ninja not found.
        echo Install: winget install Ninja-build.Ninja
        exit /b 1
    )
)

REM -- cmake (pip or system) --
python -m cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: cmake not found.
    echo Install: winget install Kitware.CMake  or  pip install cmake
    exit /b 1
)

echo Configuring...
python -m cmake -B build\Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake\stm32_toolchain.cmake -G Ninja
if %errorlevel% neq 0 exit /b %errorlevel%

echo Building...
python -m cmake --build build\Debug
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Build complete: build\Debug\cthrobt.elf
