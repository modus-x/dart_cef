// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#pragma once

#include "include/cef_browser.h"

#include "webview.h"

#include <flutter_linux/flutter_linux.h>

void newBrowserInstance(int64_t texture_id, const CefString &initialUrl, const CefString &bind_func, const CefString &token, const CefString &access_token);

class BrowserBridge : public virtual CefBaseRefCounted
{
public:
    BrowserBridge(FlBinaryMessenger *messenger,
                  FlTextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func, const CefString &token, const CefString &access_token);
    ~BrowserBridge();

    void setBrowser(CefRefPtr<CefBrowser> &browser);

    void resetBrowser();

    // VideoOutlet* texture_bridge;

    // VideoOutlet* texture_bridge_pet;

    uint32_t width = 1920;
    uint32_t height = 1080;

    int32_t current_offset_x = 0;
    int32_t current_offset_y = 0;

    bool isCurrent = false;

    void OnAfterCreated();

    void OnShutdown();

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

    bool closing = false;

private:
    CefRefPtr<CefBrowser> browser_;

    void OnWebviewStateChange(WebviewState state);

    FlEventChannel *event_channel_;

    FlMethodChannel *method_channel_;
    void HandleMethodCall(
        FlMethodCall *method_call);

    FlTextureRegistrar *texture_registrar_;

    template <typename T>
    void EmitEvent(const T &value)
    {
        // if (event_sink_)
        // {
        //     event_sink_->Success(value);
        // }
    }
    IMPLEMENT_REFCOUNTING(BrowserBridge);
};
