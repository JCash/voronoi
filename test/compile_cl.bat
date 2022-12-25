echo off

mkdir ..\build

SET DOUBLEDEFINES="/DTEST_USE_DOUBLE /DJCV_REAL_TYPE=double /DJCV_ATAN2=atan2 /DJCV_SQRT=sqrt /DJCV_REAL_TYPE_EPSILON=DBL_EPSILON"

cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS /W4 /I../src /I. /DJC_TEST_USE_COLORS test.cpp /link /out:..\build\test.exe || exit /b %errorlevel%
cl.exe /nologo /O2 /D_CRT_SECURE_NO_WARNINGS %DOUBLEDEFINES% /W4 /I../src /I. /DJC_TEST_USE_COLORS test.cpp /link /out:..\build\test_double.exe || exit /b %errorlevel%

del *.obj
