echo off

mkdir build

cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS /W4 -I.. simple.c /link /out:../../build/simple.exe || exit /b %errorlevel%

del *.obj
