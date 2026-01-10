@echo off
REM jester-gb Windows Build Script
REM Requires: Visual Studio 2019+ with C++ workload, CMake

echo Building jester-gb for Windows...

if not exist build mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed. Trying older VS version...
    cmake .. -G "Visual Studio 16 2019" -A x64
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build complete!
echo Executable: build\Release\jester-gb.exe
echo.

REM Create release package
if not exist ..\release mkdir ..\release
if not exist ..\release\jester-gb-windows-x64 mkdir ..\release\jester-gb-windows-x64
copy Release\jester-gb.exe ..\release\jester-gb-windows-x64\
copy ..\README.md ..\release\jester-gb-windows-x64\
if not exist ..\release\jester-gb-windows-x64\roms mkdir ..\release\jester-gb-windows-x64\roms

echo Release package created in: release\jester-gb-windows-x64\
pause
