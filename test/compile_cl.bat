echo off

mkdir ..\build

SET DOUBLEDEFINES="/DTEST_USE_DOUBLE /DJCV_REAL_TYPE=double /DJCV_ATAN2=atan2 /DJCV_SQRT=sqrt /DJCV_FABS=fabs /DJCV_FLOOR=floor /DJCV_CEIL=ceil"

cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS /WX /W4 /I../src /I. test.cpp /link /out:..\build\test.exe || exit /b %errorlevel%
cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS %DOUBLEDEFINES% /WX /W4 /I../src /I. test.cpp /link /out:..\build\test_double.exe || exit /b %errorlevel%

del *.obj
