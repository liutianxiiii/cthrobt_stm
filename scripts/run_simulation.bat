@echo off
setlocal
echo === Run Renode Simulation (Windows) ===
echo This starts Renode with cthrobt.elf and waits for controller.py on port 7777.
echo Run scripts\run_mock.bat (in cthrobt) in a separate terminal to send data.
echo.

if not exist "%~dp0..\build\Debug\cthrobt.elf" (
    echo ERROR: Firmware not built. Run scripts\build.bat first.
    exit /b 1
)

cd /d "%~dp0.."
python renode\bridge.py %*
