#ifndef FLUTTER_PLUGIN_DART_CEF_PLUGIN_C_API_H_
#define FLUTTER_PLUGIN_DART_CEF_PLUGIN_C_API_H_

#include <flutter_plugin_registrar.h>
#include <windows.h>

#ifdef FLUTTER_PLUGIN_IMPL
#define FLUTTER_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FLUTTER_PLUGIN_EXPORT __declspec(dllimport)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

FLUTTER_PLUGIN_EXPORT void DartCefPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);

FLUTTER_PLUGIN_EXPORT int initCef(HINSTANCE hInstance, int nCmdShow);
FLUTTER_PLUGIN_EXPORT void setWindowHandle(HWND handle);
FLUTTER_PLUGIN_EXPORT void sendKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);
FLUTTER_PLUGIN_EXPORT void dpiChanged(WPARAM wParam, LPARAM lParam);
FLUTTER_PLUGIN_EXPORT void sendMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_PLUGIN_DART_CEF_PLUGIN_C_API_H_