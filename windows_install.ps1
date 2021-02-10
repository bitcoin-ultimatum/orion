$ErrorActionPreference = "Stop"

if(!$PSScriptRoot){ $PSScriptRoot = Split-Path $MyInvocation.MyCommand.Path -Parent } #In case if $PSScriptRoot is empty (version of powershell V.2).  

Try {

Write-Host "Starting and installation of the BTCU..." -ForegroundColor Cyan
& Write-Host "[0%] Installing MSVC 2019 (Community Edition) with required packages -- Please make sure you've read the license agreement!" -ForegroundColor Cyan
&"$PSScriptRoot\InstallVS2019Community.ps1"
& Write-Host "[24%] Installing MSVC 2019 (Community Edition) with required packages... Done!" -ForegroundColor Cyan
& Write-Host "[25%] Installing Git..." -ForegroundColor Cyan
&"$PSScriptRoot\InstallGit.ps1"
& Write-Host "[29%] Installing Git... Done!" -ForegroundColor Cyan
& Write-Host "[30%] Installing VCPKG..." -ForegroundColor Cyan
&"$PSScriptRoot\InstallVCPKG.ps1"
& Write-Host "[34%] Installing VCPKG... Done!" -ForegroundColor Cyan
& Write-Host "[35%] Installing VCPKG dependencies..." -ForegroundColor Cyan
&"$PSScriptRoot\InstallVCKPGDependencies.ps1"
& Write-Host "[49%] Installing VCPKG dependencies... Done!" -ForegroundColor Cyan
& Write-Host "[50%] Cloning BTCU repository..." -ForegroundColor Cyan
& git clone https://github.com/bitcoin-ultimatum/orion 
& cd btcu
& Write-Host "[54%] Cloning BTCU repository... Done!" -ForegroundColor Cyan
& Write-Host "[55%] Installing Boost 1.71.0..." -ForegroundColor Cyan
&"$PSScriptRoot\BuildBoost.ps1"
& Write-Host "[64%] Installing Boost 1.71.0... Done!" -ForegroundColor Cyan
& Write-Host "[65%] Installing Berkeley DB 18.1.32..." -ForegroundColor Cyan
&"$PSScriptRoot\InstallBDB.ps1"
& Write-Host "[74%] Installing Berkeley DB 18.1.32... Done!" -ForegroundColor Cyan
& Write-Host "[75%] Starting CMake Configuration..." -ForegroundColor Cyan
&"$PSScriptRoot\RunCMake.ps1"
& Write-Host "[84%] Starting CMake Configuration... Done!" -ForegroundColor Cyan
& Write-Host "[85%] Building BTCU..." -ForegroundColor Cyan
&"$PSScriptRoot\BuildProject.ps1"
& Write-Host "[100%] Building BTCU... Done!" -ForegroundColor Cyan
# TODO: add cmake call and build call

} ### End Try block
Catch  {
      Write-Host "An error occurred:"
      Write-Host $_
   }