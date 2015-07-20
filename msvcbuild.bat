IF NOT "x%1" == "x" GOTO :%1
GOTO :build

:amd64
cmake -H. -Bbuild -G"Visual Studio 12 Win64"
GOTO :end

:ia32
cmake -H. -Bbuild -G"Visual Studio 12"
GOTO :end

:build
IF NOT EXIST build call msvcbuild.bat amd64
cmake --build build --config Release
copy build\Release\luv.dll .
copy build\Release\luajit.exe .

:end
