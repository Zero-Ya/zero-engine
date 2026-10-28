@echo off
cd /d %~dp0

echo [INFO] Configuring ZEngine using CMake Presets (Debug)...
cmake --preset debug

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake generation failed! Please ensure the Vulkan SDK is installed.
    pause
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Project files generated successfully inside the /build folder!
pause