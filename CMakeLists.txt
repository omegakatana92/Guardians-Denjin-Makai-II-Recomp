cmake_minimum_required(VERSION 3.20)
project(GuardiansDenjinMakaiII C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Python3 COMPONENTS Interpreter)

set(GRDIANS_ROM "" CACHE FILEPATH "Path to the player's grdians.zip")
if(NOT GRDIANS_ROM AND DEFINED ENV{GRDIANS_ROM})
  set(GRDIANS_ROM "$ENV{GRDIANS_ROM}")
endif()

set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GENERATED_SOURCES)

if(NOT GRDIANS_ROM)
  message(WARNING "GRDIANS_ROM is not set; the native program cannot be generated.")
elseif(NOT EXISTS "${GRDIANS_ROM}")
  message(WARNING "GRDIANS_ROM does not exist: ${GRDIANS_ROM}")
elseif(Python3_Interpreter_FOUND)
  message(STATUS "Translating program from ${GRDIANS_ROM}")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/recompile68k.py"
            "${GRDIANS_ROM}" --out "${GEN_DIR}"
    RESULT_VARIABLE RECOMPILE_RESULT
    OUTPUT_VARIABLE RECOMPILE_OUTPUT
    ERROR_VARIABLE RECOMPILE_ERROR)
  if(NOT RECOMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR "recompile68k.py failed:\n${RECOMPILE_OUTPUT}\n${RECOMPILE_ERROR}")
  endif()
  message(STATUS "${RECOMPILE_OUTPUT}")
  file(GLOB GENERATED_SOURCES CONFIGURE_DEPENDS "${GEN_DIR}/program_*.c")
endif()

add_library(grdians_core STATIC
  runtime/src/board.c
  runtime/src/m68k_ops.c
  runtime/src/runner.c
  runtime/src/png.c
  runtime/third_party/miniz/miniz.c
  runtime/third_party/miniz/miniz_tdef.c
  runtime/third_party/miniz/miniz_tinfl.c
  runtime/third_party/miniz/miniz_zip.c)
target_include_directories(grdians_core PUBLIC
  runtime/include
  ${GEN_DIR}
  runtime/third_party/miniz)


if(GENERATED_SOURCES)
  add_library(grdians_program STATIC ${GENERATED_SOURCES})
  target_include_directories(grdians_program PUBLIC runtime/include ${GEN_DIR})
  target_link_libraries(grdians_program PUBLIC grdians_core)

  add_executable(guardians_headless ports/headless/main.c)
  target_link_libraries(guardians_headless PRIVATE grdians_program)
  set_target_properties(guardians_headless PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  option(GRDIANS_BUILD_SDL2 "Build the SDL2 front end" ON)
  if(GRDIANS_BUILD_SDL2)
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
      find_path(SDL2_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2)
      find_library(SDL2_LIBRARY NAMES SDL2)
      find_library(SDL2_MAIN_LIBRARY NAMES SDL2main)
      if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
        add_library(SDL2::SDL2 UNKNOWN IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
          IMPORTED_LOCATION "${SDL2_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}")
        if(SDL2_MAIN_LIBRARY)
          add_library(SDL2::SDL2main UNKNOWN IMPORTED)
          set_target_properties(SDL2::SDL2main PROPERTIES IMPORTED_LOCATION "${SDL2_MAIN_LIBRARY}")
        endif()
        set(SDL2_FOUND TRUE)
      endif()
    endif()
    if(SDL2_FOUND)
      add_executable(guardians_sdl2 ports/sdl2/main.c)
      target_link_libraries(guardians_sdl2 PRIVATE grdians_program)
      if(TARGET SDL2::SDL2main)
        target_link_libraries(guardians_sdl2 PRIVATE SDL2::SDL2main SDL2::SDL2)
      else()
        target_link_libraries(guardians_sdl2 PRIVATE SDL2::SDL2)
      endif()
      if(WIN32)
        set_target_properties(guardians_sdl2 PROPERTIES OUTPUT_NAME "Guardians-Windows-x86_64")
      else()
        set_target_properties(guardians_sdl2 PROPERTIES OUTPUT_NAME "Guardians")
      endif()
      set_target_properties(guardians_sdl2 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
      message(STATUS "SDL2 front end enabled")
    else()
      message(WARNING "SDL2 not found; only the headless runner will be built")
    endif()
  endif()
endif()

enable_testing()
if(GENERATED_SOURCES)
  add_test(NAME headless_boot
           COMMAND guardians_headless --rom "${GRDIANS_ROM}" --frames 60)
endif()
