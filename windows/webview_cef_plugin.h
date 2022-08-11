#ifndef FLUTTER_PLUGIN_WEBVIEW_CEF_PLUGIN_H_
#define FLUTTER_PLUGIN_WEBVIEW_CEF_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include "webview.h"
#include "simple_handler.h"

#include <memory>

namespace webview_cef
{

    class WebviewCefPlugin : public flutter::Plugin
    {

    public:
        static WebviewCefPlugin *GetInstance();

        static HWND hwnd;
        static SimpleHandler *handler;
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

        WebviewCefPlugin(flutter::TextureRegistrar *textures,
                         flutter::BinaryMessenger *messenger);

        virtual ~WebviewCefPlugin();

        // Disallow copy and assign.
        WebviewCefPlugin(const WebviewCefPlugin &) = delete;
        WebviewCefPlugin &operator=(const WebviewCefPlugin &) = delete;
        void sendMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
        void sendKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);

    private:
        flutter::TextureRegistrar *textures_;
        flutter::BinaryMessenger *messenger_;
        std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
            event_channel_;
        // Called when a method is called on this plugin's channel from Dart.
        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue> &method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;

        // Mouse state tracking.
        POINT last_mouse_pos_;
        POINT current_mouse_pos_;
        bool mouse_tracking_;
        int last_click_x_;
        int last_click_y_;
        CefBrowserHost::MouseButtonType last_click_button_;
        int last_click_count_;
        double last_click_time_;
        bool  mouse_rotation_;
    };

} // namespace webview_cef

#endif // FLUTTER_PLUGIN_WEBVIEW_CEF_PLUGIN_H_
