@echo off
setlocal
cd /d "%~dp0"

cd ./built_binaries
cmake ../cgproject || goto :fail
cmake --build . --DUSE-OPENMP=ON || goto :fail

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