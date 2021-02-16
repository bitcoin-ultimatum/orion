param(
	[string]$toolset = "142",
	[string]$windowssdkversion = $(Get-Item "hklm:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0").GetValue("ProductVersion"),
	[string]$msbuildparams = "PlatformToolset=v$toolset;WindowsTargetPlatformVersion=$($windowssdkversion).0;TargetPlatformVersion=$($windowssdkversion).0;Platform=x64;PreferredToolArchitecture=x64"
)

$ErrorActionPreference = "Stop"
Write-Host "Step 1. Preparing extracting tools..." -ForegroundColor Cyan
Install-Module -Name PS7Zip -Confirm:$False -Force -AllowClobber
Import-Module PS7Zip
Write-Host "Step 1. Preparing extracting tools... Done!" -ForegroundColor Cyan
Write-Host "Step 2. Extracting Berkeley DB 18.1.32 sources..." -ForegroundColor Cyan
Expand-7Zip -FullName depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -Destination depends/packages/static
Expand-7Zip -FullName depends/packages/static/berkeley-db-18.1.32.tar -Destination depends/packages/static -Remove
cd depends/packages/static/db-18.1.32/build_windows
Write-Host "Step 2. Extracting Berkeley DB 18.1.32 sources... Done!" -ForegroundColor Cyan


Write-Host "Step 3. Compliling Berkeley DB Static Release with parametes: $msbuildparams..." -ForegroundColor Cyan
msbuild /nologo /verbosity:quiet /m:4 "/p:$msbuildparams;Configuration=Static Release" Berkeley_DB_vs2015.sln
& Write-Host "Step 3. Compliling Berkeley DB Static Release with parametes: $msbuildparams... Done!" -ForegroundColor Cyan

& Write-Host "Step 4. Compliling Berkeley DB Static Debug with parametes: $msbuildparams..." -ForegroundColor Cyan
& msbuild /nologo /verbosity:quiet /m:4 "/p:$msbuildparams;Configuration=Static Debug" Berkeley_DB_vs2015.sln
& Write-Host "Step 4. Compliling Berkeley DB Static Debug with parametes: $msbuildparams... Done!" -ForegroundColor Cyan

& cd ../../../../..