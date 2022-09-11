cmake_minimum_required(VERSION 3.14)

project(QBINARIZER VERSION 0.5.2
                   DESCRIPTION "Library for encode and decode any binary data"
                   LANGUAGES CXX C)

#Version
add_definitions(-DQBINARIZER_VERSION_STR="${QBINARIZER_VERSION_MAJOR}.${QBINARIZER_VERSION_MINOR}.${QBINARIZER_VERSION_PATCH}"
                -DQBINARIZER_VERSION_MAJOR=${QBINARIZER_VERSION_MAJOR}
                -DQBINARIZER_VERSION_MINOR=${QBINARIZER_VERSION_MINOR}
                -DQBINARIZER_VERSION_PATCH=${QBINARIZER_VERSION_PATCH}
                )

get_directory_property(QBINARIZER_HAS_PARENT PARENT_DIRECTORY)
if(QBINARIZER_HAS_PARENT)
    set(QBINARIZER_MASTER_PROJECT OFF)
else()
    set(QBINARIZER_MASTER_PROJECT ON)
endif(QBINARIZER_HAS_PARENT)

#Options
option(QBINARIZER_BUILD_TEST "Build QBinarizer tests" ${QBINARIZER_MASTER_PROJECT})
option(QBINARIZER_BUILD_EXAMPLE "Build QBinarizer examples" ${QBINARIZER_MASTER_PROJECT})
option(QBINARIZER_BUILD_DOCS "Build QBinarizer documentation" OFF)
option(QBINARIZER_INSTALL_PACKAGING "Generate target for installing QBinarizer" ${QBINARIZER_MASTER_PROJECT})
option(QBINARIZER_BUILD_SHARED "Build as shared library" OFF)

include(GNUInstallDirs)

#Module
set(QBINARIZER_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH "${QBINARIZER_MODULE_DIR}")

if (NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET AND
    NOT DEFINED CMAKE_VISIBILITY_INLINES_HIDDEN)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
endif ()

if(DEFINED QASTERIX_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ${QBINARIZER_SHARED_LIBS})
endif()

set(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP ON)
set(QT_CREATOR_SKIP_CONAN_SETUP ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

#Qt
find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Xml)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(header_path ${QBINARIZER_SOURCE_DIR}/include/qbinarizer)
set(dependency_path ${QBINARIZER_SOURCE_DIR}/3rdparty)
set(bitfield_path ${dependency_path}/bitfield-c/src)
set(crc_path ${dependency_path}/libcrc)
set(source_path ${QBINARIZER_SOURCE_DIR}/src)

include_directories(${header_path})
include_directories(${bitfield_path})
include_directories(${crc_path}/include)

set(bitfield_sources
    ${bitfield_path}/bitfield/bitfield.h
    ${bitfield_path}/bitfield/8byte.h
    ${bitfield_path}/bitfield/bitfield.c
    ${bitfield_path}/bitfield/bitarray.c
    ${bitfield_path}/bitfield/8byte.c
)

set(crc_sources
    ${crc_path}/include/checksum.h
    ${crc_path}/tab/gentab32.inc
    ${crc_path}/tab/gentab64.inc
    ${crc_path}/src/crc16.c
    ${crc_path}/src/crc32.c
    ${crc_path}/src/crc64.c
    ${crc_path}/src/crc8.c
    ${crc_path}/src/crcccitt.c
    ${crc_path}/src/crcdnp.c
    ${crc_path}/src/crckrmit.c
    ${crc_path}/src/crcsick.c
    ${crc_path}/src/nmea-chk.c
)

set(public_headers
    ${header_path}/StructEncoder
    ${header_path}/StructDecoder
)

set(private_headers
    ${header_path}/internal/structencoder.h
    ${header_path}/internal/structdecoder.h
)

set(binarizer_sources
    src/jsonutils.h
    src/jsonutils.cpp
    src/bitutils.h
    src/bitutils.cpp
    src/structencoder.cpp
    src/structdecoder.cpp
    src/structreflector.h
    src/structreflector.cpp
)

add_library(qbinarizer)
add_library(qbinarizer::qbinarizer ALIAS qbinarizer)

include(GenerateExportHeader)
generate_export_header(qbinarizer EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/qbinarizer/export/qbinarizer_export.h)
target_compile_definitions(qbinarizer PUBLIC "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:QBINARIZER_STATIC_DEFINE>")

list(APPEND public_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/qbinarizer/export/qbinarizer_export.h")

set(sources
    ${public_headers}
    ${private_headers}
    ${bitfield_sources}
    ${crc_sources}
    ${binarizer_sources}
)

source_group(TREE ${bitfield_path} FILES ${bitfield_sources})
source_group(TREE ${crc_path} FILES ${crc_sources})
source_group(TREE ${QBINARIZER_SOURCE_DIR} FILES ${binirizer_sources})

target_include_directories(qbinarizer
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_sources(qbinarizer PRIVATE ${sources})

target_link_libraries(qbinarizer Qt${QT_VERSION_MAJOR}::Core)

if (DEFINED QBINARIZER_BUILD_TEST)
    add_subdirectory(tests)
endif(QBINARIZER_BUILD_TEST)

if (DEFINED QBINARIZER_BUILD_EXAMPLE)
    add_subdirectory(examples)
endif(QBINARIZER_BUILD_EXAMPLE)

if (DEFINED QBINARIZER_INSTALL_PACKAGING)
    install(TARGETS qbinarizer
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(QBINARIZER_INSTALL_PACKAGING)
