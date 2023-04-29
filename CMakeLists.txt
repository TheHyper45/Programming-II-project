cmake_minimum_required(VERSION 3.8)
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

add_executable(tanks
    code/main.cpp
    code/platform.hpp
    code/exceptions.hpp
    code/khrplatform.h
    code/glcorearb.h
    code/defer.hpp
    code/renderer.hpp
    code/renderer.cpp
    ${PLATFORM_FILES}
)

target_compile_features(tanks PRIVATE cxx_std_20)
set_target_properties(tanks PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(tanks PRIVATE "$<$<CONFIG:DEBUG>:DEBUG_BUILD>")

if(MSVC)
    set_property(DIRECTORY ${CMAKE_BINARY_DIR} PROPERTY VS_STARTUP_PROJECT tanks)
    set_target_properties(tanks PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:tanks>"
                                           VS_DEBUGGER_COMMAND "$<TARGET_FILE:tanks>"
                                           VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${CMAKE_PREFIX_PATH}/bin")
    target_compile_options(tanks PRIVATE /utf-8 /W4 /Wall /MP /permissive- /Zc:preprocessor)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/.editorconfig DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(tanks PRIVATE OpenGL32.lib)
elseif(MINGW)
    target_compile_definitions(tanks PRIVATE __USE_MINGW_ANSI_STDIO=1)
    target_compile_options(tanks PRIVATE -Wall -Wextra -Wconversion -Wshadow -Wno-missing-field-initializers -Wcast-align=strict -pedantic)
    target_link_libraries(tanks PRIVATE OpenGL32.lib)
else()
    target_compile_options(tanks PRIVATE -Wall -Wextra -Wconversion -Wshadow -Wno-missing-field-initializers -Wcast-align=strict -pedantic)
endif()
