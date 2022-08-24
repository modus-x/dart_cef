#ifndef FLUTTER_PLUGIN_DART_CEF_PLUGIN_H_
#define FLUTTER_PLUGIN_DART_CEF_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace dart_cef {

class DartCefPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  DartCefPlugin();

  virtual ~DartCefPlugin();

  // Disallow copy and assign.
  DartCefPlugin(const DartCefPlugin&) = delete;
  DartCefPlugin& operator=(const DartCefPlugin&) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace dart_cef

#endif  // FLUTTER_PLUGIN_DART_CEF_PLUGIN_H_
