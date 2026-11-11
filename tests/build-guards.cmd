@echo off
call "%~dp0..\msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /O2 /MD tests\ttr11-adapter-guards.cpp /Fo:build\guard.obj /Fe:build\ttr11-guards.exe
if errorlevel 1 exit /b 1
build\ttr11-guards.exe %*
exit /b %errorlevel%
