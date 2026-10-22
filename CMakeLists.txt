cmake_minimum_required(VERSION 3.21)
project(cna_backrooms LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CNA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cna" CACHE PATH "CNA next checkout")
set(CNA_SHARP_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../sharp-runtime" CACHE PATH "sharp-runtime next checkout")
set(CNA_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../cna-engine" CACHE PATH "cna-engine checkout (strict modules only)")
if(EMSCRIPTEN)
    # The browser build (tools/build_web.sh): EasyGL over WebGL 2.
    set(CNA_GRAPHICS_RENDERER WEBGL2 CACHE STRING "EasyGL WebGL 2 renderer" FORCE)
else()
    set(CNA_GRAPHICS_RENDERER OPENGLES3 CACHE STRING "EasyGL OpenGL ES renderer" FORCE)
endif()
set(CNA_BUILD_TESTS OFF CACHE BOOL "CNA tests" FORCE)
set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "CNA examples" FORCE)
set(SHARP_RUNTIME_BUILD_TESTS OFF CACHE BOOL "sharp-runtime tests" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "dependency tests" FORCE)
# Standalone game builds exclude dependency-wide source inventory audits; see bugs.md.
set(CNA_PLATFORM_RATCHET OFF CACHE BOOL "Engine developer SDL-coupling audits" FORCE)
# cna-engine links CNA's static module libraries; a shared CNA would put a
# second copy of CNA into this process (cna-engine upstream-issues.md, UP-006).
set(CNA_SHARED_LIBRARY OFF CACHE BOOL "CNA as one shared library" FORCE)
add_subdirectory("${CNA_ROOT_DIR}" CNA_BUILD)
sharp_runtime_enable_component(Text.Json)
# cna-engine over the CNA just added: the strict modules (XNA 4.0 types only)
# for drawing and menus, and its enhanced graphics for shadow reception (CNA's
# shadow extension). A new compiler warning inside the engine must not break
# this game's build.
set(CNA_ENGINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CNA_ENGINE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CNA_ENGINE_WITH_EXTENDED OFF CACHE BOOL "" FORCE)
set(CNA_ENGINE_WITH_ENHANCED_GRAPHICS ON CACHE BOOL "" FORCE)
set(CNA_ENGINE_INSTALL OFF CACHE BOOL "" FORCE)
set(CNA_ENGINE_PILOT_PROBES OFF CACHE BOOL "" FORCE)
set(CNA_ENGINE_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CNA_ENGINE_ROOT}" cna_engine EXCLUDE_FROM_ALL)
add_library(backrooms_world STATIC src/World.cpp src/Lighting.cpp)
target_include_directories(backrooms_world PUBLIC src)
target_compile_options(backrooms_world PRIVATE -Wall -Wextra -Wpedantic)
if(EMSCRIPTEN)
    # Every frame an exception can cross must use CNA's JS-lowered exception ABI.
    target_compile_options(backrooms_world PUBLIC -fexceptions)
endif()
add_library(backrooms_profiles STATIC src/LevelProfiles.cpp src/Assets.cpp)
target_link_libraries(backrooms_profiles PUBLIC backrooms_world SharpRuntime::Text.Json SharpRuntime::IO)
target_compile_options(backrooms_profiles PRIVATE -Wall -Wextra -Wpedantic)
add_executable(cna_backrooms src/main.cpp src/BackroomsGame.cpp src/Materials.cpp)
set_target_properties(cna_backrooms PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN")
if(EMSCRIPTEN)
    # The browser build (tools/build_web.sh): one HTML page, the wasm, and
    # assets/ as one upfront preload mounted at /assets, where
    # FindAssetDirectory finds it from the working directory "/". The blocking
    # Game::Run loop runs through Asyncify, exceptions use CNA's JS-lowered ABI.
    # Same recipe as school-simulator.
    target_link_libraries(cna_backrooms PRIVATE CNA::EmscriptenExceptionAbi CNA::EmscriptenAsyncify)
    target_link_options(cna_backrooms PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1048576
        -sINITIAL_MEMORY=268435456
        -sFORCE_FILESYSTEM=1
        -sEXIT_RUNTIME=1
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets")
    set_property(TARGET cna_backrooms APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    set_target_properties(cna_backrooms PROPERTIES SUFFIX ".html")
    cna_apply_emscripten_renderer_link_contract(cna_backrooms)
else()
    add_custom_target(backrooms_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:cna_backrooms>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/run.sh" "$<TARGET_FILE_DIR:cna_backrooms>/run.sh"
        COMMENT "Copying the game's visual and licensed audio assets")
    add_dependencies(cna_backrooms backrooms_assets)
endif()
target_include_directories(cna_backrooms PRIVATE src)
target_link_libraries(cna_backrooms PRIVATE CNA CnaEngine::Graphics CnaEngine::EnhancedGraphics CnaEngine::UI
    backrooms_profiles)
target_compile_options(cna_backrooms PRIVATE -Wall -Wextra -Wpedantic)
add_executable(world_tests tests/WorldTests.cpp)
target_link_libraries(world_tests PRIVATE backrooms_world)
target_include_directories(world_tests PRIVATE src)
target_compile_options(world_tests PRIVATE -Wall -Wextra -Wpedantic)
add_executable(world_quality tools/WorldQuality.cpp)
target_link_libraries(world_quality PRIVATE backrooms_profiles)
target_include_directories(world_quality PRIVATE src)
target_compile_options(world_quality PRIVATE -Wall -Wextra -Wpedantic)
add_executable(world_walkability tools/WorldWalkability.cpp)
target_link_libraries(world_walkability PRIVATE backrooms_profiles)
target_include_directories(world_walkability PRIVATE src)
target_compile_options(world_walkability PRIVATE -Wall -Wextra -Wpedantic)
if(TARGET SDL3::SDL3)
    target_link_libraries(cna_backrooms PRIVATE SDL3::SDL3)
endif()
if(TARGET SDL3::SDL3main)
    target_link_libraries(cna_backrooms PRIVATE SDL3::SDL3main)
endif()
add_executable(world_route tools/WorldRoute.cpp)
target_link_libraries(world_route PRIVATE backrooms_profiles)
target_include_directories(world_route PRIVATE src)
target_compile_options(world_route PRIVATE -Wall -Wextra -Wpedantic)
add_executable(level_profile_tests tests/LevelProfileTests.cpp)
target_link_libraries(level_profile_tests PRIVATE backrooms_profiles)
target_compile_options(level_profile_tests PRIVATE -Wall -Wextra -Wpedantic)

add_executable(lighting_tests tests/LightingTests.cpp)
target_link_libraries(lighting_tests PRIVATE backrooms_world)
target_compile_options(lighting_tests PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_test(NAME world COMMAND world_tests)
add_test(NAME level_profiles COMMAND level_profile_tests)
add_test(NAME lighting COMMAND lighting_tests)
set_tests_properties(world level_profiles lighting PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
