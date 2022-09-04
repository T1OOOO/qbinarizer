cmake_minimum_required(VERSION 3.14)

project(qbinarizerexample LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(sources
    main.cpp
)

add_executable(qbinarizerexample)
target_sources(qbinarizerexample PRIVATE ${sources})

target_link_libraries(qbinarizerexample Qt${QT_VERSION_MAJOR}::Core qbinarizer::qbinarizer)
