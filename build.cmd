@echo off
setlocal
call "%~dp0msvc32.cmd"
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /LD src\LocalMetaLink.cpp /Fo:build\LocalMetaLink.obj /Fe:build\LocalMetaLink.dll /link /DEF:src\plugin.def /IMPLIB:build\LocalMetaLink.lib
exit /b %errorlevel%
