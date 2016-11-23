[CmdletBinding()]
param(
    [ValidateNotNullOrEmpty()]
    [string]$disableMetrics = "0"
)

$scriptsDir = split-path -parent $MyInvocation.MyCommand.Definition
$vcpkgRootDir = & $scriptsDir\findFileRecursivelyUp.ps1 $scriptsDir .vcpkg-root

$gitHash = "unknownhash"
if (Get-Command "git.exe" -ErrorAction SilentlyContinue)
{
    $gitHash = git rev-parse HEAD
}
Write-Verbose("Git hash is " + $gitHash)
$vcpkgSourcesPath = "$vcpkgRootDir\toolsrc"
Write-Verbose("vcpkg Path " + $vcpkgSourcesPath)

if (!(Test-Path $vcpkgSourcesPath))
{
    New-Item -ItemType directory -Path $vcpkgSourcesPath -force | Out-Null
}

try{
    pushd $vcpkgSourcesPath
    cmd /c "$env:VS140COMNTOOLS..\..\VC\vcvarsall.bat" x86 "&" msbuild "/p:VCPKG_VERSION=-$gitHash" "/p:DISABLE_METRICS=$disableMetrics" /p:Configuration=Release /p:Platform=x86 /m

    Write-Verbose("Placing vcpkg.exe in the correct location")

    Copy-Item $vcpkgSourcesPath\Release\vcpkg.exe $vcpkgRootDir\vcpkg.exe | Out-Null
    Copy-Item $vcpkgSourcesPath\Release\vcpkgmetricsuploader.exe $vcpkgRootDir\scripts\vcpkgmetricsuploader.exe | Out-Null
}
finally{
    popd
}
