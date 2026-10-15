@echo off
setlocal
cd /d "%~dp0"

cd ./built_binaries
cmake ../cgproject --DUSE_OPENMP=ON || goto :fail
cmake --build . --config Release --parallel || goto :fail

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