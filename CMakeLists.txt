cmake_minimum_required(VERSION 3.16)

project(cow_fs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

if(WIN32)
    set(PROJECT_WARNING_FLAGS
        /W4
        /WX
        /EHsc
        /permissive-
    )
else()
    set(PROJECT_WARNING_FLAGS
        -Wall
        -Werror
        -pedantic
    )
endif()

set(PROJECT_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/lib"
    "${PROJECT_SOURCE_DIR}/src"
)

add_library(Block STATIC lib/Block.cpp)
target_include_directories(Block PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(Block PRIVATE ${PROJECT_WARNING_FLAGS})

add_library(Inode STATIC lib/Inode.cpp)
target_include_directories(Inode PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(Inode PRIVATE ${PROJECT_WARNING_FLAGS})
target_link_libraries(Inode PUBLIC Block)

add_library(File STATIC lib/File.cpp)
target_include_directories(File PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(File PRIVATE ${PROJECT_WARNING_FLAGS})
target_link_libraries(File PUBLIC Inode)

add_library(Symlink STATIC lib/Symlink.cpp)
target_include_directories(Symlink PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(Symlink PRIVATE ${PROJECT_WARNING_FLAGS})

add_library(Directory STATIC lib/Directory.cpp)
target_include_directories(Directory PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(Directory PRIVATE ${PROJECT_WARNING_FLAGS})
target_link_libraries(Directory PUBLIC File Symlink)

add_library(SnapshotManager STATIC lib/SnapshotManager.cpp)
target_include_directories(SnapshotManager PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(SnapshotManager PRIVATE ${PROJECT_WARNING_FLAGS})
target_link_libraries(SnapshotManager PUBLIC Directory)

add_library(Dispatcher STATIC lib/Dispatcher.cpp)
target_include_directories(Dispatcher PUBLIC ${PROJECT_INCLUDE_DIRS})
target_compile_options(Dispatcher PRIVATE ${PROJECT_WARNING_FLAGS})
target_link_libraries(Dispatcher PUBLIC Directory SnapshotManager Symlink)

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PRIVATE ${PROJECT_WARNING_FLAGS})
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        Block
        Inode
        File
        Symlink
        Directory
        SnapshotManager
        Dispatcher
)

if(BUILD_TESTING)
    # Test executable with CPORTA defined
    add_executable(${PROJECT_NAME}_tests src/main.cpp)
    target_include_directories(${PROJECT_NAME}_tests PRIVATE ${PROJECT_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME}_tests PRIVATE ${PROJECT_WARNING_FLAGS})
    if(WIN32)
        target_compile_options(${PROJECT_NAME}_tests PRIVATE /WX-)
    endif()
    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE CPORTA)
    target_link_libraries(
        ${PROJECT_NAME}_tests
        PRIVATE
            Block
            Inode
            File
            Symlink
            Directory
            SnapshotManager
            Dispatcher
    )

    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif()
