$ErrorActionPreference = "Stop"

Write-Host "Installing vcpkg dependencies..." -ForegroundColor Cyan
cd vcpkg
Write-Host "Step 1. Downloading list of x86-windows dependencies..." -ForegroundColor Cyan
(new-object System.Net.WebClient).DownloadFile("https://raw.githubusercontent.com/bitcoin-ultimatum/orion/master/depends/vcpkg/vcpkg-packages-prerequisites-86.txt","${PWD}\vcpkg-packages-prerequisites-86.txt")

Write-Host "Step 2. Installing x86-windows dependencies..." -ForegroundColor Cyan
./vcpkg install --triplet x86-windows $(Get-Content -Path vcpkg-packages-prerequisites-86.txt).split()

Write-Host "Step 3. Downloading list of x64-windows-static dependencies..." -ForegroundColor Cyan
(new-object System.Net.WebClient).DownloadFile("https://raw.githubusercontent.com/bitcoin-ultimatum/orion/master/depends/vcpkg/vcpkg-packages.txt","${PWD}\vcpkg-packages.txt")

Write-Host "Step 4. Installing x64-windows-static dependencies..." -ForegroundColor Cyan
./vcpkg install --triplet x64-windows-static $(Get-Content -Path vcpkg-packages.txt).split()

Write-Host "Installing vcpkg dependencies... Done!" -ForegroundColor Cyan
cd ..