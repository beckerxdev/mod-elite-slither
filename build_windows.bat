@echo off
REM MOD ELITE 1.0 - build para Windows 64 bits. Requer: CMake 3.20+, Git e (Visual Studio 2022 com C++ OU MinGW-w64).
setlocal
where cmake >nul 2>nul || (echo CMake nao encontrado. Instale em https://cmake.org/download/ & exit /b 1)
where git >nul 2>nul || (echo Git nao encontrado. Instale em https://git-scm.com/ & exit /b 1)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" (
  cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || exit /b 1
  cmake --build build --config Release || exit /b 1
  set EXE=build\Release\MOD_ELITE_1.0.exe
) else (
  cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release || exit /b 1
  cmake --build build || exit /b 1
  set EXE=build\MOD_ELITE_1.0.exe
)
echo.
echo Executavel gerado: %EXE%
echo Para rodar os testes do nucleo: ctest --test-dir build -C Release --output-on-failure
endlocal
