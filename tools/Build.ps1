[CmdletBinding()]
param([string]$Reference = 'D:\Development\high-contrast\build')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vs = & 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmake = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$lib = Join-Path $Reference 'CommonLibSSE-NG'
$vcpkg = Join-Path $Reference 'vcpkg'
$build = Join-Path $root 'build'
& $cmake -S $root -B $build -G 'Visual Studio 18 2026' -A x64 "-DCOMMONLIB_SOURCE=$lib" "-DCMAKE_TOOLCHAIN_FILE=$vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DVCPKG_MANIFEST_MODE=OFF "-DVCPKG_INSTALLED_DIR=$vcpkg/installed"
if ($LASTEXITCODE) { throw 'configure failed' }
& $cmake --build $build --config Release --parallel 4
if ($LASTEXITCODE) { throw 'build failed' }
& $cmake --install $build --config Release --prefix (Join-Path $root 'dist/LessClutter')
