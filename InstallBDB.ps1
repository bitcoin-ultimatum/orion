param(
	[string]$toolset = "141",
	[string]$windowssdkversion = $(Get-Item "hklm:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0").GetValue("ProductVersion"),
	[string]$msbuildparams = "PlatformToolset=v$toolset;WindowsTargetPlatformVersion=$($windowssdkversion).0;TargetPlatformVersion=$($windowssdkversion).0;Platform=x64;PreferredToolArchitecture=x64"
)

$ErrorActionPreference = "Stop"
Write-Host "Compilation of the Berkeley DB 18.1.32 has been started." -ForegroundColor Cyan

Write-Host "Step 1. Extracting Berkeley DB 18.1.32 sources..." -ForegroundColor Cyan
tar zxvf depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -C depends/packages/static
cd ./depends/packages/static/db-18.1.32/build_windows
Write-Host "Step 1. Extracting Berkeley DB 18.1.32 sources... Done!" -ForegroundColor Cyan


Write-Host "Step 2. Compliling Berkeley DB Static Release with parametes: $msbuildparams..." -ForegroundColor Cyan
msbuild /nologo /verbosity:quiet /m:4 "/p:$msbuildparams;Configuration=Static Release" Berkeley_DB_vs2015.sln
& Write-Host "Step 2. Compliling Berkeley DB Static Release with parametes: $msbuildparams... Done!" -ForegroundColor Cyan

& Write-Host "Step 3. Compliling Berkeley DB Static Debug with parametes: $msbuildparams..." -ForegroundColor Cyan
& msbuild /nologo /verbosity:quiet /m:4 "/p:$msbuildparams;Configuration=Static Debug" Berkeley_DB_vs2015.sln
& Write-Host "Step 3. Compliling Berkeley DB Static Debug with parametes: $msbuildparams... Done!" -ForegroundColor Cyan

& Write-Host "Compilation of the Berkeley DB 18.1.32 has been started. Done!" -ForegroundColor Cyan