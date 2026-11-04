cmake_minimum_required(VERSION 3.24)
project(mages VERSION 0.1.0 LANGUAGES C)

# C11 for our own targets only (SDL builds with its own settings).
function(mages_c11 target)
    set_target_properties(${target} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
endfunction()

option(MAGES_BUILD_APP "Build the SDL3 game executable" ON)
option(MAGES_BUILD_TESTS "Build unit and game tests" ON)
option(MAGES_COVERAGE "Instrument the core library for coverage (GCC/Clang)" OFF)
set(MAGES_SDL_TAG "release-3.2.24" CACHE STRING "SDL3 git tag fetched when SDL3 is not installed")

if(EMSCRIPTEN)
    set(MAGES_BUILD_TESTS OFF)
endif()

# ------------------------------------------------------------------ core
file(GLOB MAGES_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/core/*.c)
add_library(mages_core STATIC ${MAGES_CORE_SOURCES})
mages_c11(mages_core)
target_include_directories(mages_core PUBLIC ${CMAKE_SOURCE_DIR}/src/core)
if(MSVC)
    target_compile_options(mages_core PRIVATE /W4 /wd4244 /wd4267)
    target_compile_definitions(mages_core PUBLIC _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(mages_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
    target_link_libraries(mages_core PUBLIC m)
endif()
if(MAGES_COVERAGE)
    target_compile_options(mages_core PRIVATE --coverage -O0 -g)
    target_link_options(mages_core PUBLIC --coverage)
endif()

# ------------------------------------------------------------------- app
if(MAGES_BUILD_APP)
    if(NOT EMSCRIPTEN)
        find_package(SDL3 CONFIG QUIET)
    endif()
    if(NOT TARGET SDL3::SDL3)
        include(FetchContent)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG ${MAGES_SDL_TAG}
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(SDL3)
    endif()

    add_executable(mages src/platform/main_sdl.c)
    mages_c11(mages)
    target_link_libraries(mages PRIVATE mages_core SDL3::SDL3)
    if(WIN32)
        set_target_properties(mages PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    if(EMSCRIPTEN)
        # EM_JS needs GNU extensions
        set_target_properties(mages PROPERTIES SUFFIX ".html" C_EXTENSIONS ON)
        target_link_options(mages PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sSTACK_SIZE=1048576
            --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html)
        set_property(TARGET mages APPEND PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/web/shell.html)
    endif()
endif()

# ----------------------------------------------------------------- tests
if(MAGES_BUILD_TESTS)
    enable_testing()
    file(GLOB MAGES_UNIT_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/unit/*.c)
    add_executable(unit_tests ${MAGES_UNIT_SOURCES})
    mages_c11(unit_tests)
    target_link_libraries(unit_tests PRIVATE mages_core)
    add_test(NAME unit COMMAND unit_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    add_executable(game_runner tests/game/runner.c)
    mages_c11(game_runner)
    target_link_libraries(game_runner PRIVATE mages_core)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/screenshots)
    file(GLOB MAGES_GAME_SCRIPTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/game/*.script)
    foreach(script ${MAGES_GAME_SCRIPTS})
        get_filename_component(name ${script} NAME_WE)
        add_test(NAME game_${name} COMMAND game_runner ${script} ${CMAKE_BINARY_DIR}/screenshots)
    endforeach()

    if(MAGES_BUILD_APP)
        add_test(NAME smoke_sdl
            COMMAND mages --script ${CMAKE_SOURCE_DIR}/tests/game/smoke.script --frames 180 --no-save)
        set_tests_properties(smoke_sdl PROPERTIES
            ENVIRONMENT "SDL_VIDEO_DRIVER=dummy;SDL_AUDIO_DRIVER=dummy")
    endif()

    find_program(GCOVR gcovr)
    if(MAGES_COVERAGE AND GCOVR)
        add_custom_target(coverage
            COMMAND ${GCOVR} --root ${CMAKE_SOURCE_DIR} --filter ${CMAKE_SOURCE_DIR}/src/core/
                    --object-directory ${CMAKE_BINARY_DIR} --print-summary --fail-under-line 95
                    --html-details ${CMAKE_BINARY_DIR}/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Measuring core line coverage (>= 95% required)")
    endif()
endif()
