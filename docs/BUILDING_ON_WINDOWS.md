# Building Triton on Windows

This guide describes how to build Triton from source on Windows using MSVC.

## Prerequisites

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with C++ build tools
- **CUDA Toolkit 12.8** (or compatible version)
- **Python 3.10**
- **Git**
- **uv** (recommended) or pip

## Step 1: Clone the Repository

```cmd
git clone https://github.com/triton-lang/triton.git
cd triton
```

## Step 2: Set Up Python Environment

```cmd
uv venv --python 3.10
.venv\Scripts\activate
uv pip install torch==2.10.0 --index-url https://download.pytorch.org/whl/cu128
uv pip install packaging wheel setuptools ninja pip build psutil pybind11
```

## Step 3: Configure Environment Variables

Create a batch file or run these commands in your terminal:

```cmd
set INCLUDE=
set LIB=
set LIBPATH=

set CUDA_VERSION=12.8
set CUDA_BASE=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA
set CUDA_PATH=%CUDA_BASE%\v%CUDA_VERSION%
set CUDA_HOME=%CUDA_PATH%

set MSVC_DIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.43.34808
set WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10\
set WindowsSDKVersion=10.0.22621.0

set PATH=%MSVC_DIR%\bin\Hostx64\x64;%PATH%
set PATH=%WindowsSdkDir%bin\%WindowsSDKVersion%\x64;%PATH%
set PATH=%CUDA_PATH%\bin;%PATH%

set INCLUDE=%MSVC_DIR%\include
set INCLUDE=%INCLUDE%;%WindowsSdkDir%Include\%WindowsSDKVersion%\ucrt
set INCLUDE=%INCLUDE%;%WindowsSdkDir%Include\%WindowsSDKVersion%\shared
set INCLUDE=%INCLUDE%;%WindowsSdkDir%Include\%WindowsSDKVersion%\um
set INCLUDE=%INCLUDE%;%WindowsSdkDir%Include\%WindowsSDKVersion%\winrt

set LIB=%MSVC_DIR%\lib\x64
set LIB=%LIB%;%WindowsSdkDir%Lib\%WindowsSDKVersion%\ucrt\x64
set LIB=%LIB%;%WindowsSdkDir%Lib\%WindowsSDKVersion%\um\x64

set DISTUTILS_USE_SDK=1
set TORCH_CUDA_ARCH_LIST=7.5;8.0;8.6;8.9;9.0
set TORCHINDUCTOR_CPP_DISABLE=1
set TORCHINDUCTOR_DISABLE=1
set TORCH_COMPILE_DISABLE=1
set TORCH_COMPILE_DEBUG=0
set CUDA_DEVICE_MAX_CONNECTIONS=1
set MAX_JOBS=1

set TRITON_CUPTI_INCLUDE_PATH=%CUDA_PATH%\extras\CUPTI\include
set TRITON_CUDA_INCLUDE_PATH=%CUDA_PATH%\include
for %f in ("%CUDA_PATH%\extras\CUPTI\lib64\cupti64_*.dll") do set TRITON_CUPTI_LIB_PATH=%f
```

> **Note**: Adjust `MSVC_DIR` and `WindowsSDKVersion` to match your installed versions. You can find the MSVC version in `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\`.

## Step 4: Build LLVM

Triton requires a custom LLVM build. Run the provided build script:

```cmd
build_llvm_windows.bat
```

This will build LLVM and install it to `E:\llvm-install` by default. The build may take 1-2 hours depending on your hardware.

Alternatively, you can build LLVM manually:

```cmd
pip download triton==3.3.0 --no-deps -d .
tar -xzf triton-3.3.0.tar.gz
cd triton-3.3.0

cmake -G Ninja -B build-llvm ^
  -S cmake/llvm-project/llvm ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLLVM_ENABLE_PROJECTS="mlir;llvm" ^
  -DLLVM_TARGETS_TO_BUILD="host;NVPTX;AMDGPU" ^
  -DLLVM_ENABLE_ASSERTIONS=ON ^
  -DMLIR_ENABLE_CUDA_RUNNER=OFF ^
  -DCMAKE_INSTALL_PREFIX=E:\llvm-install

cmake --build build-llvm --target install
```

## Step 5: Build Triton

Set the LLVM path and build:

```cmd
set LLVM_SYSPATH=E:\llvm-install
set MAX_JOBS=1

python setup.py bdist_wheel
```

The wheel file will be generated in the `dist/` directory.

## Step 6: Install the Wheel

```cmd
pip install dist\triton-*.whl
```

## Troubleshooting

### CUPTI DLL Not Found

If you get errors related to CUPTI, ensure the CUPTI DLL path is correctly set:

```cmd
dir "%CUDA_PATH%\extras\CUPTI\lib64\cupti64_*.dll"
for %f in ("%CUDA_PATH%\extras\CUPTI\lib64\cupti64_*.dll") do set TRITON_CUPTI_LIB_PATH=%f
```

### MSVC Version Mismatch

If you have a different MSVC version, find the correct path:

```cmd
dir "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\"
```

Update `MSVC_DIR` accordingly.

### Out of Memory During Build

If the build runs out of memory, ensure `MAX_JOBS=1` is set to limit parallel compilation.

### Missing CUDA Headers

Ensure `CUDA_PATH` is correctly set and the CUDA Toolkit is properly installed:

```cmd
echo %CUDA_PATH%
dir "%CUDA_PATH%\include\cuda.h"
```

## Known Limitations

- **HIP/ROCm is not supported on Windows**: AMD GPU support via HIP is only available on Linux.
- **Proton profiler limitations**: Some profiling features may have limited functionality on Windows.

## Platform-Specific Notes

The Windows build includes the following adaptations:

- Uses `LoadLibrary`/`GetProcAddress` instead of `dlopen`/`dlsym`
- Uses `_aligned_malloc`/`_aligned_free` instead of `aligned_alloc`/`free`
- Uses `__declspec(dllexport)` instead of `__attribute__((visibility("default")))`
- Windows-specific library names (e.g., `nvcuda.dll` instead of `libcuda.so.1`)
