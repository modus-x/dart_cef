# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.15)

# Project-level configuration.
set(PROJECT_NAME "dart_cef")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)

set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/../bin)

set(THIRD_PARTY ${BIN}/third_party)

# add_subdirectory(${THIRD_PARTY}/fmt "${CMAKE_BINARY_DIR}/fmt")
add_subdirectory(third_party/fmt)

# This value is used when generating builds using this plugin, so it must
# not be changed.
set(PLUGIN_NAME "dart_cef_plugin")

# Define the plugin library target. Its name must not be changed (see comment
# on PLUGIN_NAME above).
#
# Any new source files that you add to the plugin should be added here.
add_library(${PLUGIN_NAME} SHARED
  "dart_cef_plugin.cc"
  "app_delegates_browser.cc"
  "app_delegates_renderer.cc"
  "browser_delegate.cc"
  "client_app_other.cc"
  "client_app.cc"
  "client_browser.cc"
  "client_renderer.cc"
  "client_switches.cc"
  "main_message_loop_multithreaded_gtk.cc"
  "main_message_loop.cc"
  "renderer_delegate.cc"
  "data.cpp"
  "browser.cc"
  "simple_handler.cc"
  "simple_handler_win.cc"
)

# Apply a standard set of build settings that are configured in the
# application-level CMakeLists.txt. This can be removed for plugins that want
# full control over build settings.
apply_standard_settings(${PLUGIN_NAME})

# Symbols are hidden by default to reduce the chance of accidental conflicts
# between plugins. This should not be removed; any symbols that should be
# exported should be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

# Source include directories and library dependencies. Add any plugin-specific
# dependencies here.
target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE ${BIN}/linux/arm64 "third_party/fmt/include")


target_link_directories(
  ${PLUGIN_NAME} PRIVATE ${BIN}/linux/arm64/Debug ${BIN}/linux/arm64/build/libcef_dll_wrapper
)

target_link_libraries(
  ${PLUGIN_NAME}
  PRIVATE flutter
  PkgConfig::GTK
  fmt::fmt
  debug
  cef_dll_wrapper
  libcef.so
  optimized
  cef_dll_wrapper
  libcef.so
)

# Set rpath so that libraries can be placed next to the executable.


# List of absolute paths to libraries that should be bundled with the plugin.
# This list could contain prebuilt libraries, or libraries created by an
# external build triggered from this build file.
set(dart_cef_bundled_libraries
  ${BIN}/linux/arm64/Resources/icudtl.dat
  ${BIN}/linux/arm64/Resources/resources.pak
  $<$<CONFIG:Debug>:${BIN}/linux/arm64/Debug/libcef.so>
  $<$<CONFIG:Debug>:${BIN}/linux/arm64/Debug/v8_context_snapshot.bin>
  $<$<CONFIG:Debug>:${BIN}/linux/arm64/Debug/snapshot_blob.bin>
  $<$<CONFIG:Debug>:${BIN}/linux/arm64/Debug/chrome-sandbox>
  $<$<CONFIG:Release>:${BIN}/linux/arm64/Release/libcef.so>
  $<$<CONFIG:Release>:${BIN}/linux/arm64/Release/v8_context_snapshot.bin>
  $<$<CONFIG:Profile>:${BIN}/linux/arm64/Release/libcef.so>
  $<$<CONFIG:Profile>:${BIN}/linux/arm64/Release/v8_context_snapshot.bin>
  PARENT_SCOPE)
