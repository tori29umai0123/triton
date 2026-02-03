@echo off
REM LLVM Build Script for Windows (Manual Environment)
REM
REM Prerequisites: Run setup_env.bat first to set environment variables

setlocal EnableDelayedExpansion

REM ============================================
REM Verify Environment
REM ============================================
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: cl.exe not found in PATH.
    echo Please run setup_env.bat first or add MSVC to PATH.
    exit /b 1
)

where ninja.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ninja.exe not found in PATH.
    exit /b 1
)

echo Environment OK.
echo   CL:
where cl.exe
echo.

REM ============================================
REM Configuration
REM ============================================
set LLVM_BUILD_DIR=%~dp0..\llvm-build
set LLVM_INSTALL_DIR=%~dp0..\llvm-install

REM Read LLVM commit hash from Triton's cmake/llvm-hash.txt
set /p LLVM_HASH=<cmake\llvm-hash.txt
set LLVM_HASH=%LLVM_HASH:~0,40%

echo LLVM commit hash: %LLVM_HASH%

REM ============================================
REM Clone LLVM if not exists
REM ============================================
if not exist "%LLVM_BUILD_DIR%\llvm-project" (
    echo Cloning LLVM repository...
    mkdir "%LLVM_BUILD_DIR%" 2>nul
    cd /d "%LLVM_BUILD_DIR%"
    git clone --depth 1 https://github.com/llvm/llvm-project.git
    cd llvm-project
    git fetch --depth 1 origin %LLVM_HASH%
    git checkout %LLVM_HASH%
) else (
    echo LLVM repository already exists, checking out correct version...
    cd /d "%LLVM_BUILD_DIR%\llvm-project"
    git fetch --depth 1 origin %LLVM_HASH%
    git checkout %LLVM_HASH%
)

REM ============================================
REM Configure LLVM
REM ============================================
set LLVM_SRC=%LLVM_BUILD_DIR%\llvm-project\llvm
set BUILD_DIR=%LLVM_BUILD_DIR%\build

echo.
echo Configuring LLVM...
mkdir "%BUILD_DIR%" 2>nul
cd /d "%BUILD_DIR%"

cmake -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%LLVM_INSTALL_DIR%" ^
    -DLLVM_ENABLE_PROJECTS="mlir;lld" ^
    -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" ^
    -DLLVM_ENABLE_ASSERTIONS=ON ^
    -DLLVM_ENABLE_RTTI=ON ^
    -DLLVM_ENABLE_EH=ON ^
    -DLLVM_BUILD_EXAMPLES=OFF ^
    -DLLVM_INCLUDE_TESTS=ON ^
    -DLLVM_BUILD_TESTS=OFF ^
    -DLLVM_INCLUDE_BENCHMARKS=OFF ^
    -DLLVM_INSTALL_UTILS=ON ^
    -DMLIR_ENABLE_BINDINGS_PYTHON=OFF ^
    -DCMAKE_C_COMPILER=cl ^
    -DCMAKE_CXX_COMPILER=cl ^
    -DCMAKE_C_FLAGS="/bigobj /Zc:preprocessor" ^
    -DCMAKE_CXX_FLAGS="/bigobj /Zc:preprocessor /EHsc" ^
    "%LLVM_SRC%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo LLVM configuration failed!
    exit /b %ERRORLEVEL%
)

REM ============================================
REM Build LLVM
REM ============================================
echo.
echo Building LLVM...
ninja

if %ERRORLEVEL% NEQ 0 (
    echo LLVM build failed!
    exit /b %ERRORLEVEL%
)

REM ============================================
REM Install LLVM
REM ============================================
echo.
echo Installing LLVM to %LLVM_INSTALL_DIR%...
ninja install

if %ERRORLEVEL% NEQ 0 (
    echo LLVM installation failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo LLVM build completed successfully!
echo LLVM installed to: %LLVM_INSTALL_DIR%
echo.
echo Add to your environment:
echo   set LLVM_SYSPATH=%LLVM_INSTALL_DIR%
echo ============================================

endlocal
