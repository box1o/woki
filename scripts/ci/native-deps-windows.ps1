$ErrorActionPreference = 'Stop'

if ($env:VCPKG_INSTALLATION_ROOT) {
    $vcpkgRoot = $env:VCPKG_INSTALLATION_ROOT
} elseif (Test-Path 'C:\vcpkg\vcpkg.exe') {
    $vcpkgRoot = 'C:\vcpkg'
} else {
  throw 'vcpkg was not found on this runner (expected C:\vcpkg or VCPKG_INSTALLATION_ROOT)'
}

& (Join-Path $vcpkgRoot 'vcpkg.exe') install `
  freetype:x64-windows `
  libarchive:x64-windows

"VCPKG_ROOT=$vcpkgRoot" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8

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
