// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#pragma once

#include "include/cef_client.h"
#include "include/cef_browser.h"
#include "include/cef_render_process_handler.h"

#include <memory>
#include <functional>
#include <optional>
#include <thread>
#include <mutex>

#include "browser.h"

class SimpleHandler : public CefClient,
                      public CefDisplayHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRenderHandler,
                      public CefContextMenuHandler
{
public:
  SimpleHandler();
  ~SimpleHandler();

  // Provide access to the single global instance of this object.
  static SimpleHandler *GetInstance();

  virtual void OnAddressChange(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString &url) override;

  virtual void SimpleHandler::OnBeforeContextMenu(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefContextMenuParams> params,
      CefRefPtr<CefMenuModel> model) override
  {
    model->Clear();
  };

  virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString &title) override;

  virtual bool OnCursorChange(CefRefPtr<CefBrowser> browser,
                              CefCursorHandle cursor,
                              cef_cursor_type_t type,
                              const CefCursorInfo &custom_cursor_info) override;

  // CefClient methods:
  virtual CefRefPtr<CefDisplayHandler>
  GetDisplayHandler() override
  {
    return this;
  }

  virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override
  {
    return this;
  }
  virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
  {
    return this;
  }
  virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

  virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefProcessId source_process,
                                        CefRefPtr<CefProcessMessage> message) override;

  virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                    bool isLoading,
                                    bool canGoBack,
                                    bool canGoForward) override;

  virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) override;

  virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

  // CefLifeSpanHandler methods:
  virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
  virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // create new browser and return texture_id for flutter side
  int64_t createBrowser(flutter::BinaryMessenger *messenger,
                        flutter::TextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func,
                        const CefString &token, const CefString &access_token);

  // CefLoadHandler methods:
  virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           ErrorCode errorCode,
                           const CefString &errorText,
                           const CefString &failedUrl) override;

  virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;

  virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) override;
  virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;

  virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect) override;

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

  // Request that all existing browser windows close.
  void CloseAllBrowsers(bool force_close);

  bool IsClosing() const { return is_closing_; }

  CefRefPtr<BrowserBridge> getBridge(int browser_id);

private:
  // Platform-specific implementation.
  void PlatformTitleChange(CefRefPtr<CefBrowser> browser,
                           const CefString &title);

  // Map of existing browser windows (texture id -> bridge). Needs to be cleaned when browser destroys
  std::map<int64_t, CefRefPtr<BrowserBridge>> browser_list_;

  // Map of browser id -> texture id
  std::map<int, int64_t> cache_;

  // Handler is closing
  bool is_closing_;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(SimpleHandler);
};