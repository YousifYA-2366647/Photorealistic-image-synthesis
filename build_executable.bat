@echo off
setlocal
cd /d "%~dp0"

cd ./built_binaries
cmake ../cgproject -G "Visual Studio 18 2026" || goto :fail
cmake --build . || goto :fail

echo.
echo.
echo.
echo Build succeeded.
pause
exit /b 0

:fail
echo.
echo.
echo.
echo Build FAILED.
pause
exit /b 1