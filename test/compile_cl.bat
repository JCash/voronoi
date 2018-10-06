echo off

if NOT DEFINED VCINSTALLDIR (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio 15.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio 15.0\VC\vcvarsall.bat" amd64
        echo "USING VISUAL STUDIO 15"
    )
)

if NOT DEFINED VCINSTALLDIR (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
        echo "USING VISUAL STUDIO 14"
    )
)

if NOT DEFINED VCINSTALLDIR (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio 13.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio 13.0\VC\vcvarsall.bat" amd64
        echo "USING VISUAL STUDIO 13"
    )
)

if NOT DEFINED VCINSTALLDIR (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
        echo "USING VISUAL STUDIO 12"
    )
)

if NOT DEFINED VCINSTALLDIR (
    echo "No compatible visual studio found! run vcvarsall.bat first!"
)

mkdir ..\build

SET DOUBLEDEFINES="/DTEST_USE_DOUBLE /DJCV_REAL_TYPE=double /DJCV_ATAN2=atan2 /DJCV_SQRT=sqrt /DJCV_FABS=fabs /DJCV_FLOOR=floor /DJCV_CEIL=ceil"

cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS /WX /W4 /I../src /I. test.c /link /out:..\build\test.exe
cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS %DOUBLEDEFINES% /WX /W4 /I../src /I. test.c /link /out:..\build\test_double.exe

del *.obj

