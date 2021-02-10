$ErrorActionPreference = "Stop"
Try {
    Write-Host "Step 1. Cloning vcpkg..." -ForegroundColor Cyan
    Try {
        git clone https://github.com/Microsoft/vcpkg.git
    } Catch  {}
    cd vcpkg
    Write-Host "Step 2. Bootstrapping vcpkg..." -ForegroundColor Cyan
    PowerShell.exe -ExecutionPolicy Bypass -File ".\scripts\bootstrap.ps1" 2>&1>$null
    cmd ./bootstrap-vcpkg.bat        # Installation script
    Write-Host "Step 3. Integration vcpkg with MSVC..." -ForegroundColor Cyan
    ./vcpkg integrate install    # Integration with MSVC

    Write-Host "Step 4. Adding env variabled..." -ForegroundColor Cyan
    setx VCPKG_ROOT "${PWD}" /M  # Set env variable VCPKG_ROOT
    setx VCPKG_DEFAULT_TRIPLET "x64-windows-static" /M
            # The default supported build configuration is
            # x64 build with static dependencies
    refreshenv                   # Update environment variables


    Write-Host "Adding vcpkg to PATH..." -ForegroundColor Cyan
    Add-Path "${PWD}"
    Write-Host "Installing vcpkg... Done!" -ForegroundColor Cyan
} ### End Try block
Catch  {
      Write-Host "An error occurred:"
      Write-Host $_
   }