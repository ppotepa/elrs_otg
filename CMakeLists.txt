cmake_minimum_required(VERSION 3.15)
project(elrs_otg_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    main.cpp
    src/usb_bridge.cpp
    src/serial_bridge.cpp
    src/crsf_protocol.cpp
    src/msp_commands.cpp
    src/telemetry_handler.cpp
    src/elrs_transmitter.cpp
    src/driver_installer.cpp
)

# Check for libusb
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0)
endif()

# Windows specific libusb detection
if(WIN32)
    # Look for libusb in common installation paths
    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/libusb-1.0"
            "${CMAKE_CURRENT_SOURCE_DIR}/lib"
            "C:/Program Files/libusb-1.0/include/libusb-1.0"
            "C:/Program Files (x86)/libusb-1.0/include/libusb-1.0"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/include/libusb-1.0"
    )
    
    find_library(LIBUSB_LIBRARY
        NAMES usb-1.0 libusb-1.0
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/lib"
            "C:/Program Files/libusb-1.0/lib"
            "C:/Program Files (x86)/libusb-1.0/lib"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/lib"
    )
    
    # Check if we have header but no library (runtime loading mode)
    if(LIBUSB_INCLUDE_DIR AND NOT LIBUSB_LIBRARY)
        message(STATUS "LibUSB header found but no library - enabling runtime loading")
        set(LIBUSB_FOUND TRUE)
        set(LIBUSB_LIBRARIES "")  # Will load at runtime
        set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
        set(LIBUSB_RUNTIME_MODE TRUE)
    elseif(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARY)
        message(STATUS "LibUSB library found - using static linking")
        set(LIBUSB_FOUND TRUE)
        set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
        set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
        set(LIBUSB_STUB_MODE FALSE)
    endif()
endif()

if(LIBUSB_FOUND)
    add_definitions(-DHAVE_LIBUSB)
    if(LIBUSB_RUNTIME_MODE)
        message(STATUS "LibUSB runtime loading enabled")
        message(STATUS "Application will attempt to load USB drivers at runtime")
        message(STATUS "LibUSB include dir: ${LIBUSB_INCLUDE_DIRS}")
        add_definitions(-DLIBUSB_RUNTIME_LOADING)
    else()
        message(STATUS "LibUSB found - static linking enabled")  
        message(STATUS "LibUSB include dir: ${LIBUSB_INCLUDE_DIRS}")
        message(STATUS "LibUSB library: ${LIBUSB_LIBRARIES}")
    endif()
else()
    message(STATUS "LibUSB not found - using runtime detection only")
    message(STATUS "Application will attempt to load USB drivers dynamically")
    message(STATUS "Supported runtime drivers:")
    message(STATUS "  - libusb-1.0.dll (place in same directory as executable)")
    message(STATUS "  - WinUSB driver (Windows built-in)")
    add_definitions(-DLIBUSB_RUNTIME_ONLY)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
if(LIBUSB_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})
    if(LIBUSB_LIBRARIES)
        target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})
    endif()
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build
)

# Copy platform files to build directory
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/platform
        COMMENT "Copying platform files to build directory"
    )
endif()
