// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "browser.h"

#include <string>
#include <fmt/core.h>
#include <optional>

#include "data.h"
#include "renderer_delegate.h"
#include "simple_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

namespace
{

  class WebviewCookieClearCompletionCallBack : public CefDeleteCookiesCallback
  {
  public:
    WebviewCookieClearCompletionCallBack(WebviewEvent event, CefRefPtr<BrowserBridge> bridge) : event_(event), bridge_(bridge){};

    void OnComplete(int num) override
    {
      bridge_->browserEvent(event_);
    }

  private:
    WebviewEvent event_;
    CefRefPtr<BrowserBridge> bridge_;

    IMPLEMENT_REFCOUNTING(WebviewCookieClearCompletionCallBack);
  };

}

void newBrowserInstance(int64_t texture_id, const CefString &initialUrl, const CefString &bind_func, const CefString &token, const CefString &access_token)
{
  if (!CefCurrentlyOn(TID_UI))
  {
    CefPostTask(TID_UI, base::BindOnce(newBrowserInstance, texture_id, initialUrl, bind_func, token, access_token));
    return;
  }
  else
  {
    CefWindowInfo window_info;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsWindowless(nullptr);
#endif
    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = 60;
    CefRefPtr<CefDictionaryValue> extra = CefDictionaryValue::Create();
    extra->SetString("texture_id", std::to_string(texture_id));
    extra->SetString("bind_func", bind_func);
    extra->SetString("token", token);
    extra->SetString("access_token", access_token);
    CefBrowserHost::CreateBrowser(window_info, SimpleHandler::GetInstance(),
                                  initialUrl, browser_settings,
                                  extra, nullptr);
    LOG(INFO) << "Sent request to create browser for texture " << texture_id << " with initial url " << initialUrl;
  }
}

BrowserBridge::BrowserBridge(
    FlBinaryMessenger *messenger,
    FlTextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func, const CefString &token, const CefString &access_token)
    : texture_registrar_(texture_registrar)
{
}

void BrowserBridge::setBrowser(CefRefPtr<CefBrowser> &browser)
{
  browser_ = browser;
}

void BrowserBridge::clearAllCookies()
{
  CefRefPtr<CefDeleteCookiesCallback> callback =
      new WebviewCookieClearCompletionCallBack(WebviewEvent::CookiesCleared, this);
  CefCookieManager::GetGlobalManager(nullptr)->DeleteCookies("", "", callback);
}

BrowserBridge::~BrowserBridge()
{
}

void BrowserBridge::OnLoadingStateChange(
    bool isLoading,
    bool canGoBack,
    bool canGoForward)
{
}

void BrowserBridge::mainFrameLoaded(int statusCode)
{
}

void BrowserBridge::onPopupShow(bool show)
{
}

void BrowserBridge::OnPopupSize(int x,
                                int y,
                                int popupWidth,
                                int popupHeight)
{
}

void BrowserBridge::OnWebMessage(const CefString &message)
{
}

void BrowserBridge::OnAfterCreated()
{
  OnWebviewStateChange(WebviewState::Ready);
}

void BrowserBridge::setToken(std::string token)
{
  CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kTokenUpdate);
  CefRefPtr<CefListValue> args = message->GetArgumentList();
  args->SetString(0, token);
  browser_->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void BrowserBridge::setAccessToken(std::string token)
{
  CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kAccessTokenUpdate);
  CefRefPtr<CefListValue> args = message->GetArgumentList();
  args->SetString(0, token);
  browser_->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void BrowserBridge::OnShutdown()
{
  OnWebviewStateChange(WebviewState::Shutdown);
}

void BrowserBridge::OnTitleChanged(const CefString &title)
{
}

void BrowserBridge::textSelectionReport(const CefString &url)
{
}

void BrowserBridge::onCursorChanged(const cef_cursor_type_t cursor)
{
  // const auto &name = GetCursorName(cursor);
}

void BrowserBridge::OnUrlChanged(const CefString &url)
{
}

void BrowserBridge::browserEvent(WebviewEvent event)
{
}

// TODO: can be called from multiple parts of application (e.g. webview can be broken somewhere unexpectedly)
void BrowserBridge::OnWebviewStateChange(WebviewState state)
{
}

void BrowserBridge::HandleMethodCall(FlMethodCall *methodCall)
{
}

void BrowserBridge::loadUrl(const CefString &url)
{
  browser_->GetMainFrame()->LoadURL(url);
}

void BrowserBridge::loadHTML(std::string text)
{
  browser_->GetMainFrame()->LoadURL(GetDataURI(text, "text/html"));
}

void BrowserBridge::scrollUp()
{
  CefMouseEvent ev;
  ev.x = 500;
  ev.y = 500;
  browser_->GetHost()->SendMouseWheelEvent(ev, 0, -100);
}

void BrowserBridge::scrollDown()
{
  CefMouseEvent ev;
  ev.x = 500;
  ev.y = 500;
  browser_->GetHost()->SendMouseWheelEvent(ev, 0, 100);
}

void BrowserBridge::changeSize(int w, int h)
{
  this->width = w;
  this->height = h;
  browser_->GetHost()->WasResized();
}

void BrowserBridge::changeOffset(int x, int y)
{
  this->current_offset_x = x;
  this->current_offset_y = y;
}

void BrowserBridge::reload(bool ignoreCache)
{
  if (ignoreCache)
  {
    browser_->ReloadIgnoreCache();
  }
  else
  {
    browser_->Reload();
  }
}

void BrowserBridge::setZoomLevel(double level)
{
  browser_->GetHost()->SetZoomLevel(level);
}

double BrowserBridge::getZoomLevel()
{
  return browser_->GetHost()->GetZoomLevel();
}

void BrowserBridge::executeJavaScript(std::string js)
{
  browser_->GetMainFrame()->ExecuteJavaScript(js, browser_->GetMainFrame()->GetURL(), 0);
}

void BrowserBridge::cursorClick(int x, int y, bool up)
{
  CefMouseEvent ev;
  ev.x = x;
  ev.y = y;
  browser_->GetHost()->SetFocus(true);
  browser_->GetHost()->SendMouseClickEvent(ev, CefBrowserHost::MouseButtonType::MBT_LEFT, up, 1);
}

void BrowserBridge::sendKeyEvent(const CefKeyEvent &event)
{
  browser_->GetHost()->SendKeyEvent(event);
}

void BrowserBridge::sendMouseWheelEvent(CefMouseEvent &event,
                                        int deltaX,
                                        int deltaY)
{
  event.x = event.x - current_offset_x;
  event.y = event.y - current_offset_y;
  browser_->GetHost()->SendMouseWheelEvent(event, deltaX, deltaY);
}

void BrowserBridge::sendMouseClickEvent(CefMouseEvent &event,
                                        CefBrowserHost::MouseButtonType type,
                                        bool mouseUp,
                                        int clickCount)
{
  event.x = event.x - current_offset_x;
  event.y = event.y - current_offset_y;

  if (type == MBT_RIGHT && mouseUp == false)
  {
  }
  browser_->GetHost()->SetFocus(true);
  browser_->GetHost()->SendMouseClickEvent(event, type, mouseUp, clickCount);
}

void BrowserBridge::sendMouseMoveEvent(CefMouseEvent &event,
                                       bool mouseLeave)
{

  event.x = event.x - current_offset_x;
  event.y = event.y - current_offset_y;
  browser_->GetHost()->SendMouseMoveEvent(event, false);
}

void BrowserBridge::setCursorPos(int x, int y)
{
  CefMouseEvent ev;
  ev.x = x;
  ev.y = y;
  browser_->GetHost()->SendMouseMoveEvent(ev, false);
}

void BrowserBridge::closeBrowser(bool force)
{
  closing = true;
  browser_->GetHost()->CloseBrowser(force);
}

void BrowserBridge::resetBrowser()
{
  browser_.reset();
}