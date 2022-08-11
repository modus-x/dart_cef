#include "include/webview_cef/webview_cef_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "webview_cef_plugin.h"
#include "include/cef_app.h"
#include <memory>
#include <thread>
#include "client_browser.h"
#include "client_app_other.h"
#include "client_switches.h"
#include "client_renderer.h"
#include "simple_handler.h"
#include "data.h"

using namespace client;

#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"

void WebviewCefPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar)
{
  webview_cef::WebviewCefPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

FLUTTER_PLUGIN_EXPORT int initCEFProcesses(HINSTANCE hInstance, int nCmdShow)
{
  // Enable High-DPI support on Windows 7 or newer.
  CefEnableHighDPISupport();

  CefMainArgs main_args(hInstance);

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromString(::GetCommandLineW());

  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);

  std::cout << "process type is " << process_type << std::endl;
  if (process_type == ClientApp::BrowserProcess)
    app = new ClientAppBrowser();
  else if (process_type == ClientApp::RendererProcess)
    app = new ClientAppRenderer();
  else if (process_type == ClientApp::OtherProcess)
    app = new ClientAppOther();

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app, nullptr);
  if (exit_code >= 0)
    return exit_code;

  CefSettings settings;

  settings.no_sandbox = true;
  settings.windowless_rendering_enabled = true;
  settings.multi_threaded_message_loop = true;
  CefString(&settings.log_file).FromASCII("webview_cef.log");

  // Initialize CEF.
  CefInitialize(main_args, settings, app, nullptr);

  return exit_code;
}

FLUTTER_PLUGIN_EXPORT void setWindowHandle(HWND handle)
{
  webview_cef::WebviewCefPlugin::hwnd = handle;
}

FLUTTER_PLUGIN_EXPORT void sendKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
  if (webview_cef::WebviewCefPlugin::GetInstance())
  {
    webview_cef::WebviewCefPlugin::GetInstance()->sendKeyboardEvent(message, wParam, lParam);
  }
}

FLUTTER_PLUGIN_EXPORT void dpiChanged(WPARAM wParam, LPARAM lParam)
{
  LOG(INFO) << "dpi changed!";
}

FLUTTER_PLUGIN_EXPORT void sendMouseEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
  if (webview_cef::WebviewCefPlugin::GetInstance())
  {
    webview_cef::WebviewCefPlugin::GetInstance()->sendMouseEvent(message, wParam, lParam);
  }
}
