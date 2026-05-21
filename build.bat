@ECHO OFF
SETLOCAL

DIR "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" > NUL 2>&1
IF ERRORLEVEL 1 (
    ECHO Cannot find Visual Studio Install Path
)
FOR /F "usebackq tokens=*" %%I IN (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) DO SET VS_INSTALL_DIR=%%I
DIR "%VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat" > NUL 2>&1
IF ERRORLEVEL 1 (
    ECHO Cannot find Visual Studio Command Prompt
)
REM ECHO Found Visual Studio at %VS_INSTALL_DIR%

CALL "%VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat" > NUL
IF "%1"=="clean" CALL :CLEAN
IF "%1"=="test" CALL :TEST
IF "%1"=="" CALL :BUILD
GOTO :EOF

:BUILD
ECHO ON
cl.exe /std:c++20 /EHsc /Fe:calc.exe main.cpp
@ECHO OFF
EXIT /B 0

:CLEAN
IF EXIST calc.exe DEL /Q calc.exe
IF EXIST main.obj DEL /Q main.obj
EXIT /B 0

:TEST
CALL :BUILD
.\calc.exe
EXIT /B 0