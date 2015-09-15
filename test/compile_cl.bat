echo off

if NOT DEFINED VCINSTALLDIR call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"

cl.exe /O2 /I.. ../src/voronoi.cpp test.cpp

