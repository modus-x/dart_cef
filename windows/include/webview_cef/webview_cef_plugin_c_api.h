#ifndef FLUTTER_PLUGIN_WEBVIEW_CEF_PLUGIN_C_API_H_
#define FLUTTER_PLUGIN_WEBVIEW_CEF_PLUGIN_C_API_H_

#include <windows.h>
#include <flutter_plugin_registrar.h>

#ifdef FLUTTER_PLUGIN_IMPL
#define FLUTTER_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FLUTTER_PLUGIN_EXPORT __declspec(dllimport)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

FLUTTER_PLUGIN_EXPORT void WebviewCefPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);

FLUTTER_PLUGIN_EXPORT int initCEFProcesses(HINSTANCE hInstance, int nCmdShow);

FLUTTER_PLUGIN_EXPORT void dpiChanged(WPARAM wParam, LPARAM lParam);

FLUTTER_PLUGIN_EXPORT void setWindowHandle(HWND handle);

FLUTTER_PLUGIN_EXPORT void sendKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);

FLUTTER_PLUGIN_EXPORT void sendMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_PLUGIN_WEBVIEW_CEF_PLUGIN_C_API_H_
