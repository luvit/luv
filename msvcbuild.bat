@echo off

if not defined VS set VS=15
if "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2019" (set VS=16)
if "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2017" (set VS=15)
if "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2015" (set VS=14)
if "%APPVEYOR_BUILD_WORKER_IMAGE%"=="Visual Studio 2013" (set VS=12)

if not defined platform set platform=x64
if "%platform%" EQU "x64" (set VS=%VS% Win64)

cmake -H. -Bbuild -G"Visual Studio %VS%"
cmake --build build --config Release
copy build\Release\luv.dll .
copy build\Release\luajit.exe .
