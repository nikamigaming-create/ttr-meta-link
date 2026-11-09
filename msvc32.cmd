@echo off
if defined SC_VCVARS32 goto configure
set "SC_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%SC_VSWHERE%" goto missing
for /f "usebackq delims=" %%I in (`"%SC_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SC_VCVARS32=%%I\VC\Auxiliary\Build\vcvars32.bat"
:configure
if not exist "%SC_VCVARS32%" goto missing
call "%SC_VCVARS32%" >nul
exit /b %errorlevel%
:missing
echo C++ Build Tools not found. Install the C++ workload or set SC_VCVARS32.
exit /b 1
