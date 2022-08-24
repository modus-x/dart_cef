#include "include/dart_cef/dart_cef_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "dart_cef_plugin.h"

void DartCefPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  dart_cef::DartCefPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
