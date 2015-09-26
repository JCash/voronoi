echo off

if NOT DEFINED VCINSTALLDIR (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
    ) else (
        echo "No cmpatible visual studio found! run vcvarsall.bat first!"
    )
)

cl.exe /O2 /I. src/main.c

del main.obj

