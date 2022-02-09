@echo off

echo Cleaning Visual Studio solution...
2>NUL del "%~dp0*.sln"

echo Cleaning Visual Studio cache...
2>NUL rmdir /s /q "%~dp0/.vs"

echo Cleaning bin directory...
2>NUL rmdir /s /q "%~dp0/bin"

echo Cleaning build directory...
2>NUL rmdir /s /q "%~dp0/build"

echo Cleaning BatteryEngine...
call "%~dp0modules/BatteryEngine/clean-win.bat"

echo Done

Timeout 5