cd depends\packages\static\  
    # The folder where
    # the Boost files would be
Write-Host "Downloading boost 1.71.0 sources..." -ForegroundColor Cyan
(new-object System.Net.WebClient).DownloadFile("https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.zip","${PWD}\boost_1_71_0.zip")
Write-Host "Downloading boost 1.71.0 sources... Done!" -ForegroundColor Cyan

Write-Host "Extracting boost 1.71.0 sources..." -ForegroundColor Cyan
    # This command is only supported on PowerShell v5.1+
    # If you don't have it you can simply unpack 
    # the package manually
$FolderPath = "$PWD"

Expand-Archive -Path $FolderPath\boost_1_71_0.zip -DestinationPath "$FolderPath" -Force

Write-Host "Extracting boost 1.71.0 sourc111es..." -ForegroundColor Cyan
cd boost_1_71_0
Write-Host "Extracting boost 1.71.0 sources... Done!" -ForegroundColor Cyan

Write-Host "Bootstrapping boost..." -ForegroundColor Cyan
./bootstrap.bat              # Prepare installer
Write-Host "Bootstrapping boost... Done!" -ForegroundColor Cyan


Write-Host "Compiling boost..." -ForegroundColor Cyan
$nproc = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

./b2 --variant=debug,release address-model=64 architecture=x86 --build-type=complete -j $nproc --with-regex --with-test --with-filesystem --with-date_time --with-random --with-system --with-thread --with-program_options --with-chrono --with-fiber --with-log --with-context --with-math stage link=static runtime-link=static threading=multi define=BOOST_USE_WINAPI_VERSION=0x0A00 define=_SECURE_SCL=0 define=_CRT_SECURE_NO_DEPRECATE define=_CRT_SECURE_NO_WARNINGS asynch-exceptions=on exception-handling=on extern-c-nothrow=off define=BOOST_THREAD_PLATFORM_WIN32 define=BOOST_LOG_BUILDING_THE_LIB=1
Write-Host "Compiling boost... Done!" -ForegroundColor Cyan

cd ../../../..