#ifndef FLUTTER_PLUGIN_dart_cef_PLUGIN_H_
#define FLUTTER_PLUGIN_dart_cef_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include "webview.h"
#include "simple_handler.h"

#include <memory>

namespace dart_cef
{

    class DartCefPlugin : public flutter::Plugin, public client::OsrDragEvents
    {

    public:
        static DartCefPlugin *GetInstance();

        static HWND hwnd;
        static SimpleHandler *handler;
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);
        DWORD cefUIThread;
        DWORD appUIThread;
        void setHWND(HWND whwnd);
        CComPtr<client::DropTargetWin> drop_target;
        CefRenderHandler::DragOperation current_drag_op = DRAG_OPERATION_NONE;
        float device_scale_factor;
        void createOle();

        DartCefPlugin(flutter::TextureRegistrar *textures,
                         flutter::BinaryMessenger *messenger);

        virtual ~DartCefPlugin();

        // Disallow copy and assign.
        DartCefPlugin(const DartCefPlugin &) = delete;
        DartCefPlugin &operator=(const DartCefPlugin &) = delete;
        void sendMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
        void sendKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);

        template <typename T>
        void EmitEvent(const T &value)
        {
            if (event_sink_)
            {
                event_sink_->Success(value);
            }
        }

        CefBrowserHost::DragOperationsMask OnDragEnter(
            CefRefPtr<CefDragData> drag_data,
            CefMouseEvent ev,
            CefBrowserHost::DragOperationsMask effect) override;
        CefBrowserHost::DragOperationsMask OnDragOver(
            CefMouseEvent ev,
            CefBrowserHost::DragOperationsMask effect) override;
        void OnDragLeave() override;
        CefBrowserHost::DragOperationsMask OnDrop(
            CefMouseEvent ev,
            CefBrowserHost::DragOperationsMask effect) override;

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
        bool mouse_rotation_;
    };

} // namespace dart_cef

#endif // FLUTTER_PLUGIN_dart_cef_PLUGIN_H_
