set ARCH= Win64"
if "%platform%" EQU "x86" (set ARCH=")

set VS_VER="12"
if "%configuration%"=="2015" ( set VS_VER="14" )
if "%configuration%"=="2013" ( set VS_VER="12" )

cmake -H. -Bbuild -G"Visual Studio %VS_VER%%ARCH%
cmake --build build --config Release
copy build\Release\luv.dll .
copy build\Release\luajit.exe .
