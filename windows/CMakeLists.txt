# The Flutter tooling requires that developers have a version of Visual Studio
# installed that includes CMake 3.14 or later. You should not increase this
# version, as doing so will cause the plugin to fail to compile for some
# customers of the plugin.
cmake_minimum_required(VERSION 3.14)

# Project-level configuration.
set(PROJECT_NAME "dart_cef")
project(${PROJECT_NAME} LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(STATUS "${CMAKE_BUILD_TYPE}")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

add_subdirectory(third_party/fmt)

option(USE_SANDBOX "" OFF)

set(CEF_ARCH windows64)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL YES)

FetchContent_Declare(
  cef
  URL https://cef-builds.spotifycdn.com/cef_binary_107.1.12+g65b79a6+chromium-107.0.5304.122_${CEF_ARCH}.tar.bz2
)
set(CEF_RUNTIME_LIBRARY_FLAG
    "/MD"
    CACHE STRING "Optional flag specifying which runtime to use")

FetchContent_MakeAvailable(cef)

FetchContent_GetProperties(
  cef
  SOURCE_DIR cef_source
  BINARY_DIR cef_binary)

# This value is used when generating builds using this plugin, so it must not be
# changed
set(PLUGIN_NAME "dart_cef_plugin")

# Any new source files that you add to the plugin should be added here.
list(
  APPEND
  PLUGIN_SOURCES
  "app_delegates_browser.cc"
  "app_delegates_renderer.cc"
  "browser_delegate.cc"
  "browser_delegate.h"
  "client_app_other.cc"
  "client_app_other.h"
  "client_app.cc"
  "client_app.h"
  "client_browser.cc"
  "client_browser.h"
  "client_renderer.cc"
  "client_renderer.h"
  "client_switches.cc"
  "client_switches.h"
  "main_message_loop_multithreaded_win.cc"
  "main_message_loop_multithreaded_win.h"
  "main_message_loop.cc"
  "main_message_loop.h"
  "renderer_delegate.cc"
  "renderer_delegate.h"
  "resource.h"
  "util_win.h"
  "geometry_util.cc"
  "geometry_util.h"
  "bytes_write_handler.cc"
  "bytes_write_handler.h"
  "osr_dragdrop_events.h"
  "osr_dragdrop_win.cc"
  "osr_dragdrop_win.h"
  "util_win.cc"
  "data.cpp"
  "data.h"
  "browser.cc"
  "browser.h"
  "dart_cef_plugin.cpp"
  "dart_cef_plugin.h"
  "simple_handler.cc"
  "simple_handler.h"
  "simple_handler_win.cc"
  "texture.cc"
  "texture.h"
  "webview.h")

# Define the plugin library target. Its name must not be changed (see comment on
# PLUGIN_NAME above).
add_library(${PLUGIN_NAME} SHARED "include/dart_cef/dart_cef_plugin_c_api.h"
                                  "dart_cef_plugin_c_api.cpp" ${PLUGIN_SOURCES})

# Apply a standard set of build settings that are configured in the
# application-level CMakeLists.txt. This can be removed for plugins that want
# full control over build settings.
apply_standard_settings(${PLUGIN_NAME})

# Symbols are hidden by default to reduce the chance of accidental conflicts
# between plugins. This should not be removed; any symbols that should be
# exported should be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

# Source include directories and library dependencies. Add any plugin-specific
# dependencies here.
target_include_directories(
  ${PLUGIN_NAME}
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE ${cef_source} "third_party/fmt/include")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_link_directories(${PLUGIN_NAME} PRIVATE ${cef_source}/Debug)
else()
  target_link_directories(${PLUGIN_NAME} PRIVATE ${cef_source}/Release)
endif()

target_link_libraries(
  ${PLUGIN_NAME}
  PRIVATE flutter
          flutter_wrapper_plugin
          fmt::fmt
          libcef_dll_wrapper
          debug
          ${cef_source}/Debug/libcef.lib
          optimized
          ${cef_source}/Release/libcef.lib)

set(dart_cef_bundled_libraries
    ${cef_source}/Resources/icudtl.dat
    ${cef_source}/Resources/resources.pak
    ${cef_source}/Resources/locales
    $<$<CONFIG:Debug>:${cef_source}/Debug/libcef.dll>
    $<$<CONFIG:Debug>:${cef_source}/Debug/chrome_elf.dll>
    $<$<CONFIG:Debug>:${cef_source}/Debug/v8_context_snapshot.bin>
    $<$<CONFIG:Release>:${cef_source}/Release/libcef.dll>
    $<$<CONFIG:Release>:${cef_source}/Release/chrome_elf.dll>
    $<$<CONFIG:Release>:${cef_source}/Release/v8_context_snapshot.bin>
    $<$<CONFIG:Profile>:${cef_source}/Release/libcef.dll>
    $<$<CONFIG:Profile>:${cef_source}/Release/chrome_elf.dll>
    $<$<CONFIG:Profile>:${cef_source}/Release/v8_context_snapshot.bin>
    PARENT_SCOPE)
