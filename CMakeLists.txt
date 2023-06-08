cmake_minimum_required(VERSION 3.13)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

if(NOT WIN32 AND NOT (UNIX AND NOT APPLE))
	message(FATAL_ERROR "Only Windows and Linux are supported.")
endif()

project(project_tanks LANGUAGES CXX)

if(WIN32)
    set(PLATFORM_FILES code/platform_windows.cpp code/wglext.h)
else()
    set(PLATFORM_FILES code/platform_linux.cpp)
endif()

if(NOT MINGW)
    set(RESOURCE_FILE "")
else()
    #[[ When using GCC on Windows, including the manifest file in an executable is different than including such manifest when using MSVC.
        When using MSVC, you just supply the linker with the manifest file.
        When using GCC, you have to compile that manifest into a *.res file that is then linked together with your object files. ]]
    string(REPLACE "/" "\\" TMP_PATH ${CMAKE_SOURCE_DIR})
    file(WRITE ${CMAKE_BINARY_DIR}\\_resource.rc "#include \"winuser.h\"\n1 RT_MANIFEST ${TMP_PATH}\\win32app.manifest")
    execute_process(COMMAND windres --input "${CMAKE_BINARY_DIR}\\_resource.rc" --output "${CMAKE_BINARY_DIR}\\_resource.res" --output-format=coff WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    file(REMOVE ${CMAKE_BINARY_DIR}\\_resource.rc)
    add_library(resource_file OBJECT IMPORTED)
    set_target_properties(resource_file PROPERTIES IMPORTED_OBJECTS ${CMAKE_BINARY_DIR}\\_resource.res)
    set(RESOURCE_FILE $<TARGET_OBJECTS:resource_file>)
endif()

add_executable(tanks
    code/main.cpp
    code/platform.hpp
    code/exceptions.hpp
    code/khrplatform.h
    code/glcorearb.h
    code/defer.hpp
    code/renderer.hpp
    code/renderer.cpp
    code/opengl.hpp
    code/math.hpp
    code/math.cpp
    code/game.hpp
    code/game.cpp
    code/bullets.cpp
    code/bullets.cpp
    ${PLATFORM_FILES}
    ${RESOURCE_FILE}
)

target_compile_features(tanks PRIVATE cxx_std_20)
set_target_properties(tanks PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(tanks PRIVATE "$<$<CONFIG:DEBUG>:DEBUG_BUILD>")
add_custom_command(TARGET tanks POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:tanks>/assets)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_BINARY_DIR} PROPERTY VS_STARTUP_PROJECT tanks)
    set_target_properties(tanks PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:tanks>"
                                           VS_DEBUGGER_COMMAND "$<TARGET_FILE:tanks>"
                                           VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${CMAKE_PREFIX_PATH}/bin")
    target_compile_options(tanks PRIVATE /utf-8 /W4 /Wall /MP /permissive- /Zc:preprocessor /wd4514 /wd5045 /wd4820 /wd4626 /wd4191)
    target_link_options(tanks PRIVATE /MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/win32app.manifest)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/.editorconfig DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(tanks PRIVATE OpenGL32.lib)
elseif(MINGW)
    target_compile_definitions(tanks PRIVATE __USE_MINGW_ANSI_STDIO=1)
    target_compile_options(tanks PRIVATE -Wall -Wextra -Wconversion -Wshadow -Wno-missing-field-initializers -Wno-cast-function-type -pedantic)
    target_link_libraries(tanks PRIVATE OpenGL32.lib)
else()
    target_compile_options(tanks PRIVATE -Wall -Wextra -Wconversion -Wshadow -Wno-missing-field-initializers -Wno-cast-function-type -pedantic)
endif()
