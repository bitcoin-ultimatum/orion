$ErrorActionPreference = "Stop"
Try {
    Write-Host "Step 1. Setting downloading Git installer parameters..." -ForegroundColor Cyan
    # get latest download url for git-for-windows 64-bit exe
    $git_url = "https://api.github.com/repos/git-for-windows/git/releases/latest"
    $asset = Invoke-RestMethod -Method Get -Uri $git_url | % assets | where name -like "*64-bit.exe"
    Write-Host "Step 1. Setting downloading Git installer parameters... Done!" -ForegroundColor Cyan
    # download installer
    $installer = "$env:temp\$($asset.name)"
    Write-Host "Step 2. Downloading Git installer..." -ForegroundColor Cyan
    & Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $installer
    & Write-Host "Step 2. Downloading Git installer... Done!" -ForegroundColor Cyan
    # run installer
    & Write-Host "Step 3. Installing Git..." -ForegroundColor Cyan
    & Start-Process -FilePath $installer -ArgumentList "/SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /CLOSEAPPLICATIONS /RESTARTAPPLICATIONS /LOADINF=""git.install.inf""" -Wait
    & Write-Host "Step 3. Installing Git... Done!" -ForegroundColor Cyan
    & Write-Host "Step 4. Configuring PATH variable..." -ForegroundColor Cyan
    & {$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")}
    & Write-Host "Step 4. Configuring PATH variable... Done!" -ForegroundColor Cyan
} ### End Try block
Catch  {
      Write-Host "An error occurred:"
      Write-Host $_
   }