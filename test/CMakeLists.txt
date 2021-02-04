#calculate values
if(ENABLE_WALLET)
    set( ENABLE_WALLET_VALUE true )
else()
    set( ENABLE_WALLET_VALUE false )
endif()
if(BUILD_UTILS)
    set( BUILD_UTILS_VALUE true )
else()
    set( BUILD_UTILS_VALUE false )
endif()
if(ENABLE_ZMQ)
    set( ENABLE_ZMQ_VALUE true )
else()
    set( ENABLE_ZMQ_VALUE false )
endif()

# Create build ini file
configure_file(config.ini.cmake.in config.ini)