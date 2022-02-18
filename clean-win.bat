@echo off

echo Cleaning Visual Studio solution...
2>NUL del "%~dp0*.sln"

echo Cleaning Visual Studio cache...
2>NUL rmdir /s /q "%~dp0.vs"

echo Cleaning bin directory...
2>NUL rmdir /s /q "%~dp0bin"

echo Cleaning build directory...
2>NUL rmdir /s /q "%~dp0build"

echo Cleaning BatteryEngine...
call "%~dp0modules/BatteryEngine/clean-win.bat"

echo Cleaning libusbcpp...
2>NUL rmdir /s /q "%~dp0modules/libusbcpp/bin"
2>NUL rmdir /s /q "%~dp0modules/libusbcpp/build"

echo Done
