param(
    [string]$BuildDir = "build-v2-msvc"
)

$ErrorActionPreference = "Stop"

$cmake = "C:\Program Files\CMake\bin\cmake.exe"
$ctest = "C:\Program Files\CMake\bin\ctest.exe"

& $cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $ctest --test-dir $BuildDir -C Release --output-on-failure
exit $LASTEXITCODE
