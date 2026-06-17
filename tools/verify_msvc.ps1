param(
    [string]$BuildDir = "build-v2-msvc",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$cmake = "C:\Program Files\CMake\bin\cmake.exe"
$ctest = "C:\Program Files\CMake\bin\ctest.exe"

Write-Host "Configuring MSVC verification matrix in '$BuildDir'..."
& $cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Building $Config targets, including strict syntax, /MD, /MT, and no-CRT fixtures..."
& $cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Running $Config CTest suite..."
& $ctest --test-dir $BuildDir -C $Config --output-on-failure
exit $LASTEXITCODE
