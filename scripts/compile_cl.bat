echo off

mkdir build

cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS /c -Fobuild\stb_wrapper.obj src\stb_wrapper.c
if %errorlevel% neq 0 exit /b %errorlevel%

cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS /W4 build\stb_wrapper.obj src\main.c /link /out:build\main.exe
if %errorlevel% neq 0 exit /b %errorlevel%

del *.obj
