// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/texture_registrar.h>
#include "osr_dragdrop_win.h"

#include "include/cef_browser.h"

#include "webview.h"
#include "texture.h"

void newBrowserInstance(int64_t texture_id, const CefString &initialUrl, const CefString &bind_func, const CefString &token, const CefString &access_token);

class BrowserBridge : public virtual CefBaseRefCounted, public client::OsrDragEvents
{
public:
    BrowserBridge(flutter::BinaryMessenger *messenger,
                  flutter::TextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func, const CefString &token, const CefString &access_token);
    ~BrowserBridge();

    void setBrowser(CefRefPtr<CefBrowser> &browser);

    void resetBrowser();

    std::unique_ptr<Texture> texture_bridge;

    std::unique_ptr<Texture> texture_bridge_pet;

    int64_t texture_id() const { return texture_id_; }

    uint32_t width = 1920;
    uint32_t height = 1080;

    int32_t current_offset_x = 0;
    int32_t current_offset_y = 0;

    bool isCurrent = false;

    void OnAfterCreated();

    void OnShutdown();

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

    void OnDragEnterQ(
        const CefString &data);
    void OnDragOverQ();
    void OnDropQ();

    CefMouseEvent getCurrentCursorPosition();

    void OnWebMessage(const CefString &message);

    void OnLoadingStateChange(
        bool isLoading,
        bool canGoBack,
        bool canGoForward);

    void clearAllCookies();

    void mainFrameLoaded(int statusCode);

    void OnTitleChanged(const CefString &title);

    void OnUrlChanged(const CefString &url);

    void onCursorChanged(const cef_cursor_type_t cursor);

    void onPopupShow(bool show);

    void OnPopupSize(int x,
                     int y,
                     int width,
                     int height);

    void scrollUp();

    void scrollDown();

    void changeSize(int width, int height);

    void changeOffset(int x, int y);

    void cursorClick(int x, int y, bool up);

    void loadUrl(const CefString &url);

    void loadHTML(std::string text);

    void browserEvent(WebviewEvent event);

    void sendKeyEvent(const CefKeyEvent &event);

    void sendMouseWheelEvent(CefMouseEvent &event,
                             int deltaX,
                             int deltaY);

    void sendMouseClickEvent(CefMouseEvent &event,
                             CefBrowserHost::MouseButtonType type,
                             bool mouseUp,
                             int clickCount);

    void sendMouseMoveEvent(CefMouseEvent &event,
                            bool mouseLeave);

    void setZoomLevel(double level);

    void textSelectionReport(const CefString &url);

    double getZoomLevel();

    void executeJavaScript(std::string js);

    void setCursorPos(int x, int y);

    void reload(bool ignoreCache);

    void openDevTools();

    void closeBrowser(bool force);

    void setToken(std::string token);

    void setAccessToken(std::string token);

    void paste();

    void setFocus(bool focus);

    bool closing = false;

    CefRefPtr<CefBrowser> browser_;

    bool StartDragging(
        CefRefPtr<CefDragData> drag_data,
        CefRenderHandler::DragOperationsMask allowed_ops,
        int x,
        int y);

private:
    bool dart_focus = false;

    void OnWebviewStateChange(WebviewState state);

    std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
    std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
        event_channel_;

    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
        method_channel_;
    void HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue> &method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    std::unique_ptr<flutter::TextureVariant> flutter_texture_;
    std::unique_ptr<flutter::TextureVariant> flutter_texture_pet_;
    flutter::TextureRegistrar *texture_registrar_;
    int64_t texture_id_;
    int64_t texture_id_pet_;

    template <typename T>
    void EmitEvent(const T &value)
    {
        if (event_sink_)
        {
            event_sink_->Success(value);
        }
    }
    IMPLEMENT_REFCOUNTING(BrowserBridge);
};
