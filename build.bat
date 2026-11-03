@echo off
rem Build ThermaltakeX1RGBPlugin.dll for OpenRGB 1.1 (Qt 6.8.3, MSVC x64).
rem
rem Inputs (environment variables, or set them in an optional build.local.bat
rem next to this file, which is not committed):
rem   QTDIR        Qt 6.8.3 msvc2022_64 root     (default C:\Qt\6.8.3\msvc2022_64)
rem   OPENRGB_SRC  OpenRGB release_1.1 sources   (default .\OpenRGB)
rem   VCVARS       path to vcvars64.bat          (default: found with vswhere)
setlocal

if exist "%~dp0build.local.bat" call "%~dp0build.local.bat"

if "%QTDIR%"=="" set "QTDIR=C:\Qt\6.8.3\msvc2022_64"
if "%OPENRGB_SRC%"=="" set "OPENRGB_SRC=%~dp0OpenRGB"

if "%VCVARS%"=="" (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
)

if not exist "%QTDIR%\bin\moc.exe" (echo Qt not found: set QTDIR to your Qt 6.8.3 msvc2022_64 directory & exit /b 1)
if not exist "%OPENRGB_SRC%\OpenRGBPluginInterface.h" (echo OpenRGB sources not found: set OPENRGB_SRC to an OpenRGB release_1.1 checkout & exit /b 1)
if not exist "%VCVARS%" (echo vcvars64.bat not found: install VS 2022 Build Tools with C++ or set VCVARS & exit /b 1)

call "%VCVARS%" >nul || (echo vcvars64.bat failed & exit /b 1)

cd /d "%~dp0"
if not exist build mkdir build

set "INCS=/I. /I"%QTDIR%\include" /I"%QTDIR%\include\QtCore" /I"%QTDIR%\include\QtGui" /I"%QTDIR%\include\QtWidgets" /I"%OPENRGB_SRC%" /I"%OPENRGB_SRC%\RGBController" /I"%OPENRGB_SRC%\dependencies\json""
set "DEFS=/DNDEBUG /DQT_NO_DEBUG /DQT_PLUGIN /DQT_SHARED /DQT_CORE_LIB /DQT_GUI_LIB /DQT_WIDGETS_LIB /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN"

echo [moc]
"%QTDIR%\bin\moc.exe" %DEFS:/D=-D% -I. -I"%QTDIR%\include" -I"%QTDIR%\include\QtCore" -I"%OPENRGB_SRC%" -I"%OPENRGB_SRC%\RGBController" -I"%OPENRGB_SRC%\dependencies\json" ThermaltakeX1RGBPlugin.h -o build\moc_ThermaltakeX1RGBPlugin.cpp || exit /b 1

echo [cl]
cl /nologo /c /std:c++17 /EHsc /O2 /MD /utf-8 /permissive- /Zc:__cplusplus /W3 %DEFS% %INCS% /Fobuild\ ThermaltakeX1RGBPlugin.cpp X1RGBDevice.cpp build\moc_ThermaltakeX1RGBPlugin.cpp || exit /b 1

echo [link]
link /nologo /DLL /OUT:build\ThermaltakeX1RGBPlugin.dll build\ThermaltakeX1RGBPlugin.obj build\X1RGBDevice.obj build\moc_ThermaltakeX1RGBPlugin.obj "%QTDIR%\lib\Qt6Core.lib" "%QTDIR%\lib\Qt6Gui.lib" "%QTDIR%\lib\Qt6Widgets.lib" hid.lib setupapi.lib || exit /b 1

echo OK: %~dp0build\ThermaltakeX1RGBPlugin.dll
endlocal
