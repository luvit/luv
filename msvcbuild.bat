cmake -H. -Bbuild
cmake --build build --config Release
copy build\Release\luv.dll .
copy build\Release\luajit.exe .
