cmake_minimum_required(VERSION 3.14)

project(qbinarizertest LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(sources
    binarizertest.h
    binarizertest.cpp
    main.cpp
)

add_executable(qbinarizertest)
target_sources(qbinarizertest PRIVATE ${sources})

add_test(NAME encode_test COMMAND qbinarizertest)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Xml)
find_package(GTest REQUIRED)

target_link_libraries(qbinarizertest Qt${QT_VERSION_MAJOR}::Core qbinarizer::qbinarizer GTest::GTest)
