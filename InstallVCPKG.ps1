if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))  
{  
  $arguments = "& '" +$myinvocation.mycommand.definition + "'"
  Start-Process powershell -Verb runAs -ArgumentList $arguments
  Break
}

$ErrorActionPreference = "Stop"
$env:GIT_REDIRECT_STDERR = '2>&1'
Try {
    Write-Host "Step 1. Cloning vcpkg..." -ForegroundColor Cyan
    git clone https://github.com/Microsoft/vcpkg.git;
    cd vcpkg
    & Write-Host "Step 2. Bootstrapping vcpkg..." -ForegroundColor Cyan
    & Start-Process -Verb RunAs "cmd.exe"  "/c ${PWD}/bootstrap-vcpkg.bat" -Wait
    & Write-Host "Step 3. Integration vcpkg with MSVC..." -ForegroundColor Cyan
    & Start-Process -Verb RunAs "${PWD}/vcpkg.exe"  "integrate instal" -Wait

    Write-Host "Step 4. Adding env variabled..." -ForegroundColor Cyan
    setx VCPKG_ROOT "${PWD}" /M  # Set env variable VCPKG_ROOT
    setx VCPKG_DEFAULT_TRIPLET "x64-windows-static" /M
            # The default supported build configuration is
            # x64 build with static dependencies


    Write-Host "Adding vcpkg to PATH..." -ForegroundColor Cyan
    setx PATH "$env:path;${PWD}" -m
    Write-Host "Installing vcpkg... Done!" -ForegroundColor Cyan
} ### End Try block
Catch  {
      Write-Host "An error occurred:"
      Write-Host $_
   }