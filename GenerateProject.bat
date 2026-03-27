@echo off

REM
set CMAKE_CMD=cmake -G "Visual Studio 17 2022" -A x64 -B ./build .

REM
echo %CMAKE_CMD%
%CMAKE_CMD%

pause