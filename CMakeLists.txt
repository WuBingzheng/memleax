cmake_minimum_required (VERSION 3.1.0)

# projectname is the same as the main-executable
project(memleax)

find_package(PkgConfig)

if (NOT PKG_CONFIG_FOUND)
  message(FATAL_ERROR "Could not find PkgConfig")
endif()

add_executable(
  ${PROJECT_NAME}
  addr_maps.c
  breakpoint.c
  callstack.c
  debug_file.c
  debug_line.c
  memblock.c
  memleax.c
  proc_info.c
  ptr_backtrace.c
  symtab.c
)

target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -DMLX_X86_64)
if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  target_compile_options(${PROJECT_NAME} PRIVATE -DMLX_LINUX)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL FreeBSD)
  target_compile_options(${PROJECT_NAME} PRIVATE -DMLX_FREEBSD)
  target_link_libraries(${PROJECT_NAME} PRIVATE procstat util z)
else()
  message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}. Only GNU/Linux and FreeBSD are supported.")
endif()

pkg_search_module(LIBUNWIND REQUIRED libunwind-ptrace)
pkg_search_module(LIBELF REQUIRED libelf)
target_compile_options(${PROJECT_NAME} PRIVATE ${LIBUNWIND_CFLAGS})
target_compile_options(${PROJECT_NAME} PRIVATE ${LIBELF_CFLAGS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUNWIND_LIBRARIES} ${LIBELF_LIBRARIES})
pkg_search_module(LIBDW libdw)
if (LIBDW_FOUND)
  target_compile_options(${PROJECT_NAME} PRIVATE ${LIBDW_CFLAGS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDW_LIBRARIES})
else()
  pkg_search_module(LIBDWARF libdwarf)
  if (LIBDWARF_FOUND)
    target_compile_options(${PROJECT_NAME} PRIVATE ${LIBDWARF_CFLAGS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDWARF_LIBRARIES})
  else()
    message(WARNING "Could not find libdw or libdwarf. File name and line number will not be shown in backtrace.")
  endif()
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES memleax.1 DESTINATION share/man/man1/)
