param(
    [Parameter(Mandatory = $true)][string]$AssetSuffix,
    [string]$DawnPrefix = (Join-Path (Get-Location) '3rdparty/.deps/dawn-install')
)

$ErrorActionPreference = 'Stop'

if (Test-Path $DawnPrefix) {
    Remove-Item -Recurse -Force $DawnPrefix
}
New-Item -ItemType Directory -Force -Path $DawnPrefix | Out-Null

$archive = Join-Path $env:RUNNER_TEMP 'dawn-prebuilt.tar.gz'
$repo = if ($env:DAWN_RELEASE_REPO) { $env:DAWN_RELEASE_REPO } else { 'google/dawn' }

gh release download -R $repo --pattern "*$AssetSuffix" --output $archive
tar -xzf $archive -C $DawnPrefix --strip-components=1

$dawnConfig = Get-ChildItem -Path $DawnPrefix -Recurse -Filter DawnConfig.cmake | Select-Object -First 1
if ($null -eq $dawnConfig) {
    throw 'Downloaded Dawn archive does not contain DawnConfig.cmake'
}

"DAWN_PREFIX=$DawnPrefix" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
"Dawn_DIR=$($dawnConfig.Directory.FullName)" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
