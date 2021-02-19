# macOS Build Instructions and Notes

The commands in this guide should be executed in a Terminal application.
The built-in one is located in
```
    /Applications/Utilities/Terminal.app
```

## Prerequisites
In order to build BTCU on Mac it is required to have a MacOS computer, with at least 3GB of free disk space (some of the dependencies will require more space, but you may have these installed already) without the database.

## Preparation
The next step is required only if you don't have an already instaled XCode. Otherwise you may skip it.
Install the macOS command line tools:

```shell
    xcode-select --install
```

When the popup appears, click `Install`.
You can check the [detailed installation guide](https://www.ics.uci.edu/~pattis/common/handouts/macmingweclipse/allexperimental/macxcodecommandlinetools.html) for a proper explanation.

Then you will need to install the [Homebrew](https://brew.sh). If you already have it on a computer you may skip it.

To check it you can run the followed command:

```shell
    brew --version
```

If it throws an error, you don't have homebrew. In that case, install homebrew using the following command.
```shell
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

It is important:  Even if you had homebrew installed beforehand, update your version of homebrew and upgrade all the packages it installed by running the following command.
```shell
    brew update && brew upgrade
```

## Troubleshooting
If you see throught brew installation process errors like  `LibreSSL SSL_connect: SSL_ERROR_SYSCALL in connection`, try:
```shell
    networksetup -setv6off Wi-Fi
```

If you goes into error  `Error: Operation already in progress `, try:
```shell
    rm -rf /usr/local/var/homebrew/locks
```

For other issues please check [Homebrew's Troubleshooting page](https://docs.brew.sh/Troubleshooting).

In a case when you get `dmg` image or sources via browser, telegramm and etc you'll face with an error `Operation not permitted` for the application extracted from the the `dmg` or in a terminal thought the building process accordingly.

Note: The described behaviour for the sources case will be overlapped with another problem while you try to complete a command `cmake .` from the build section. The terminal will show a successfull generation but commands for the autogen.sh and configure won't show any actual result and appopriate result files won't be generated. You can run separately a command `./autogen.sh` to see the actual problem which is the same `Operation not permitted` as for the dmg image case.

To solve the problem in a sources folder please run:
```shell
    xattr -d com.apple.quarantine autogen.sh
```

To solve the problem for the image case:
```shell
    xattr -d com.apple.quarantine btcu-qt.dmg
```
in the download folder where you've placed the file. If the file has another name please change the command accordingly to the file name (i.e. if your file name for example if `btcu-qt_somesimbols.dmg`, you'll run `xattr -d com.apple.quarantine btcu-qt_somesimbols.dmg`).

## Dependencies
```shell
    brew install automake libtool miniupnpc pkg-config python qt libevent qrencode protobuf snappy zeromq openssl libjson-rpc-cpp google-benchmark googletest cmake git gmp gflags
    # libscrypt from local since we need a version with cmake support but you still can get it via brew
```

If you want to build the disk image with `make deploy` (.dmg / optional), you need RSVG:
```shell
    brew install librsvg
```

OpenSSL has it's uniq way to organize a folder structure so we have to try this command at first:
```shell
    brew link openssl --force
```

If you'll get a refuse result such as "Warning: Refusing to link macOS provided/shadowed software: openssl", you'll have to call this command instead:
```shell
    ( brew --prefix openssl && echo '/include/openssl'; ) | tr -d "[:space:]" | xargs -I '{}' ln -s {} /usr/local/include
```

#### Boost
In order to make things simplier and to support Apple M-series we will have to build Boost from the sources.
To do this please run commands:
```shell
    alias nproc="sysctl -n hw.logicalcpu"

    curl https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.gz -o boost_1_71_0.tar.gz
    tar -xf boost_1_71_0.tar.gz

    cd boost_1_71_0
    ./bootstrap.sh --prefix=/usr/local/opt/boost --with-python=python3 &&
    sudo ./b2 stage -j$(nproc) cxxflags="-std=c++11" --reconfigure threading=multi link=shared --with-regex --with-test --with-filesystem --with-date_time --with-random --with-system --with-thread --with-program_options --with-chrono --with-fiber --with-log --with-context --with-math && sudo ./b2 install --prefix=/usr/local/opt/boost
    sudo ln -s /usr/local/opt/boost/lib/*.dylib /usr/local/lib
    cd -
```

This will install Boost to the brew default packages folder. You can change it but make sure it is visible in system path for lib and for includes.

#### Rocksdb
If you build on Mac Intell you can just run:
```shell
    brew install rocksdb
```

In an M-series you will have to compile it from the sources:
```shell
    git clone https://github.com/facebook/rocksdb.git
    cd rocksdb
    cmake . -DPORTABLE=ON -DUSE_RTTI=ON -DWITH_BENCHMARK_TOOLS=OFF -DWITH_BZ2=O -DWITH_TESTS=OFF -DCMAKE_BUILD_TYPE=Release

    sudo make install
    cd -
```

#### SQLite

Usually, macOS installation already has a suitable SQLite installation.
In order to check is there an installed SQLite you may run a command:

```shell
    sqlite3 --version
```

If you haven't SQLite installed it can be solved by the Homebrew package:

```shell
    brew install sqlite
```

In that case the Homebrew package will prevail.

#### Berkeley DB

It is recommended to use Berkeley DB 18.1.32.

The Homebrew package could be installed:

```shell
    brew install berkeley-db@18
```

The project is configured with the dependency Berkeley DB v18.1.32. In order to check the version you can run:
```shell
    brew info berkeley-db 
```

If the brew installed a different version run the followed command:
```shell
    # since brew switch is depreceted it is required to use workaround
    brew uninstall berkeley-db@18
    # since brew prohibited to use Git commits urls in install command
    curl https://raw.githubusercontent.com/Homebrew/homebrew-core/f325e0637fbf513819129744dc107382de028fc5/Formula/berkeley-db.rb -o berkeley-db.rb
    brew install ./berkeley-db.rb
```

## Build BTCU

1. Clone the BTCU source code:
```shell
    git clone https://github.com/bitcoin-ultimatum/orion
    cd btcu
```

It is important: Do not use spaces and other line breaking simbols in a path to the downloaded project because it'll cause a compilation problems.

2.  (Optional) Make the Homebrew OpenSSL headers visible to the configure script  (do ```brew info openssl``` to find out why this is necessary, or if you use Homebrew with installation folders different from the default).

        export LDFLAGS="$LDFLAGS -L/usr/local/opt/openssl/lib"
        export CPPFLAGS="$CPPFLAGS -I/usr/local/opt/openssl/include/openssl"

3.  Build BTCU

    Configure and build the headless BTCU binaries as well as the GUI (if Qt is found).

    You can disable the GUI build by passing `-DENABLE_GUI=OFF` to cmake.

    The wallet support requires one or both of the dependencies ([*SQLite*](#sqlite) and [*Berkeley DB*](#berkeley-db)) from the previous section.
    To build BTCU without wallet, see [*Disable-wallet mode*](#disable-wallet-mode).

    To build the project run followed commands:
```shell
    cmake .
    make
```

4.  It is recommended to build and run the unit tests:
```shell
    make check
```

5.  You can also create a .dmg that contains the .app bundle (optional):
```shell
    make osx-dmg
```

## XCode build

You can run and build the application from the Apple XCode.
Firstly, you will have to create an XCode project file by the command: 
```shell
    cmake . -G Xcode
```

This will create an btcu.xcodeproj file in a root project folder.
Next, open it via XCode and select target you want. For example it can be `btcu-qt`.
After that you can select options `Run` (⌘R) of `Build` (⌘R).

Happy building!

## Compiling for different MacOS versions
In a case when you need a different version of the OSX platforms add the parameter to a cmake command (for example we use 10.10 version) :
```shell
    cmake . -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10
```

It may be required to have an installed appopriate SDK version. You can find an instruction how to make it [here](https://gist.github.com/robvanoostenrijk/7a1a32d2071232d9cd98).

## Disable-wallet mode
When the intention is to run only a P2P node without a wallet, BTCU may be
compiled in disable-wallet mode with:
```shell
    cmake . -DENABLE_WALLET=OFF
```

In this case there is no dependency on [*Berkeley DB*](#berkeley-db) and [*SQLite*](#sqlite).

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

## Running
BTCU is now available at `./btcud`

Before running, it's recommended that you create an RPC configuration file:
```shell
    echo -e "rpcuser=btcurpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/BTCU/btcu.conf"

    chmod 600 "/Users/${USER}/Library/Application Support/BTCU/btcu.conf"
```

The first time you run btcud, it will start downloading the blockchain. This process could take many hours, or even days on slower than average systems.

You can monitor the download process by looking at the debug.log file:
```shell
    tail -f $HOME/Library/Application\ Support/BTCU/debug.log
```

## Other commands:
```shell
    btcud -daemon      # Starts the btcu daemon.
    btcu-cli --help    # Outputs a list of command-line options.
    btcu-cli help      # Outputs a list of RPC commands when the daemon is running.
    ./bin/btcu-qt   # Start GUI
```

## Notes
* Tested on OS X (11.1 Big Sur and 10.15 Catalina) on 64-bit Intel processors only.
