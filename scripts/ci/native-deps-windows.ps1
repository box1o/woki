$ErrorActionPreference = 'Stop'

if ($env:VCPKG_INSTALLATION_ROOT) {
    $vcpkgRoot = $env:VCPKG_INSTALLATION_ROOT
} elseif (Test-Path 'C:\vcpkg\vcpkg.exe') {
    $vcpkgRoot = 'C:\vcpkg'
} else {
    throw 'vcpkg was not found on this runner (expected C:\vcpkg or VCPKG_INSTALLATION_ROOT)'
}

$env:VCPKG_BUILD_TYPE = 'release'

$bincache = Join-Path $env:GITHUB_WORKSPACE '.vcpkg\bincache'
New-Item -ItemType Directory -Force -Path $bincache | Out-Null
$env:VCPKG_BINARY_SOURCES = "clear;files,$bincache,readwrite"

& (Join-Path $vcpkgRoot 'vcpkg.exe') install libarchive:x64-windows

"VCPKG_ROOT=$vcpkgRoot" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
"WOKI_VCPKG_ROOT=$vcpkgRoot" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
"VCPKG_DEFAULT_BINARY_CACHE=$bincache" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8

choco install llvm -y --no-progress

$llvmBin = 'C:\Program Files\LLVM\bin'
if (-not (Test-Path (Join-Path $llvmBin 'clang++.exe'))) {
    throw "LLVM clang++ was not installed at $llvmBin"
}

"PATH=$llvmBin;$env:PATH" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
"WOKI_WASM_CLANG=$llvmBin\clang.exe" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8

& (Join-Path $llvmBin 'clang++.exe') --version

$wasmProbe = Join-Path $env:RUNNER_TEMP 'wasm-probe.cpp'
Set-Content -Path $wasmProbe -Value 'int main(){}'
& (Join-Path $llvmBin 'clang++.exe') --target=wasm32-unknown-unknown -x c++ -c $wasmProbe -o (Join-Path $env:RUNNER_TEMP 'wasm-probe.o')
Remove-Item -Force $wasmProbe, (Join-Path $env:RUNNER_TEMP 'wasm-probe.o')
