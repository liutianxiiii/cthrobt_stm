@echo off
echo === Clean build directory (Windows) ===
cd /d "%~dp0.."
if exist build (
    rmdir /s /q build
    echo Build directory removed.
) else (
    echo Nothing to clean.
)
