# stop if cmake version below 3.8
cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

# project name
project(memleax)

# enable c language
enable_language(C CXX)

# project version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# detect operating system
message(STATUS "We are on a ${CMAKE_SYSTEM_NAME} system")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_definitions(-DMLX_LINUX)
    add_definitions(-DMLX_WITH_LIBDW)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    add_definitions(-DMLX_DARWIN)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "AIX")
    add_definitions(-DMLX_AIX)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DMLXM_WINDOWS)
endif()

# detect host processor
message(STATUS "The host processor is ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86")
    add_definitions(-DMLX_X86)
endif()
if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    add_definitions(-DMLX_X86_64)
endif()

# build executable
file(GLOB C_SRC_FILES "*.c")
add_executable(memleax ${C_SRC_FILES})

find_package(PkgConfig REQUIRED)

pkg_check_modules(ELF    REQUIRED libelf)
pkg_check_modules(DWARF  REQUIRED libdw)
pkg_check_modules(UNWIND REQUIRED libunwind-ptrace)

target_link_libraries(memleax             ${ELF_LIBRARIES}    ${DWARF_LIBRARIES}    ${UNWIND_LIBRARIES})
target_compile_options(memleax     PUBLIC ${ELF_CFLAGS_OTHER} ${DWARF_CFLAGS_OTHER} ${UNWIND_CFLAGS_OTHER})
target_include_directories(memleax PUBLIC ${ELF_INCLUDE_DIRS} ${DWARF_INCLUDE_DIRS} ${UNWIND_INCLUDE_DIRS})
