# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause the
# plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.15)

# Project-level configuration.
set(PROJECT_NAME "dart_cef")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)

remove_definitions(-Werror)

add_subdirectory(third_party/fmt)

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

# enable FetchContent
include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

if(LINUX)
  execute_process(
    COMMAND uname -m
    COMMAND tr -d '\n'
    OUTPUT_VARIABLE ARCH)
  if(${ARCH} STREQUAL "x86_64")
    set(ARCH "64")
  elseif(${ARCH} STREQUAL "aarch64")
    set(ARCH "arm64")
    set(PROJECT_ARCH
        ${ARCH}
        CACHE INTERNAL "specify project arch")
  endif()
  set(CEF_ARCH linux${ARCH})
endif()
if(APPLE)
  set(CEF_ARCH macosx64)
endif()

message("building for ${CEF_ARCH}")

option(USE_SANDBOX "" OFF)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL YES)

FetchContent_Declare(
  cef
  URL https://cef-builds.spotifycdn.com/cef_binary_107.1.12+g65b79a6+chromium-107.0.5304.122_${CEF_ARCH}.tar.bz2
)

FetchContent_MakeAvailable(cef)

FetchContent_GetProperties(
  cef
  SOURCE_DIR cef_source
  BINARY_DIR cef_binary)

get_target_property(LIBCEF_OPTIONS libcef_dll_wrapper COMPILE_OPTIONS)
list(REMOVE_ITEM LIBCEF_OPTIONS -Werror)
set_target_properties(libcef_dll_wrapper PROPERTIES COMPILE_OPTIONS "${LIBCEF_OPTIONS}")

# This value is used when generating builds using this plugin, so it must not be
# changed.
set(PLUGIN_NAME "dart_cef_plugin")

# Define the plugin library target. Its name must not be changed (see comment on
# PLUGIN_NAME above).
#
# Any new source files that you add to the plugin should be added here.
add_library(
  ${PLUGIN_NAME} SHARED
  "dart_cef_plugin.cc"
  "app_delegates_browser.cc"
  "app_delegates_renderer.cc"
  "browser_delegate.cc"
  "client_app_other.cc"
  "client_app.cc"
  "client_browser.cc"
  "client_renderer.cc"
  "client_switches.cc"
  "main_message_loop.cc"
  "main_message_loop_multithreaded_gtk.cc"
  "renderer_delegate.cc"
  "data.cpp"
  "browser.cc"
  "simple_handler.cc"
  "simple_handler_win.cc"
  "video_outlet.cc")

# Apply a standard set of build settings that are configured in the
# application-level CMakeLists.txt. This can be removed for plugins that want
# full control over build settings.

# apply_standard_settings(${PLUGIN_NAME} )

# Symbols are hidden by default to reduce the chance of accidental conflicts
# between plugins. This should not be removed; any symbols that should be
# exported should be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

get_target_property(PLUGIN_OPTIONS ${PLUGIN_NAME} COMPILE_OPTIONS)
message("plugin options are ${PLUGIN_OPTIONS}")
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

target_link_libraries(${PLUGIN_NAME} PRIVATE flutter PkgConfig::GTK fmt::fmt
                                             libcef_dll_wrapper libcef.so)



# List of absolute paths to libraries that should be bundled with the plugin.
# This list could contain prebuilt libraries, or libraries created by an
# external build triggered from this build file.
set(dart_cef_bundled_libraries
    ${cef_source}/Resources/icudtl.dat
    ${cef_source}/Resources/resources.pak
    ${cef_source}/Resources/locales
    $<$<CONFIG:Debug>:${cef_source}/Debug/libcef.so>
    $<$<CONFIG:Debug>:${cef_source}/Debug/v8_context_snapshot.bin>
    $<$<CONFIG:Release>:${cef_source}/Release/libcef.so>
    $<$<CONFIG:Release>:${cef_source}/Release/v8_context_snapshot.bin>
    $<$<CONFIG:Profile>:${cef_source}/Release/libcef.so>
    $<$<CONFIG:Profile>:${cef_source}/Release/v8_context_snapshot.bin>
    PARENT_SCOPE)
